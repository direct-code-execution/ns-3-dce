#include "test-macros.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

#define MAXLINE 4096

// Examples stolen from Advanced Programming in The Unix Environnement BOOK

void
err_sys (char *msg)
{
  fprintf (stderr, "%s errno:%d", msg, errno);
  OUTPUT (msg);
  TEST_ASSERT (false);
}

void
set_fl (int fd, int flags) /* flags are file status flags to turn on */
{
  int             val;

  if ((val = fcntl (fd, F_GETFL, 0)) < 0)
    {
      err_sys ((char*)"fcntl F_GETFL error");
    }

  val |= flags;           /* turn on flags */

  if (fcntl (fd, F_SETFL, val) < 0)
    {
      err_sys ((char*)"fcntl F_SETFL error");
    }
}

static int capacity = 10;

void
test1 (void)
{
  int i, n;
  int fd[2];

  if (pipe (fd) < 0)
    {
      err_sys ((char*)"pipe error");
    }
  set_fl (fd[1], O_NONBLOCK);

  /*
   * Write 1 byte at a time until pipe is full.
   */
  for (n = 0;; n++)
    {
      if ((i = write (fd[1], "a", 1)) != 1)
        {
          printf ("write ret %d, ", i);
          break;
        }
    }
  printf ("pipe capacity = %d\n", n);
  TEST_ASSERT (n > 0);
  capacity = n;
  close (fd[0]);
  close (fd[1]);
}
void
test2 ()
{
  int tot = 0;
  int n;
  int fd[2];
  pid_t pid;
  char line[MAXLINE];

  for (int i = 0; i < MAXLINE; i++)
    {
      line [i] = 'a' + (i % 26);
    }

  if (pipe (fd) < 0)
    {
      err_sys ((char*)"pipe error");
    }

  if ((pid = fork ()) < 0)
    {
      err_sys ((char*)"fork error");
    }
  else if (pid > 0)                                                               /* parent */
    {
      close (fd[0]);           /* close read end */

      for (int j = 0; j < 20; j++)
        {
          if ((n = write (fd[1], line, MAXLINE)) < 0)
            {
              err_sys ((char*)"write error to pipe");
            }
          printf ("test2 write:%d\n",n);
          tot += n;
        }

      close (fd[1]);   /* close write end of pipe for reader */

      if (waitpid (pid, NULL, 0) < 0)
        {
          err_sys ((char*)"waitpid error");
        }
      printf ("test2 father end %d.\n", tot);
    }
  else                                                                                    /* child */
    {
      close (fd[1]);   /* close write end */
      while ((n = read (fd[0], line, MAXLINE)) > 0)
        {
          printf ("test2 read:%d\n",n);
          tot += n;
        }
      close (fd[0]);
      printf ("test2 child end %d.\n", tot);
      exit (0);
    }
}

static void *
test3_thread (void *arg)
{
  int *fd = (int*)arg;
  int readFd = fd[0];
  char line[MAXLINE];
  int tot = 0;
  int n = 4;

  while ((n = read (readFd, line, MAXLINE)) > 0)
    {
      printf ("test3 read %d\n",n);
      tot += n;
    }

  printf ("test3 thread end read:%d.\n", tot);

  return arg;
}

void
test3 ()
{
  pthread_t thread;
  int fd[2];
  int status = 12;
  int tot = 0;
  int n = 3;
  char line[MAXLINE];

  for (int i = 0; i < MAXLINE; i++)
    {
      line [i] = 'a' + (i % 26);
    }
  TEST_ASSERT_EQUAL (pipe (fd), 0);

  // try to join after the thread exits.
  status = pthread_create (&thread, NULL, &test3_thread, (void*)fd);
  TEST_ASSERT_EQUAL (status, 0);

  for (int j = 0; j < 20; j++)
    {
      if ((n = write (fd[1], line, MAXLINE)) < 0)
        {
          err_sys ((char*)"write error to pipe");
        }
      printf ("test3 write %d\n",n);
      tot += n;
    }
  close (fd[1]);
  printf ("test3 end write:%d.\n", tot);
  status = pthread_join (thread, NULL);
}

void
sigpipe (int sig)
{
  printf ("get signal %d\n",sig);
}

