// futex(2) through syscall(2), as GLib and Rust's std use it.
#include "test-macros.h"
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

static long
futex (uint32_t *uaddr, int op, uint32_t val, const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3)
{
  return syscall (SYS_futex, uaddr, op, val, timeout, uaddr2, val3);
}

static double
elapsed_ms (struct timespec *start)
{
  struct timespec now;
  clock_gettime (CLOCK_MONOTONIC, &now);
  return (now.tv_sec - start->tv_sec) * 1000.0 + (now.tv_nsec - start->tv_nsec) / 1e6;
}

// A futex based mutex (the classic two-state version).
static uint32_t g_lock = 0;
static int g_counter = 0;

static void
lock (void)
{
  while (__sync_val_compare_and_swap (&g_lock, 0, 1) != 0)
    {
      futex (&g_lock, FUTEX_WAIT_PRIVATE, 1, 0, 0, 0);
    }
}

static void
unlock (void)
{
  __sync_lock_release (&g_lock);
  futex (&g_lock, FUTEX_WAKE_PRIVATE, 1, 0, 0, 0);
}

static void *
worker (void *arg)
{
  for (int i = 0; i < 1000; i++)
    {
      lock ();
      g_counter++;
      if (i % 100 == 0)
        {
          usleep (1000); // hold the lock a while to force contention
        }
      unlock ();
    }
  return 0;
}

static uint32_t g_word = 0;

static void *
waker (void *arg)
{
  usleep (100000);
  g_word = 1;
  TEST_ASSERT_EQUAL (futex (&g_word, FUTEX_WAKE, 1, 0, 0, 0), 1);
  return 0;
}

int
main (int argc, char *argv[])
{
  struct timespec start;

  // value mismatch: no wait
  g_word = 0;
  TEST_ASSERT_EQUAL (futex (&g_word, FUTEX_WAIT, 1, 0, 0, 0), -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  // relative timeout
  struct timespec to = { 0, 100000000 };
  clock_gettime (CLOCK_MONOTONIC, &start);
  TEST_ASSERT_EQUAL (futex (&g_word, FUTEX_WAIT, 0, &to, 0, 0), -1);
  TEST_ASSERT_EQUAL (errno, ETIMEDOUT);
  double ms = elapsed_ms (&start);
  TEST_ASSERT (ms >= 99.0 && ms < 150.0);

  // absolute timeout (FUTEX_WAIT_BITSET, CLOCK_MONOTONIC)
  struct timespec abs;
  clock_gettime (CLOCK_MONOTONIC, &abs);
  abs.tv_nsec += 50000000;
  if (abs.tv_nsec >= 1000000000)
    {
      abs.tv_sec++;
      abs.tv_nsec -= 1000000000;
    }
  clock_gettime (CLOCK_MONOTONIC, &start);
  TEST_ASSERT_EQUAL (futex (&g_word, FUTEX_WAIT_BITSET, 0, &abs, 0, FUTEX_BITSET_MATCH_ANY), -1);
  TEST_ASSERT_EQUAL (errno, ETIMEDOUT);
  ms = elapsed_ms (&start);
  TEST_ASSERT (ms >= 49.0 && ms < 100.0);

  // nobody waiting: wake returns 0
  TEST_ASSERT_EQUAL (futex (&g_word, FUTEX_WAKE, 1, 0, 0, 0), 0);

  // woken up by another thread after 100 ms
  pthread_t thread;
  TEST_ASSERT_EQUAL (pthread_create (&thread, 0, waker, 0), 0);
  clock_gettime (CLOCK_MONOTONIC, &start);
  TEST_ASSERT_EQUAL (futex (&g_word, FUTEX_WAIT, 0, 0, 0, 0), 0);
  TEST_ASSERT_EQUAL (g_word, 1);
  ms = elapsed_ms (&start);
  TEST_ASSERT (ms >= 99.0 && ms < 150.0);
  TEST_ASSERT_EQUAL (pthread_join (thread, 0), 0);

  // a futex mutex shared by four threads
  pthread_t workers[4];
  for (int i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL (pthread_create (&workers[i], 0, worker, 0), 0);
    }
  for (int i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL (pthread_join (workers[i], 0), 0);
    }
  TEST_ASSERT_EQUAL (g_counter, 4000);

  // other system calls go through
  TEST_ASSERT (syscall (SYS_gettid) > 0);
  return 0;
}
