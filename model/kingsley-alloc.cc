#include "kingsley-alloc.h"
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "ns3/assert.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("Alloc");

#ifdef HAVE_VALGRIND_H
# include "valgrind/valgrind.h"
# include "valgrind/memcheck.h"
# define REPORT_MALLOC(buffer, size) \
  VALGRIND_MALLOCLIKE_BLOCK (buffer,size, 0, 0)
# define REPORT_FREE(buffer) \
  VALGRIND_FREELIKE_BLOCK (buffer, 0)
# define MARK_DEFINED(buffer, size)                             \
  VALGRIND_MAKE_MEM_DEFINED (buffer, size)
# define MARK_UNDEFINED(buffer, size)                   \
  VALGRIND_MAKE_MEM_UNDEFINED (buffer, size)
#else
# define REPORT_MALLOC(buffer, size)
# define REPORT_FREE(buffer)
# define MARK_DEFINED(buffer, size)
# define MARK_UNDEFINED(buffer, size)
#endif


KingsleyAlloc::KingsleyAlloc ()
  : m_defaultMmapSize (1 << 15)
{
  NS_LOG_FUNCTION (this);
  memset (m_buckets, 0, sizeof(m_buckets));
}
KingsleyAlloc::~KingsleyAlloc ()
{
  NS_LOG_FUNCTION (this);
  // return;
  for (std::list<struct KingsleyAlloc::MmapChunk>::iterator i = m_chunks.begin ();
       i != m_chunks.end (); ++i)
    {
      if (i->copy)
        {
          // ok, this means that _our_ buffer is not the
          // original mmap buffer which means that we were
          // cloned once so, we need to free our local
          // buffer.
          free (i->copy);

          if (i->copy == i->mmap->current)
            {
              // Current must be nullify because we the next switch of context do not need to save our heap.
              i->mmap->current = 0;
            }
          i->copy = 0;
        }
      i->mmap->refcount--;
      if (i->mmap->refcount == 0)
        {
          // we are the last to release this chunk.
          // so, release the mmaped data.
          MmapFree (i->mmap->buffer, i->mmap->size);
          delete i->mmap;
        }
    }
  m_chunks.clear ();
}
// Call me only from my context
void
KingsleyAlloc::Dispose ()
{
  NS_LOG_FUNCTION (this);
  for (std::list<struct KingsleyAlloc::MmapChunk>::iterator i = m_chunks.begin ();
       i != m_chunks.end (); ++i)
    {
      if (i->copy == i->mmap->current)
        {
          // Current must be nullify because we the next switch of context do not need to save our heap.
          i->mmap->current = 0;
        }
    }
}
KingsleyAlloc *
KingsleyAlloc::Clone (void)
{
  NS_LOG_FUNCTION (this << "begin");
  KingsleyAlloc *clone = new KingsleyAlloc ();
  *clone->m_buckets = *m_buckets;
  for (std::list<struct KingsleyAlloc::MmapChunk>::iterator i = m_chunks.begin ();
       i != m_chunks.end (); ++i)
    {
      struct KingsleyAlloc::MmapChunk chunk = *i;
      chunk.mmap->refcount++;
      if ((chunk.mmap->refcount == 2)&&(0 == chunk.copy))
        {
          // this is the first clone of this heap so, we first
          // create buffer copies for ourselves
          chunk.mmap->current = i->copy = (uint8_t *)malloc (chunk.mmap->size);
        }
      // now, we create a buffer copy for the clone
      struct KingsleyAlloc::MmapChunk chunkClone = chunk;
      chunkClone.copy = (uint8_t *)malloc (chunkClone.mmap->size);
      // Save the heap in the clone copy memory
      memcpy (chunkClone.copy, chunk.mmap->buffer, chunk.mmap->size);
      clone->m_chunks.push_back (chunkClone);
    }
  NS_LOG_FUNCTION (this << "end");
  return clone;
}

void
KingsleyAlloc::SwitchTo (void)
{
  NS_LOG_FUNCTION (this);
  for (std::list<struct KingsleyAlloc::MmapChunk>::const_iterator i = m_chunks.begin ();
       i != m_chunks.end (); ++i)
    {
      struct KingsleyAlloc::MmapChunk chunk = *i;

      // save the previous user's heap if necessary
      if (chunk.mmap->current && (chunk.mmap->current != chunk.mmap->buffer))
        {
          memcpy (chunk.mmap->current, chunk.mmap->buffer, chunk.mmap->size);
        }

      // swap in our own copy of the heap if necessary
      if (chunk.copy && (chunk.mmap->buffer != chunk.copy))
        {
          memcpy (chunk.mmap->buffer, chunk.copy, chunk.mmap->size);
        }
      // and, now, remember that _we_ own the heap
      chunk.mmap->current = chunk.copy;
    }
}

