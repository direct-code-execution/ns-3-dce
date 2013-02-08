/* Tests exec assuming that search path for executables are the sum of real PATH and real LD_LIBRARY_PATH */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "test-macros.h"

// Test ExecV with an invalid path then with a valid one.
int test1 ()
{
  const char * args[] = { "test-exec", "2", 0 };
  int ret = execv ("test-exec", (char * const *) args);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  setenv ("CALLER", "TEST1", 1);
  ret = execv ("/bin_dce/test-exec", (char * const *)args);
  TEST_ASSERT (false); // Must not be reached

  return ret;
}
// Test execvp
int test2 ()
{
  TEST_ASSERT_EQUAL (strcmp ("TEST1", getenv ("CALLER")), 0);
  setenv ("CALLER", "TEST2", 1);
  const char * args[] = { "test-exec-not-existing", "3", 0 };
  int ret = execvp ("test-exec-not-existing", (char * const *)args);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execvp ("test-exec", (char * const *)args);
  TEST_ASSERT (false); // Must not be reached

  return ret;
}
// Test execl
int test3 ()
{
  TEST_ASSERT_EQUAL (strcmp ("TEST2", getenv ("CALLER")), 0);
  setenv ("CALLER", "TEST3", 1);
  int ret = execl ("test-exec-not-existing", "4", NULL);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execl ("/bin_dce/test-exec", "/bin_dce/test-exec", "4", NULL);
  TEST_ASSERT (false); // Must not be reached

  return ret;
}
// Test execve
int test4 ()
{
  TEST_ASSERT_EQUAL (strcmp ("TEST3", getenv ("CALLER")), 0);
  setenv ("CALLER", "", 1);

  const char * args[] = { "test-exec-not-existing", "5", 0 };
  const char * env[] = { "CALLER=TEST4", "LD_LIBRARY_PATH=./build/lib", 0 };

  int ret = execve ("test-exec-not-existing", (char * const *)args, (char * const *)env);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  {
    const char * args[] = { "build/bin/test-exec", "5", 0 };

    ret = execve ("/bin_dce/test-exec", (char * const *)args, (char * const *)env);
    TEST_ASSERT (false); // Must not be reached
  }

  return ret;
}
// Test execlp
int test5 ()
{
  TEST_ASSERT_EQUAL (strcmp ("TEST4", getenv ("CALLER")), 0);
  setenv ("CALLER", "TEST5", 1);
  int ret = execlp ("test-exec-not-existing", "6", NULL);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execlp ("test-exec", "test-exec", "6", NULL);
  TEST_ASSERT (false); // Must not be reached

  return ret;
}
// Test execle
int test6 ()
{
  TEST_ASSERT_EQUAL (strcmp ("TEST5", getenv ("CALLER")), 0);
  setenv ("CALLER", "", 1);
  const char * env[] = { "CALLER=TEST6", "LD_LIBRARY_PATH=./build/lib", 0 };
  int ret = execle ("test-exec", "test-exec",  "7", NULL, (char * const *) env);
  TEST_ASSERT_EQUAL (ret, -1);
  TEST_ASSERT_EQUAL (errno,  ENOENT);

  ret = execle ("/bin_dce/test-exec", "/bin_dce/test-exec", "7", NULL, (char * const *) env);
  printf ("execle -> %d, errno=%d\n", ret, errno);
  TEST_ASSERT (false); // Must not be reached

  return ret;
}

static void *
thread_test7 (void *arg)
{
  struct pollfd fds;
  int ret = -1;

  fds.fd = 0;
  fds.events = POLLIN;
  fds.revents = 0;

  // Block on stdin
  ret = poll (&fds, 1, -1);

  return arg;
}

// Create some thread before execing
int test7 ()
{
  for (int i = 0; i < 30; i++)
    {
      int status;
      pthread_t thread;

      // try to join after the thread exits.
      status = pthread_create (&thread, NULL, &thread_test7, 0);
    }
  int ret = execl ("/bin_dce/test-exec", "/bin_dce/test-exec", "8", NULL);
  TEST_ASSERT (false); // Must not be reached

  return ret;
}

int test8 ()
{
  int fd = open ("Script8", O_CREAT | O_WRONLY, 0755);
  FILE *f = fdopen (fd, "w");
  char *argv[] = { 0 };
  char *envp[] = {  0 };

  TEST_ASSERT_UNEQUAL (f, 0);

  fprintf (f,"#!/bin/sh\n");
  fprintf (f,"CALLER=TEST6\n");
  fprintf (f,"export CALLER\n");
  fprintf (f,"/bin_dce/test-exec 9\n");
  fclose (f);

  int ret = execve  ("Script8", argv, envp);
  TEST_ASSERT (false); // Must not be reached

  return ret;
}
int last_test ()
{
  TEST_ASSERT_EQUAL (strcmp ("TEST6", getenv ("CALLER")), 0);
  sleep (1);
  printf ("Last test.\n");
  return 0;
}
extern char *__progname;
int main (int c, char **v)
{
  printf ("ProgName: %s %s %s \n", __progname, program_invocation_name, program_invocation_short_name);
  printf ("%s: argc=%d", v[0], c);
  if (c == 1)
    {
      printf ("\n");
      return test1 ();
    }
  printf (" %d\n", atoi (v[1]));

  switch (atoi (v[1]))
    {
    case 2:
      return test2 ();
    case 3:
      return test3 ();
    case 4:
      return test4 ();
    case 5:
      return test5 ();
    case 6:
      return test6 ();
    case 7:
      return test7 ();
    // TEST 8 Disabled because this test need a sh recompiled for DCE which is not
    // part of the DCE distribution for now.
    // case 8: return test8 ();

    default:
      return last_test ();
    }

}
