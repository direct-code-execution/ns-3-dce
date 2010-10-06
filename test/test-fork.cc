#include <unistd.h>
#include "test-macros.h"

static void test_fork (void)
{
  int parent = 1;
  pid_t pid = fork ();
  if (pid == 0)
    {
      // child.
      parent = 2;
      sleep (2);
      TEST_ASSERT_EQUAL (parent, 2);
    }
  else
    {
      sleep (1);
      parent = 0;
      sleep (10);
    }
}

int main (int argc, char *argv[])
{
  test_fork ();
  return 0;
}
