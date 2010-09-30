#ifndef KINGSLEY_ALLOC_H
#define KINGSLEY_ALLOC_H

#include <stdint.h>
#include <list>

class KingsleyAlloc
{
public:
  KingsleyAlloc (void);
  ~KingsleyAlloc ();

  KingsleyAlloc *Clone (void);
  void SwitchTo (void);
  uint8_t * Malloc (uint32_t size);
  void Free (uint8_t *buffer, uint32_t size);
  uint8_t *Realloc(uint8_t *oldBuffer, uint32_t oldSize, uint32_t newSize);
private:
  struct Mmap
  {
    uint32_t refcount;
    uint32_t size;
    uint8_t *buffer;
    uint8_t *current;
  };

  struct MmapChunk
  {
    struct Mmap *mmap;
    uint8_t *copy;
    uint32_t brk;
  };
  struct Available
  {
    struct Available *next;
  };
  void MmapAlloc (uint32_t size);
  void MmapFree (uint8_t *buffer, uint32_t size);
  uint8_t *Brk (uint32_t needed);
  uint8_t SizeToBucket (uint32_t size);
  uint32_t BucketToSize (uint8_t bucket);
  
  std::list<struct KingsleyAlloc::MmapChunk> m_chunks;
  struct Available *m_buckets[32];
  uint32_t m_defaultMmapSize;
};


#endif /* KINGSLEY_ALLOC_H */
