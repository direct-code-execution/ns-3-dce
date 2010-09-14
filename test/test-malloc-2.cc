#include <stdlib.h>

int
main (int argc, char *argv[])
{
  void *a = malloc (32000);
  void *b = malloc (2000);
  free (a);
  free (b);

  return 0;
}
