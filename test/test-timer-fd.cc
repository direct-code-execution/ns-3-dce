#include <sys/timerfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "test-macros.h"

static void test_file (void)
{
  int fd = open ("X", O_CREAT | O_TRUNC | O_WRONLY);
  TEST_ASSERT_UNEQUAL (fd, -1);

  struct itimerspec cur_value;
  int status = timerfd_gettime (fd, &cur_value);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_timerfd (void)
{
  int fd = timerfd_create (CLOCK_MONOTONIC, 0);
  TEST_ASSERT_UNEQUAL (fd, -1);

  uint64_t buf;
  ssize_t bytes_read = read (fd, &buf, 7);
  TEST_ASSERT_EQUAL (bytes_read, -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);

  struct itimerspec cur_value;
  int status = timerfd_gettime (fd, &cur_value);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (cur_value.it_value.tv_sec, 0);
  TEST_ASSERT_EQUAL (cur_value.it_value.tv_nsec, 0);
  TEST_ASSERT_EQUAL (cur_value.it_interval.tv_sec, 0);
  TEST_ASSERT_EQUAL (cur_value.it_interval.tv_nsec, 0);

  int flags = fcntl (fd, F_GETFL);
  TEST_ASSERT_UNEQUAL (flags, -1);
  flags |= O_NONBLOCK;
  status = fcntl (fd, F_SETFL, flags);
  TEST_ASSERT_EQUAL (status, 0);
  bytes_read = read (fd, &buf, 8);
  TEST_ASSERT_EQUAL (bytes_read, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);
  flags &= ~O_NONBLOCK;
  status = fcntl (fd, F_SETFL, flags);
  TEST_ASSERT_EQUAL (status, 0);

  struct itimerspec new_value;
  new_value.it_value.tv_sec = 1;
  new_value.it_value.tv_nsec = 500000000;
  new_value.it_interval.tv_sec = 2;
  new_value.it_interval.tv_nsec = 500000000;
  status = timerfd_settime (fd, 0, &new_value, 0);
  TEST_ASSERT_EQUAL (status, 0);

  status = timerfd_gettime (fd, &cur_value);
  TEST_ASSERT_EQUAL (status, 0);

  for (int i = 0; i < 1; i++)
    {
      uint64_t buf;
      bytes_read = read (fd, &buf, 8);
      TEST_ASSERT_EQUAL (bytes_read, 8);
      TEST_ASSERT_EQUAL (buf, 1);
    }

  sleep (6);
  bytes_read = read (fd, &buf, 8);
  TEST_ASSERT_EQUAL (bytes_read, 8);
  TEST_ASSERT_EQUAL (buf, 2);

  int copy = dup (fd);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (copy);
  TEST_ASSERT_EQUAL (status, 0);
}

static void test_otherops (void)
{
  int fd = timerfd_create (CLOCK_MONOTONIC, 0);
  TEST_ASSERT_UNEQUAL (fd, -1);

  struct itimerspec new_value;
  new_value.it_value.tv_sec = 1;
  new_value.it_value.tv_nsec = 500000000;
  new_value.it_interval.tv_sec = 2;
  new_value.it_interval.tv_nsec = 500000000;
  int status = timerfd_settime (fd, 0, &new_value, 0);
  TEST_ASSERT_EQUAL (status, 0);

  uint64_t buf;
  status = recv (fd, &buf, 8, 0);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTSOCK);
  status = send (fd, &buf, 8, 0);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTSOCK);

  void *p = mmap ((void*)0, 4096, 0, 0, fd, 0);
  TEST_ASSERT_EQUAL (p, MAP_FAILED);
  TEST_ASSERT_EQUAL (errno, EINVAL);

  status = lseek (fd, 1, SEEK_CUR);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ESPIPE);

  status = write (fd, &buf, 8);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);

  // Why this is so is beyond me but it's legal to call
  // fstat on a timer fd.
  struct stat stbuf;
  status = fstat (fd, &stbuf);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (fd);
  TEST_ASSERT_EQUAL (status, 0);
}




int main (int argc, char *argv[])
{
  test_timerfd ();
  test_otherops ();
  test_file ();

  return 0;
}
