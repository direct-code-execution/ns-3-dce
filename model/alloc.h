#ifndef ALLOC_H
#define ALLOC_H

#include <stdint.h>
#include <list>

class Alloc
{
public:
  virtual ~Alloc () = 0;
  virtual uint8_t * Malloc (uint32_t size) = 0;
  virtual void Free (uint8_t *buffer, uint32_t size) = 0;
  virtual uint8_t *Realloc(uint8_t *oldBuffer, uint32_t oldSize, uint32_t newSize) = 0;
private:
};

class StupidAlloc : public Alloc
{
public:
  StupidAlloc ();
  virtual ~StupidAlloc ();

  virtual uint8_t * Malloc (uint32_t size);
  virtual void Free (uint8_t *buffer, uint32_t size);
  virtual uint8_t *Realloc(uint8_t *oldBuffer, uint32_t oldSize, uint32_t newSize);
private:
  std::list<uint8_t *> m_alloced;
};

class KingsleyAlloc : public Alloc
{
public:
  KingsleyAlloc (void);
  virtual ~KingsleyAlloc ();

  virtual uint8_t * Malloc (uint32_t size);
  virtual void Free (uint8_t *buffer, uint32_t size);
  virtual uint8_t *Realloc(uint8_t *oldBuffer, uint32_t oldSize, uint32_t newSize);
private:
  struct MmapChunk
  {
    uint8_t *buffer;
    uint32_t size;
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


#endif /* ALLOC_H */
