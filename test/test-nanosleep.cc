#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "test-macros.h"
#include <sys/time.h>
#include <signal.h>
#include <sys/sysinfo.h>

void sigresp (int)
{
  return;
}

int main (int argc, char *argv[])
{
  // Simple nanolseep() without interruption
  {
    timespec req = { 1, 5000};
    timespec rem;

    timeval start;
    timeval end;

    gettimeofday (&start, NULL);
    int result = nanosleep (&req, &rem);
    gettimeofday (&end, NULL);

    TEST_ASSERT_EQUAL (result, 0);
    TEST_ASSERT_EQUAL (end.tv_sec - start.tv_sec, 1);
    TEST_ASSERT_EQUAL (end.tv_usec - start.tv_usec, 5);
  }

  // Simple nanolseep() without interruption: clock_gettime () version
  {
    timespec req = { 1, 5000};
    timespec rem;

    timespec start;
    timespec end;

    clock_gettime (CLOCK_REALTIME, &start);
    int result = nanosleep (&req, &rem);
    clock_gettime (CLOCK_REALTIME, &end);

    TEST_ASSERT_EQUAL (result, 0);
    TEST_ASSERT_EQUAL (end.tv_sec - start.tv_sec, 1);
    TEST_ASSERT_EQUAL (end.tv_nsec - start.tv_nsec, 5000);
  }

  // Simple nanolseep() without interruption: sysinfo (): uptime version
  {
    timespec req = { 1, 5000};
    timespec rem;

    struct sysinfo start;
    struct sysinfo end;

    sysinfo (&start);
    int result = nanosleep (&req, &rem);
    sysinfo (&end);

    TEST_ASSERT_EQUAL (result, 0);
    TEST_ASSERT_EQUAL (end.uptime - start.uptime, 1);
  }

  // Test with nanosleep() interrupted by SIGALRM fired by itimer
  {
    signal (SIGALRM, sigresp);
    struct itimerval it;
    memset (&it, 0, sizeof (it));
    it.it_value.tv_sec = 1;
    it.it_value.tv_usec = 500000;

    timespec req = { 3, 0};
    timespec rem;
    timeval end;
    timeval start;

    gettimeofday (&start, NULL);
    setitimer (ITIMER_REAL, &it, NULL);
    int result = nanosleep (&req, &rem);
    gettimeofday (&end, NULL);
    TEST_ASSERT_EQUAL (result, -1);
    TEST_ASSERT_EQUAL (end.tv_sec - start.tv_sec, 1);
    TEST_ASSERT_EQUAL (end.tv_usec - start.tv_usec, 500000);
    TEST_ASSERT_EQUAL (rem.tv_sec, 1);
    TEST_ASSERT (rem.tv_nsec = 500000000);
  }

  exit (0);
  // never reached.
  return -1;
}
