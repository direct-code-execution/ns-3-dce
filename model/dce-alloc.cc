#include "dce-stdlib.h"
#include "dce-unistd.h"
#include "utils.h"
#include "process.h"
#include "kingsley-alloc.h"
#include "ns3/log.h"
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("SimuAlloc");

using namespace ns3;

void * dce_calloc (size_t nmemb, size_t size)
{
  GET_CURRENT (nmemb << size);
  void *ptr = dce_malloc (nmemb * size);
  memset (ptr, 0, nmemb * size);
  return ptr;
}
void * dce_malloc (size_t size)
{
  GET_CURRENT (size);
  size += sizeof (size_t);
  uint8_t *buffer = current->process->alloc->Malloc (size);
  memcpy (buffer, &size, sizeof (size_t));
  buffer += sizeof (size_t);
  NS_LOG_DEBUG ("alloc=" << (void*)buffer);
  return buffer;
}
void dce_free (void *ptr)
{
  GET_CURRENT (ptr);
  if (ptr == 0)
    {
      return;
    }
  uint8_t *buffer = (uint8_t*)ptr;
  size_t size;
  buffer -= sizeof (size_t);
  memcpy (&size, buffer, sizeof (size_t));
  current->process->alloc->Free (buffer, size);
}
void * dce_realloc (void *ptr, size_t size)
{
  GET_CURRENT (ptr << size);
  if (ptr == 0 && size == 0)
    {
      return 0;
    }
  if (ptr == 0)
    {
      return dce_malloc (size);
    }
  size_t oldSize;
  uint8_t *buffer = (uint8_t*)ptr;
  buffer -= sizeof (size_t);
  size += sizeof (size_t);
  memcpy (&oldSize, buffer, sizeof (size_t));
  if (size <= oldSize)
    {
      return ptr;
    }
  buffer = current->process->alloc->Realloc (buffer, oldSize, size);
  memcpy (buffer, &size, sizeof (size_t));
  buffer += sizeof (size_t);
  return buffer;
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
