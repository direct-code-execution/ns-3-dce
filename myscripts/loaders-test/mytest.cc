#include <stdio.h>
#include <unistd.h>
#include "mylib.h"

int main (int c, char **v)
{
  printf ("get_thing () -> %d\n", get_thing ());

  test_singleton ();

  sleep (18);

  return 0;
}
