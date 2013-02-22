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

  char * toto = strdup ("tintin");
  char * houla = (char*)42;

  printf ("Before Fork toto: %p %s \nhoula %p\n", toto, toto, houla);

  pid_t pid = fork ();



  if (pid == 0)
    {
      // child.
      printf ("CHILD toto: %p %s \nhoula %p\n", toto, toto, houla);
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
      printf ("FATHER toto: %p %s \nhoula %p\n", toto, toto, houla);
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
  int status = 0;

  printf ("Thread about to wait \n");

  pid_t w = wait (&status);

  printf ("test wait fork thread: after wait >>> %d,  status = %d signal %d \n", w,  WEXITSTATUS (status), WTERMSIG (status));

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
      pid_t w =  waitpid (-1, &status, WNOHANG);
      if (0 == w)
        {
          printf ("child not yet ended.\n");
          w = waitpid (pid, &status, 0);
        }
      printf ("father wait fork thread: after wait >>> %d, status = %d signal %d \n", w,  WEXITSTATUS (status), WTERMSIG (status));

      pthread_join (thread, 0);
    }
}

void big_fork (int prof)
{
  while (true)
    {
      pid_t pid = fork ();

      if (-1 == pid)
        {
          printf ("fork failed errno:%d, prof=%d\n", errno, prof);
          return;
        }
      if (pid)
        {
          int st = 0;

          pid_t w =  waitpid (pid, &st, 0);
          printf ("Child pid:%d %d\n", pid, w);
          if (prof > 0)
            {
              exit (0);
            }
          return;
        }
      else
        {
          if (prof++ >= 100)
            {
              exit (0);
            }
          printf ("pid:%d prof%d\n",getpid (), prof);
        }
    }
}

void big_fork_exec (int prof)
{
  while (true)
    {
      pid_t pid = fork ();

      if (-1 == pid)
        {
          printf ("fork failed errno:%d, prof=%d\n", errno, prof);
          return;
        }
      char buffer[1024];

      sprintf (buffer,"The Father have a prof of:%d !", prof);

      char *copy = strdup (buffer);

      if (pid)
        {
          int st = 0;

          printf ("Father %p %s\n", copy,copy);

          sprintf (copy,"changed string !");

          free (copy);

          pid_t w =  waitpid (pid, &st, 0);
          printf ("Child pid:%d %d\n", pid, w);
          if (prof > 0)
            {
              exit (0);
            }
          return;
        }
      else
        {
          if (prof++ >= 100)
            {
              printf ("exit \n");
              exit (0);
            }
          char arg[100];

          sprintf (arg,"%d",prof);

          static char* const args[] = { (char * const) "build/bin/test-fork", arg, 0 };

          sleep (1);

          printf ("Before exec ! %s %p \n", copy, copy);
          free (copy);

          execv ("build/bin/test-fork", args);

          return;
        }
    }
}

void
first_test (void)
{
  pid_t pid = fork ();

  if (-1 == pid)
    {
      printf ("fork failed errno:%d\n", errno);
      return;
    }
  int nfsd = 1;
  fd_set r;
  int n = 0;
  struct timeval tv;

  tv.tv_sec  = 1;
  tv.tv_usec = 0;

  FD_ZERO (&r);
  FD_SET (0, &r);

  if (pid)
    {
      printf ("Fork father\n");
      n = select (nfsd, &r, NULL, NULL, &tv);
      printf ("sel: %d\n",n);


      sleep (5);
    }
  else
    {
      printf ("Fork child\n");
      n = select (nfsd, &r, NULL, NULL, &tv);
      printf ("sel: %d\n",n);

      sleep (6);

      exit (0);

    }
}

void test_fork_simple ()
{
  int pid = fork ();
  int fd[2];

  pipe (fd);

  if (!pid)
    {
      close (fd[0]);
      exit (127);
    }
  else
    {
      close (fd[1]);
      int st;

      st = read (fd[0], &st, sizeof (st));
      printf ("read -> %d\n",st);
      st = waitpid (pid, &st, 0);
    }
}


int main (int argc, char *argv[])
{
  printf ("main argc=%d\n",argc);
  for (int i = 0; i < argc; ++i)
    {
      printf ("main argv[%d]=%s\n",i, argv[i]);
    }
  if (argc == 1)
    {
      test_fork_simple ();

      first_test ();

      test_fork ();

      wait (0);

      test_wait_fork ();

      big_fork (0);

      printf ("pid: %d after \n",getpid ());
      big_fork_exec (0);
    }
  else
    {
      big_fork_exec (argc > 1 ? (atoi (argv[1])) : 0);
    }
  return 0;
}
