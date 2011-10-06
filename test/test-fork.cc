#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
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
      printf ("I am the child ! \n");
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
      exit (1);
    }
  else
    {
      printf ("I am the father, my son pid is %d\n", pid);
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
      //     sleep (30);
    }
}

static void *
test_wait_fork_thread (void *arg)
{
  pid_t pid = *((pid_t*) arg );
  int status = 0;

  printf ("Thread about to wait \n");

  pid_t w = wait (&status);

  printf ("test wait fork thread: after wait >>> %d,  status = %d signal %d \n", w,  WEXITSTATUS ( status), WTERMSIG (status) );

  return 0;
}

// A fork , father 2 thread with two wait !
static void test_wait_fork (void)
{
  int parent = 1;
  g_static = 1;
  g_global = 1;
  pid_t pid = fork ();
  if (pid == 0)
    {
      // child.
      printf ("I am the child ! \n");
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
      sleep (20);
      exit (2);
    }
  else
    {
      printf ("I am the father, my son pid is %d\n", pid);
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

      pthread_t thread;
      int status = pthread_create (&thread, NULL, &test_wait_fork_thread, (void*)&pid);
      sleep (1);
      pid_t w =  waitpid ( -1, &status, WNOHANG);
      if ( 0 == w )
        {
          printf ("child not yet ended.\n");
          w = waitpid ( pid, &status, 0);
        }
      printf ("father wait fork thread: after wait >>> %d, status = %d signal %d \n", w,  WEXITSTATUS ( status), WTERMSIG (status) );

      pthread_join (thread, 0);
    }
}

void big_fork (int prof)
{
  while (true)
    {
      pid_t pid = fork ();

      if ( -1 == pid )
        {
          printf ("fork failed errno:%d, prof=%d\n", errno, prof);
          return;
        }
      if (pid)
        {
          int st = 0;

          pid_t w =  waitpid ( pid, &st, 0);
          printf ("Child pid:%d %d\n", pid, w);
          if (prof>0) exit (0);
          return;
        }
      else
        {
          if (prof++ >= 100) exit (0);
          printf ("pid:%d prof%d\n",getpid (), prof);
        }
    }
}

void big_fork_exec (int prof)
{
  while (true)
    {
      pid_t pid = fork ();

      if ( -1 == pid )
        {
          printf ("fork failed errno:%d, prof=%d\n", errno, prof);
          return;
        }
      if (pid)
        {
          int st = 0;

          pid_t w =  waitpid ( pid, &st, 0);
          printf ("Child pid:%d %d\n", pid, w);
          if (prof>0) exit (0);
          return;
        }
      else
        {
          if (prof++ >= 100) exit (0);

          char arg[100];

          sprintf (arg,"%d",prof);

          static char* const args[] = { "build/bin/test-fork", arg };

          int ret = execv ("build/bin/test-fork", args);

          return;
        }
    }
}

int main (int argc, char *argv[])
{
  if (argc == 1)
    {
      test_fork ();

      pid_t w = wait (0);

      test_wait_fork ();

      big_fork (0);

      printf ("pid: %d after \n",getpid ());
      big_fork_exec (0);
    }
  else
    {
      big_fork_exec (argc > 1 ? (atoi (argv[1])) : 0 );
    }
  return 0;
}
