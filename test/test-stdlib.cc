#include <stdlib.h>
#include <errno.h>
#include "test-macros.h"

static void test_strtod (void)
{
  double retval;
  retval = strtod ("55", NULL);
  TEST_ASSERT_EQUAL (retval, 55);
  retval = strtod ("-127", NULL);
  TEST_ASSERT_EQUAL (retval, -127);
  retval = strtod ("122.233", NULL);
  TEST_ASSERT_EQUAL (retval, 122.233);
  retval = strtod ("0x122.3", NULL);
  TEST_ASSERT_EQUAL (retval, 290.1875);
  retval = strtod ("2e3", NULL);
  TEST_ASSERT_EQUAL (retval, 2000);
}

int main (int argc, char *argv[])
{
  test_strtod ();
  return 0;
}
