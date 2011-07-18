/* Tests exec assuming that search path for executables are the sum of real PATH and real LD_LIBRARY_PATH */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "test-macros.h"

// Test ExecV with an invalid path then with a valid one.
int test1 ()
{
  char * args[] = { "test-exec", "2", 0 };
  int ret = execv ("test-exec", args);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  setenv ("CALLER", "TEST1", 1);
  ret = execv ("build/bin/test-exec", args);
  TEST_ASSERT( false ); // Must not be reached

  return ret;
}
// Test execvp
int test2 ()
{
  TEST_ASSERT_EQUAL ( strcmp ("TEST1", getenv ("CALLER")), 0 );
  setenv ("CALLER", "TEST2", 1);
  char * args[] = { "test-exec-not-existing", "3", 0 };
  int ret = execvp ("test-exec-not-existing", args);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execvp ("test-exec", args);
  TEST_ASSERT( false ); // Must not be reached

  return ret;
}
// Test execl
int test3 ()
{
  TEST_ASSERT_EQUAL ( strcmp ("TEST2", getenv ("CALLER")), 0 );
  setenv ("CALLER", "TEST3", 1);
  int ret = execl ("test-exec-not-existing", "4", 0 );
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execl ("build/bin/test-exec", "build/bin/test-exec", "4", 0);
  TEST_ASSERT( false ); // Must not be reached

  return ret;
}
// Test execve
int test4 ()
{
  TEST_ASSERT_EQUAL ( strcmp ("TEST3", getenv ("CALLER")), 0 );
  setenv ("CALLER", "", 1);

  char * args[] = { "test-exec-not-existing", "5", 0 };
  char * env[] = { "CALLER=TEST4", "LD_LIBRARY_PATH=./build/lib", 0 };

  int ret = execve ("test-exec-not-existing", args, env);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  {
    char * args[] = { "build/bin/test-exec", "5", 0 };

    ret = execve ("build/bin/test-exec", args, env);
    TEST_ASSERT( false ); // Must not be reached
  }

  return ret;
}
// Test execlp
int test5 ()
{
  TEST_ASSERT_EQUAL ( strcmp ("TEST4", getenv ("CALLER")), 0 );
  setenv ("CALLER", "TEST5", 1);
  int ret = execlp ("test-exec-not-existing", "6", 0 );
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execlp ("build/bin/test-exec", "build/bin/test-exec", "6", 0);
  TEST_ASSERT( false ); // Must not be reached

  return ret;
}
// Test execle
int test6 ()
{
  TEST_ASSERT_EQUAL ( strcmp ("TEST5", getenv ("CALLER")), 0 );
  setenv ("CALLER", "", 1);
  char * env[] = { "CALLER=TEST6", "LD_LIBRARY_PATH=./build/lib", 0 };
  int ret = execle ("test-exec", "test-exec",  "7", 0 , env);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execle ("build/bin/test-exec", "build/bin/test-exec", "7", (char*)0 , env );
  printf ("execle -> %d, errno=%d\n", ret, errno);
  TEST_ASSERT( false ); // Must not be reached

  return ret;
}

int last_test ()
{
  TEST_ASSERT_EQUAL ( strcmp ("TEST6", getenv ("CALLER")), 0 );
  printf ("Last test.\n");
  return 0;
}

int main (int c, char **v)
{
  printf ("%s: argc=%d", v[0], c);
  if ( c == 1)
    {
      printf("\n");
      return test1 ();
    }
  printf(" %d\n", atoi (v[1]));

  switch (atoi(v[1]))
  {
    case 2: return test2 ();
    case 3: return test3 ();
    case 4: return test4 ();
    case 5: return test5 ();
    case 6: return test6 ();

    default: return last_test ();
  }

}
