#include <stdlib.h>
#include "test-macros.h"

int main (int argc, char *argv[])
{
  srand (0);
  int a = random ();
  int b = rand ();

  TEST_ASSERT_UNEQUAL (a, 0);
  TEST_ASSERT_UNEQUAL (a, b);

  exit (0);
  // never reached.
  return -1;
}
