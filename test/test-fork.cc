#include <unistd.h>
#include "test-macros.h"

static int g_static;
int g_global;

static void test_fork (void)
{
  int parent = 1;
  g_static = 1;
  g_global = 1;
  pid_t pid = fork ();
  if (pid == 0)
    {
      // child.
      TEST_ASSERT_EQUAL (parent, 1);
      TEST_ASSERT_EQUAL (g_static, 1);
      TEST_ASSERT_EQUAL (g_global, 1);
      parent = 2;
      g_static = 2;
      g_global = 2;
      sleep (2);
      TEST_ASSERT_EQUAL (parent, 2);
      TEST_ASSERT_EQUAL (g_static, 2);
      TEST_ASSERT_EQUAL (g_global, 2);
      sleep (10);
    }
  else
    {
      TEST_ASSERT_EQUAL (parent, 1);
      TEST_ASSERT_EQUAL (g_static, 1);
      TEST_ASSERT_EQUAL (g_global, 1);
      sleep (1);
      TEST_ASSERT_EQUAL (parent, 1);
      TEST_ASSERT_EQUAL (g_static, 1);
      TEST_ASSERT_EQUAL (g_global, 1);
      parent = 0;
      g_static = 0;
      g_global = 0;
      sleep (10);
      TEST_ASSERT_EQUAL (parent, 0);
      TEST_ASSERT_EQUAL (g_static, 0);
      TEST_ASSERT_EQUAL (g_global, 0);
    }
}

int main (int argc, char *argv[])
{
  test_fork ();
  return 0;
}