void
test4 ()
{
  int fd[2];
  int status = 12;

  signal (SIGPIPE, sigpipe);

  TEST_ASSERT_EQUAL (pipe (fd), 0);


  char line[MAXLINE];

  for (int i = 0; i < MAXLINE; i++)
    {
      line [i] = 'a' + (i % 26);
    }
  status = write (fd[1], line, MAXLINE);

  close (fd[0]);

  status = write (fd[1], line, MAXLINE);

  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EPIPE);

  close (fd[1]);

  printf ("test4 end.\n\n");
}

static void *
test5_thread (void *arg)
{
  int *fd = (int*)arg;
  int readFd = fd[0];
  char line[MAXLINE];
  int tot = 0;
  int n = 4;

  while (true)
    {
      int nfsd = readFd + 1;
      fd_set r;

      FD_ZERO (&r);
      FD_SET (readFd, &r);

      n = select (nfsd, &r, NULL, NULL, NULL);
      TEST_ASSERT_EQUAL (n, 1);

      if ((n = read (readFd, line, MAXLINE)) > 0)
        {
          printf ("test5 read %d\n",n);
          tot += n;
        }
      else
        {
          break;
        }
    }

  printf ("test5 thread end read:%d.\n", tot);

  return arg;
}

void
test5 ()
{
  pthread_t thread;
  int fd[2];
  int status = 12;
  int tot = 0;
  int n = 3;
  char line[MAXLINE];

  for (int i = 0; i < MAXLINE; i++)
    {
      line [i] = 'a' + (i % 26);
    }
  TEST_ASSERT_EQUAL (pipe (fd), 0);

  // try to join after the thread exits.
  status = pthread_create (&thread, NULL, &test5_thread, (void*)fd);
  TEST_ASSERT_EQUAL (status, 0);

  for (int j = 0; j < 20; j++)
    {
      int nfsd = fd[1] + 1;
      fd_set w;

      FD_ZERO (&w);
      FD_SET (fd[1], &w);

      n = select (nfsd, NULL, &w, NULL, NULL);
      TEST_ASSERT_EQUAL (n, 1);

      if ((n = write (fd[1], line, MAXLINE)) < 0)
        {
          err_sys ((char*)"write error to pipe");
        }
      printf ("test5 write %d\n",n);
      tot += n;
    }
  close (fd[1]);
  printf ("test5 end write:%d.\n", tot);
  status = pthread_join (thread, NULL);
}

void
eat_stack (int fd, int pid)
{
  int tot = 0;
  int n;
  char line[MAXLINE * 100];



  for (int j = 0; j < 20; j++)
    {
      if ((n = write (fd, line, MAXLINE)) < 0)
        {
          err_sys ((char*)"write error to pipe");
        }
      printf ("test6 write:%d\n",n);
      sleep (1);
      tot += n;
    }

  close (fd);   /* close write end of pipe for reader */


  printf ("test6 father end %d.\n", tot);
}

void
test6 ()
{
  int tot = 0;
  int n;
  int fd[2];
  pid_t pid;
  char line[MAXLINE];

  for (int i = 0; i < MAXLINE; i++)
    {
      line [i] = 'a' + (i % 26);
    }

  if (pipe (fd) < 0)
    {
      err_sys ((char*)"pipe error");
    }

  if ((pid = fork ()) < 0)
    {
      err_sys ((char*)"fork error");
    }
  else if (pid <= 0)
    {
      close (fd[0]);           /* close read end */
      eat_stack (fd[1], pid);

      for (int i = 0; i < 10; i++)
        {
          sleep (1);
        }
      printf ("Hello\n");
      exit (0);

    }
  else
    {
      /* child */
      close (fd[1]);   /* close write end */
      while ((n = read (fd[0], line, MAXLINE)) > 0)
        {
          printf ("test6 read:%d\n",n);
          sleep (1);
          tot += n;
        }
      close (fd[0]);
      printf ("test6 child end %d.\n", tot);

      for (int i = 0; i < 10; i++)
        {
          sleep (1);
        }

      if (waitpid (pid, NULL, 0) < 0)
        {
          err_sys ((char*)"waitpid error");
        }
    }
}


int
main (int c, char **v)
{
  test1 ();
  test2 ();
  test3 ();
  test4 ();
  test5 ();
  test6 ();

  return 0;
}
