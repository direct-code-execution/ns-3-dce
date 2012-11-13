#include <stdlib.h>
#include <unistd.h>
#include "test-macros.h"

static bool environ_search (const char *str)
{
  for (char **cur = environ; *cur != 0; cur++)
    {
      if (strcmp (str, *cur) == 0)
        {
          return true;
        }
    }
  return false;
}

void test_environ (void)
{
  clearenv ();
  setenv ("F", "10", 1);
  TEST_ASSERT (environ_search ("F=10"));
  setenv ("E", "9", 1);
  TEST_ASSERT (environ_search ("F=10"));
  TEST_ASSERT (environ_search ("E=9"));
  setenv ("D", "8", 1);
  TEST_ASSERT (environ_search ("F=10"));
  TEST_ASSERT (environ_search ("E=9"));
  TEST_ASSERT (environ_search ("D=8"));
  unsetenv ("E");
  TEST_ASSERT (environ_search ("F=10"));
  TEST_ASSERT (!environ_search ("E=9"));
  TEST_ASSERT (environ_search ("D=8"));
}

void test_putenv (void)
{
  clearenv ();
  char *nothing = getenv ("THIS_IS_A_TEST");
  TEST_ASSERT_EQUAL (nothing, 0);
  char *shel = getenv ("SHEL");
  TEST_ASSERT_EQUAL (shel, 0);

  int status = putenv ((char*)"SHEL=foo");
  TEST_ASSERT_EQUAL (status, 0);
  shel = getenv ("SHEL");
  TEST_ASSERT_UNEQUAL (shel, 0);
  TEST_ASSERT (strcmp (shel, "foo") == 0);
  status = putenv ((char*)"SHEL=bar");
  TEST_ASSERT_EQUAL (status, 0);
  shel = getenv ("SHEL");
  TEST_ASSERT_UNEQUAL (shel, 0);
  TEST_ASSERT (strcmp (shel, "bar") == 0);

  status = putenv ((char*)"SHEL");
  TEST_ASSERT_EQUAL (status, 0);
  shel = getenv ("SHEL");
  TEST_ASSERT_EQUAL (shel, 0);

  status = putenv ((char*)"A=a");
  TEST_ASSERT_EQUAL (status, 0);
  status = putenv ((char*)"B=b");
  TEST_ASSERT_EQUAL (status, 0);
  status = putenv ((char*)"C=c");
  TEST_ASSERT_EQUAL (status, 0);
  status = putenv ((char*)"B=b");
  TEST_ASSERT_EQUAL (status, 0);
  status = putenv ((char*)"B");
  TEST_ASSERT_EQUAL (status, 0);
  shel = getenv ("A");
  TEST_ASSERT_UNEQUAL (shel, 0);
  shel = getenv ("B");
  TEST_ASSERT_EQUAL (shel, 0);
  shel = getenv ("C");
  TEST_ASSERT_UNEQUAL (shel, 0);
}

void test_setenv (void)
{
  int status = clearenv ();
  TEST_ASSERT_EQUAL (status, 0);
  char *shel = getenv ("A");
  TEST_ASSERT_EQUAL (shel, 0);
  shel = getenv ("B");
  TEST_ASSERT_EQUAL (shel, 0);
  shel = getenv ("C");
  TEST_ASSERT_EQUAL (shel, 0);

  status = setenv ("A", "a", 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = setenv ("B", "b", 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = setenv ("C", "c", 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = setenv ("B", "c", 0);
  TEST_ASSERT_EQUAL (status, 0);
  shel = getenv ("B");
  TEST_ASSERT (strcmp (shel, "b") == 0);
  status = setenv ("B", "c", 1);
  TEST_ASSERT_EQUAL (status, 0);
  shel = getenv ("B");
  TEST_ASSERT (strcmp (shel, "c") == 0);
  status = unsetenv ("B");
  TEST_ASSERT_EQUAL (status, 0);
  status = unsetenv ("D");
  TEST_ASSERT_EQUAL (status, 0);
  shel = getenv ("A");
  TEST_ASSERT_UNEQUAL (shel, 0);
  shel = getenv ("B");
  TEST_ASSERT_EQUAL (shel, 0);
  shel = getenv ("C");
  TEST_ASSERT_UNEQUAL (shel, 0);

}

int main (int argc, char *argv[])
{
  test_putenv ();
  test_setenv ();
  test_environ ();
  return 0;
}
