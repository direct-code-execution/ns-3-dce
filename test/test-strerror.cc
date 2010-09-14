#include <string.h>
#include <errno.h>
#include "test-macros.h"

int main (int argc, char *argv[])
{
  char buffer[100];
  char *s = strerror (EINVAL);
  TEST_ASSERT_UNEQUAL (s, 0);
  s = strerror_r (EINVAL, buffer, 100);
  TEST_ASSERT_UNEQUAL (s, 0);
  s = strerror_r (EINVAL, buffer, 0);
  TEST_ASSERT_UNEQUAL (s, 0);

  return 0;
}
