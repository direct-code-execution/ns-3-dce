#include <stdio.h>
#include <stdlib.h>
#include "test-macros.h"
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

static void
Block (int signum)
{
  sigset_t set;
  sigemptyset (&set);
  sigaddset (&set, signum);
  TEST_ASSERT_EQUAL (sigprocmask (SIG_BLOCK, &set, 0), 0);
}

static int
Sigwait (int signum)
{
  sigset_t set;
  int sig = 0;
  sigemptyset (&set);
  sigaddset (&set, signum);
  TEST_ASSERT_EQUAL (sigwait (&set, &sig), 0);
  return sig;
}

static void
SetTimer (int sec, int usec)
{
  struct itimerval it;
  memset (&it, 0, sizeof (it));
  it.it_value.tv_sec = sec;
  it.it_value.tv_usec = usec;
  setitimer (ITIMER_REAL, &it, NULL);
}

static volatile bool g_sent = false;

static void *
Waiter (void *arg)
{
  Block (SIGUSR1);
  Block (SIGALRM);
  // Woken by a signal sent to this thread.
  TEST_ASSERT_EQUAL (Sigwait (SIGUSR1), SIGUSR1);
  // One occurrence sent to this thread and one sent to the process: two
  // signals.
  while (!g_sent)
    {
      usleep (100000);
    }
  TEST_ASSERT_EQUAL (Sigwait (SIGALRM), SIGALRM);
  TEST_ASSERT_EQUAL (Sigwait (SIGALRM), SIGALRM);
  return 0;
}

static void
TestThreadSignals (void)
{
  Block (SIGUSR1);
  Block (SIGALRM);
  pthread_t thread;
  TEST_ASSERT_EQUAL (pthread_create (&thread, 0, &Waiter, 0), 0);
  sleep (1);
  TEST_ASSERT_EQUAL (pthread_kill (thread, SIGUSR1), 0);
  SetTimer (0, 500000);
  sleep (1);
  TEST_ASSERT_EQUAL (pthread_kill (thread, SIGALRM), 0);
  g_sent = true;
  TEST_ASSERT_EQUAL (pthread_join (thread, 0), 0);
  printf ("thread signals rcvd\n");
}

int
main (int argc, char *argv[])
{
  TestThreadSignals ();

  sigset_t sigcatch;
  int signum;
  sigemptyset (&sigcatch);
  sigaddset (&sigcatch, SIGALRM);

  {
    SetTimer (1, 500000);
    while (1)
      {
        sigwait (&sigcatch, &signum);
        switch (signum)
          {
          case SIGALRM:
            printf ("SIG rcvd\n");
            return 0;
            break;
          default:
            break;
          }
        break;
      }
  }
  // never reached
  TEST_ASSERT (0);
  return 0;
}
