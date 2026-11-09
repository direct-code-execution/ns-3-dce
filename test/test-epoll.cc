#include "test-macros.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <signal.h>

static void *
writer (void *arg)
{
  int fd = *(int *)arg;
  usleep (100000);
  uint64_t v = 1;
  TEST_ASSERT_EQUAL (write (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  return 0;
}

static double
elapsed_ms (struct timespec *start)
{
  struct timespec now;
  clock_gettime (CLOCK_MONOTONIC, &now);
  return (now.tv_sec - start->tv_sec) * 1000.0 + (now.tv_nsec - start->tv_nsec) / 1e6;
}

static void
test_ctl (void)
{
  int ep = epoll_create1 (EPOLL_CLOEXEC);
  TEST_ASSERT (ep >= 0);
  int fds[2];
  TEST_ASSERT_EQUAL (pipe2 (fds, O_NONBLOCK), 0);
  TEST_ASSERT (fcntl (fds[0], F_GETFL, 0) & O_NONBLOCK);
  struct epoll_event ev;
  memset (&ev, 0, sizeof (ev));
  ev.events = EPOLLIN;
  ev.data.u32 = 42;
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_ADD, fds[0], &ev), 0);
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_ADD, fds[0], &ev), -1);
  TEST_ASSERT_EQUAL (errno, EEXIST);
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_MOD, fds[1], &ev), -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_ADD, 12345, &ev), -1);
  TEST_ASSERT_EQUAL (errno, EBADF);
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_ADD, ep, &ev), -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);

  struct epoll_event out[4];
  // nothing to read yet
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 0);
  TEST_ASSERT_EQUAL (write (fds[1], "x", 1), 1);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 1);
  TEST_ASSERT_EQUAL (out[0].events, EPOLLIN);
  TEST_ASSERT_EQUAL (out[0].data.u32, 42);
  // level triggered: still reported until drained
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 1);
  char c;
  TEST_ASSERT_EQUAL (read (fds[0], &c, 1), 1);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 0);

  // the write side is writable
  ev.events = EPOLLOUT;
  ev.data.u32 = 7;
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_ADD, fds[1], &ev), 0);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 1);
  TEST_ASSERT_EQUAL (out[0].data.u32, 7);
  TEST_ASSERT (out[0].events & EPOLLOUT);
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_DEL, fds[1], 0), 0);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 0);

  // EPOLLONESHOT: reported once, then disabled until re-armed
  ev.events = EPOLLIN | EPOLLONESHOT;
  ev.data.u32 = 42;
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_MOD, fds[0], &ev), 0);
  TEST_ASSERT_EQUAL (write (fds[1], "y", 1), 1);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 1);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 0);
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_MOD, fds[0], &ev), 0);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 1);
  TEST_ASSERT_EQUAL (read (fds[0], &c, 1), 1);

  // a closed fd disappears from the set
  TEST_ASSERT_EQUAL (close (fds[0]), 0);
  TEST_ASSERT_EQUAL (close (fds[1]), 0);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 4, 0), 0);
  TEST_ASSERT_EQUAL (close (ep), 0);
  // not an epoll fd
  int efd = eventfd (0, 0);
  TEST_ASSERT_EQUAL (epoll_wait (efd, out, 4, 0), -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);
  TEST_ASSERT_EQUAL (close (efd), 0);
}

static void
test_wait (void)
{
  int ep = epoll_create (1);
  TEST_ASSERT (ep >= 0);
  int efd = eventfd (0, 0);
  TEST_ASSERT (efd >= 0);
  struct epoll_event ev;
  memset (&ev, 0, sizeof (ev));
  ev.events = EPOLLIN;
  ev.data.fd = efd;
  TEST_ASSERT_EQUAL (epoll_ctl (ep, EPOLL_CTL_ADD, efd, &ev), 0);

  // timeout
  struct timespec start;
  clock_gettime (CLOCK_MONOTONIC, &start);
  struct epoll_event out[2];
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 2, 100), 0);
  double ms = elapsed_ms (&start);
  TEST_ASSERT (ms >= 99.0 && ms < 150.0);

  // woken up by another thread
  pthread_t thread;
  TEST_ASSERT_EQUAL (pthread_create (&thread, 0, writer, &efd), 0);
  clock_gettime (CLOCK_MONOTONIC, &start);
  TEST_ASSERT_EQUAL (epoll_wait (ep, out, 2, -1), 1);
  TEST_ASSERT_EQUAL (out[0].data.fd, efd);
  TEST_ASSERT_EQUAL (out[0].events, EPOLLIN);
  ms = elapsed_ms (&start);
  TEST_ASSERT (ms >= 99.0 && ms < 150.0);
  TEST_ASSERT_EQUAL (pthread_join (thread, 0), 0);
  uint64_t v;
  TEST_ASSERT_EQUAL (read (efd, &v, sizeof (v)), (ssize_t) sizeof (v));

  // epoll_pwait with a timeout
  sigset_t set;
  sigemptyset (&set);
  TEST_ASSERT_EQUAL (epoll_pwait (ep, out, 2, 10, &set), 0);
  TEST_ASSERT_EQUAL (close (efd), 0);
  TEST_ASSERT_EQUAL (close (ep), 0);
}

int
main (int argc, char *argv[])
{
  test_ctl ();
  test_wait ();
  return 0;
}
