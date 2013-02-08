#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sched.h>
#include <pthread.h>

// Simulate iperf ending

static int gStop = 0;

static int gStop2 = 0;

void sigresp (int sigNum)
{
  gStop = 1;
  return;
}

void* client (void *arg)
{

  while (!gStop)
    {
      printf ("hello\n");
      sleep (1);
    }

  gStop2 = 1;

  sched_yield ();

  return arg;
}

void* reporter (void *arg)
{

  while (!gStop2)
    {
      usleep (10000);
    }

  return arg;
}

int main (int c, char **v)
{
  signal (SIGALRM, sigresp);

  struct itimerval it;
  memset (&it, 0, sizeof (it));
  it.it_value.tv_sec = 1;
  it.it_value.tv_usec = 500000;

  setitimer (ITIMER_REAL, &it, NULL);

  pthread_t ct,rt;

  int status = pthread_create (&ct, NULL,  &client, 0);

  status = pthread_create (&rt, NULL,  &reporter, 0);

  status = pthread_join (ct, NULL);
  status = pthread_join (rt, NULL);

  printf ("end of main\n");

  return 0;
}
