#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "test-macros.h"

int main (int argc, char *argv[])
{
  time_t start = time (0);
  unsigned int left = sleep (1);
  time_t end = time (0);
  TEST_ASSERT_EQUAL (left, 0);
  TEST_ASSERT_EQUAL (end - start, 1);
  exit (0);
  // never reached.
  return -1;
}
