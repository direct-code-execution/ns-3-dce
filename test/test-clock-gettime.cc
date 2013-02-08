#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include "test-macros.h"

int main (int argc, char *argv[])
{
  // check time to see if a return from the futur occurs :)
  struct timespec first;
  struct timespec last;
  struct timespec now;

  int res = clock_gettime (CLOCK_MONOTONIC, &first);
  TEST_ASSERT_EQUAL (res, 0);

  struct timespec delta =
  {
    0, 999999999L - first.tv_nsec
  };
  res = nanosleep (&delta, 0);
  TEST_ASSERT_EQUAL (res, 0);

  res = clock_gettime (CLOCK_MONOTONIC, &last);
  TEST_ASSERT_EQUAL (res, 0);

  delta.tv_nsec = 2000000; // 2 ms
  res = nanosleep (&delta, 0);
  TEST_ASSERT_EQUAL (res, 0);

  res = clock_gettime (CLOCK_MONOTONIC, &now);
  TEST_ASSERT_EQUAL (res, 0);

  uint64_t n1 = now.tv_sec * 1000000000L + now.tv_nsec;
  uint64_t n2 = last.tv_sec * 1000000000L + last.tv_nsec;

  TEST_ASSERT (n1 > n2); // Time should always advance !

  return 0;
}
