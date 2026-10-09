#include "dce-stdlib.h"
#include <errno.h>
#include <stdint.h>
#include "dce-unistd.h"
#include "utils.h"
#include "process.h"
#include "kingsley-alloc.h"
#include "ns3/log.h"
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("DceAlloc");

using namespace ns3;

// Every block handed to the application is preceded by a 16 byte header:
//
//   [ address of the KingsleyAlloc buffer ][ size of that buffer ] [ block... ]
//
// and is aligned on 16 bytes, like glibc's malloc (compilers rely on it,
// e.g. movaps on struct fields), or on the alignment posix_memalign() asks
// for. free() and realloc() find the underlying buffer in the header
// whatever the alignment.
static const size_t HEADER = 2 * sizeof (size_t);

static void *
Allocate (Thread *current, size_t size, size_t alignment)
{
  if (alignment < 16)
    {
      alignment = 16;
    }
  size_t total = size + HEADER + alignment;
  uint8_t *raw = current->process->alloc->Malloc (total);
  if (raw == 0)
    {
      return 0;
    }
  uintptr_t aligned = ((uintptr_t)raw + HEADER + alignment - 1) & ~(uintptr_t)(alignment - 1);
  uint8_t *ptr = (uint8_t *)aligned;
  memcpy (ptr - HEADER, &raw, sizeof (raw));
  memcpy (ptr - sizeof (size_t), &total, sizeof (total));
  NS_LOG_DEBUG ("alloc=" << (void*)ptr << " raw=" << (void*)raw << " size=" << size);
  return ptr;
}

static void
Header (void *ptr, uint8_t **raw, size_t *total)
{
  memcpy (raw, (uint8_t *)ptr - HEADER, sizeof (*raw));
  memcpy (total, (uint8_t *)ptr - sizeof (size_t), sizeof (*total));
}

void * dce_calloc (size_t nmemb, size_t size)
{
  GET_CURRENT (nmemb << size);
  if (size != 0 && nmemb > SIZE_MAX / size)
    {
      current->err = ENOMEM;
      return 0;
    }
  void *ptr = dce_malloc (nmemb * size);
  if (ptr != 0 && !current->process->alloc->IsFreshMapping (nmemb * size + HEADER + 16))
    {
      // Large blocks are fresh anonymous mappings, already zero: clearing
      // them again cost GTK's software renderer a large share of each frame.
      memset (ptr, 0, nmemb * size);
    }
  return ptr;
}

void * dce_malloc (size_t size)
{
  GET_CURRENT (size);
  void *ptr = Allocate (current, size, 16);
  if (ptr == 0)
    {
      current->err = ENOMEM;
    }
  return ptr;
}

size_t dce_malloc_usable_size (void *ptr)
{
  if (ptr == 0)
    {
      return 0;
    }
  uint8_t *raw;
  size_t total;
  Header (ptr, &raw, &total);
  return total - ((uint8_t *)ptr - raw);
}

void dce_free (void *ptr)
{
  GET_CURRENT (ptr);
  if (ptr == 0)
    {
      return;
    }
  uint8_t *raw;
  size_t total;
  Header (ptr, &raw, &total);
  current->process->alloc->Free (raw, total);
}

void * dce_realloc (void *ptr, size_t size)
{
  GET_CURRENT (ptr << size);
  if (ptr == 0)
    {
      return dce_malloc (size);
    }
  if (size == 0)
    {
      dce_free (ptr);
      return 0;
    }
  size_t usable = dce_malloc_usable_size (ptr);
  if (size <= usable)
    {
      return ptr;
    }
  void *fresh = Allocate (current, size, 16);
  if (fresh == 0)
    {
      current->err = ENOMEM;
      return 0;
    }
  memcpy (fresh, ptr, usable);
  dce_free (ptr);
  return fresh;
}

int dce_posix_memalign (void **memptr, size_t alignment, size_t size)
{
  GET_CURRENT (alignment << size);
  if (memptr == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0)
    {
      return EINVAL;
    }
  void *ptr = Allocate (current, size, alignment);
  if (ptr == 0)
    {
      return ENOMEM;
    }
  *memptr = ptr;
  return 0;
}

void * dce_memalign (size_t alignment, size_t size)
{
  void *ptr = 0;
  int err = dce_posix_memalign (&ptr, alignment, size);
  if (err != 0)
    {
      Current ()->err = err;
      return 0;
    }
  return ptr;
}

void * dce_aligned_alloc (size_t alignment, size_t size)
{
  return dce_memalign (alignment, size);
}
void * dce_sbrk (intptr_t increment)
{
  if (0  == increment)
    {
      return (void*)-1;
    }
  return dce_calloc (1, increment);
}
int dce_getpagesize (void)
{
  return sysconf (_SC_PAGESIZE);
}
