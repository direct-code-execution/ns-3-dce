#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <list>

int main (int argc, char *argv[])
{
  int sizes[] = { 0, 1, 2, 3, 4, 8, 10, 16, 19, 30, 64, 120, 240, 1020, 4098, 10000, 100000, 1000000};
  for (uint32_t i = 0; i < sizeof (sizes) / sizeof (int); i++)
    {
      int size = sizes[i];
      void *ptr = malloc (size);
      memset (ptr, 0x66, size);
      free (ptr);
    }
  std::list<void*> ptrs;
  for (uint32_t i = 0; i < sizeof (sizes) / sizeof (int); i++)
    {
      int size = sizes[i];
      void *ptr = malloc (size);
      memset (ptr, 0x66, size);
      ptrs.push_back (ptr);
    }
  for (uint32_t i = 0; i < sizeof (sizes) / sizeof (int); i++)
    {
      free (ptrs.front ());
      ptrs.pop_front ();
    }
  ptrs.clear ();

  return 0;
}
