#include <unistd.h>
#include "test-macros.h"

static void test_fork (void)
{
  int parent = 1;
  pid_t pid = fork ();
  if (pid == 0)
    {
      // child.
      TEST_ASSERT_EQUAL (parent, 1);
      parent = 2;
      sleep (2);
      TEST_ASSERT_EQUAL (parent, 2);
      sleep (10);
    }
  else
    {
      TEST_ASSERT_EQUAL (parent, 1);
      sleep (1);
      TEST_ASSERT_EQUAL (parent, 1);
      parent = 0;
      sleep (10);
      TEST_ASSERT_EQUAL (parent, 0);
    }
}

int main (int argc, char *argv[])
{
  test_fork ();
  return 0;
}
