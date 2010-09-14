#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "test-macros.h"

// test, that select () with timeout={0,0} exits immediately
static void test_select_null_null (void)
{
  // create timer to check whether a following call to select() blocks
  int timerfd = timerfd_create (CLOCK_MONOTONIC, 0);
  TEST_ASSERT_UNEQUAL (timerfd, -1);

  // arm timer to fire in 5 seconds
  struct itimerspec new_value;
  new_value.it_value.tv_sec = 5;
  new_value.it_value.tv_nsec = 0;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;
  int status = timerfd_settime (timerfd, 0, &new_value, 0);
  TEST_ASSERT_EQUAL (status, 0);

  // prepare fd_set to select() on it
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (timerfd, &fds);

  // doing select() with timeout = {0, 0}
  struct timeval timeout = {0, 0};
  int nfds = select (timerfd + 1, &fds, NULL, NULL, &timeout);
  close (timerfd);
  // no fds must be ready and select() should complete without errors
  TEST_ASSERT_EQUAL (nfds, 0);
}

int main (int argc, char *argv[])
{
  test_select_null_null();
  return 0;
}
