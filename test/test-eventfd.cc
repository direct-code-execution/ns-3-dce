#include "test-macros.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>

static void *
writer (void *arg)
{
  int fd = *(int *)arg;
  usleep (100000);
  uint64_t v = 5;
  TEST_ASSERT_EQUAL (write (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  return 0;
}

static void
test_counter (void)
{
  int fd = eventfd (0, 0);
  TEST_ASSERT (fd >= 0);
  struct pollfd p = { fd, POLLIN | POLLOUT, 0 };
  TEST_ASSERT_EQUAL (poll (&p, 1, 0), 1);
  TEST_ASSERT_EQUAL (p.revents, POLLOUT); // empty: writable only

  uint64_t v = 3;
  TEST_ASSERT_EQUAL (write (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  v = 4;
  TEST_ASSERT_EQUAL (write (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  p.revents = 0;
  TEST_ASSERT_EQUAL (poll (&p, 1, 0), 1);
  TEST_ASSERT_EQUAL (p.revents, POLLIN | POLLOUT);

  v = 0;
  TEST_ASSERT_EQUAL (read (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  TEST_ASSERT_EQUAL (v, 7); // counter mode: the sum, then reset
  p.revents = 0;
  TEST_ASSERT_EQUAL (poll (&p, 1, 0), 1);
  TEST_ASSERT_EQUAL (p.revents, POLLOUT);

  // a short read is an error
  uint32_t small;
  TEST_ASSERT_EQUAL (read (fd, &small, sizeof (small)), -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);
  // 0xffffffffffffffff may not be written
  v = 0xffffffffffffffffULL;
  TEST_ASSERT_EQUAL (write (fd, &v, sizeof (v)), -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);
  TEST_ASSERT_EQUAL (close (fd), 0);
}

static void
test_nonblock_and_semaphore (void)
{
  int fd = eventfd (2, EFD_NONBLOCK | EFD_SEMAPHORE | EFD_CLOEXEC);
  TEST_ASSERT (fd >= 0);
  TEST_ASSERT (fcntl (fd, F_GETFL, 0) & O_NONBLOCK);
  uint64_t v = 0;
  TEST_ASSERT_EQUAL (read (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  TEST_ASSERT_EQUAL (v, 1); // semaphore mode: one unit per read
  TEST_ASSERT_EQUAL (read (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  TEST_ASSERT_EQUAL (v, 1);
  TEST_ASSERT_EQUAL (read (fd, &v, sizeof (v)), -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN); // empty and non-blocking
  TEST_ASSERT_EQUAL (eventfd_write (fd, 1), 0);
  eventfd_t ev = 0;
  TEST_ASSERT_EQUAL (eventfd_read (fd, &ev), 0);
  TEST_ASSERT_EQUAL (ev, 1);
  TEST_ASSERT_EQUAL (close (fd), 0);
}

static void
test_blocking_read (void)
{
  int fd = eventfd (0, 0);
  TEST_ASSERT (fd >= 0);
  pthread_t thread;
  TEST_ASSERT_EQUAL (pthread_create (&thread, 0, writer, &fd), 0);
  uint64_t v = 0;
  // blocks until the other thread writes, 100 ms later
  TEST_ASSERT_EQUAL (read (fd, &v, sizeof (v)), (ssize_t) sizeof (v));
  TEST_ASSERT_EQUAL (v, 5);
  TEST_ASSERT_EQUAL (pthread_join (thread, 0), 0);
  TEST_ASSERT_EQUAL (close (fd), 0);
}

int
main (int argc, char *argv[])
{
  test_counter ();
  test_nonblock_and_semaphore ();
  test_blocking_read ();
  return 0;
}
