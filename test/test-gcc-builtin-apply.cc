#include <stdlib.h>
#include "test-macros.h"


void test_atof (void)
{
  double d_val = atof ("10.0");
  int32_t i_val = (int32_t) d_val;
  TEST_ASSERT_EQUAL (d_val, 10.0);
  TEST_ASSERT_EQUAL (i_val, 10);
}

int main (int argc, char *argv[])
{
  test_atof ();
  return 0;
}