void
KingsleyAlloc::MmapFree (uint8_t *buffer, uint32_t size)
{
  NS_LOG_FUNCTION (this << (void*)buffer << size);
  int status;
  status = ::munmap (buffer, size);
  NS_ASSERT_MSG (status == 0, "Unable to release mmaped buffer");
}
void
KingsleyAlloc::MmapAlloc (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  struct Mmap *mmap_struct = new Mmap ();
  mmap_struct->refcount = 1;
  mmap_struct->size = size;
  mmap_struct->buffer = (uint8_t*)::mmap (0, size, PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  NS_ASSERT_MSG (mmap_struct->buffer != MAP_FAILED, "Unable to mmap memory buffer");
  mmap_struct->current = mmap_struct->buffer;
  struct MmapChunk chunk;
  chunk.mmap = mmap_struct;
  chunk.brk = 0;
  chunk.copy = 0; // no clone yet, no copy yet.

  m_chunks.push_front (chunk);
  NS_LOG_DEBUG ("mmap alloced=" << size << " at=" << (void*)mmap_struct->buffer);
  MARK_UNDEFINED (mmap_struct->buffer, size);
}

uint8_t *
KingsleyAlloc::Brk (uint32_t needed)
{
  NS_LOG_FUNCTION (this << needed);
  for (std::list<struct KingsleyAlloc::MmapChunk>::iterator i = m_chunks.begin ();
       i != m_chunks.end (); ++i)
    {
      NS_ASSERT (i->mmap->size >= i->brk);
      if (i->mmap->size - i->brk >= needed)
        {
          uint8_t *buffer = i->mmap->buffer + i->brk;
          i->brk += needed;
          NS_LOG_DEBUG ("brk: needed=" << needed << ", left=" << i->mmap->size - i->brk);
          return buffer;
        }
    }
  NS_ASSERT_MSG (needed <= m_defaultMmapSize, needed << " " << m_defaultMmapSize);
  MmapAlloc (m_defaultMmapSize);
  return Brk (needed);
}
uint8_t
KingsleyAlloc::SizeToBucket (uint32_t sz)
{
  NS_LOG_FUNCTION (this << sz);
  uint8_t bucket = 0;
  uint32_t size = sz;
  size--;
  while (size > 7)
    {
      size >>= 1;
      bucket++;
    }
  NS_ASSERT (bucket < 32);
  NS_LOG_DEBUG ("size=" << sz << ", bucket=" << (uint32_t)bucket << ", size=" <<
                BucketToSize (bucket));
  return bucket;
}
uint32_t
KingsleyAlloc::BucketToSize (uint8_t bucket)
{
  uint32_t size = (1 << (bucket + 3));
  return size;
}

uint8_t *
KingsleyAlloc::Malloc (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  if (size < m_defaultMmapSize)
    {
      uint8_t bucket = SizeToBucket (size);
      if (m_buckets[bucket] == 0)
        {
          struct Available *avail = (struct Available *)Brk (BucketToSize (bucket));
          MARK_DEFINED (avail, sizeof(void*));
          avail->next = 0;
          MARK_UNDEFINED (avail, sizeof(void*));
          m_buckets[bucket] = avail;
        }
      // fast path.
      struct Available *avail = m_buckets[bucket];
      MARK_DEFINED (avail, sizeof(void*));
      m_buckets[bucket] = avail->next;
      MARK_UNDEFINED (avail, sizeof(void*));
      REPORT_MALLOC (avail, size);
      return (uint8_t*)avail;
    }
  else
    {
      MmapAlloc (size);
      uint8_t *buffer = Brk (size);
      REPORT_MALLOC (buffer, size);
      return buffer;
    }
}
void
KingsleyAlloc::Free (uint8_t *buffer, uint32_t size)
{
  NS_LOG_FUNCTION (this << (void*)buffer << size);
  if (size < m_defaultMmapSize)
    {
      // return to bucket list.
      uint8_t bucket = SizeToBucket (size);
      struct Available *avail = (struct Available *)buffer;
      avail->next = m_buckets[bucket];
      m_buckets[bucket] = avail;
      REPORT_FREE (buffer);
    }
  else
    {
      for (std::list<struct KingsleyAlloc::MmapChunk>::iterator i = m_chunks.begin ();
           i != m_chunks.end (); ++i)
        {
          if (i->mmap->buffer == buffer && i->mmap->size == size)
            {
              REPORT_FREE (buffer);
              MmapFree (buffer, size);
              m_chunks.erase (i);
              return;
            }
        }
      // this should never happen but it happens in case of a double-free
      REPORT_FREE (buffer);
    }
}
uint8_t *
KingsleyAlloc::Realloc (uint8_t *oldBuffer, uint32_t oldSize, uint32_t newSize)
{
  NS_LOG_FUNCTION (this << (void*)oldBuffer << oldSize << newSize);
  if (newSize < oldSize)
    {
      return oldBuffer;
    }
  uint8_t *newBuffer = Malloc (newSize);
  memcpy (newBuffer, oldBuffer, oldSize);
  Free (oldBuffer, oldSize);
  return newBuffer;
}
