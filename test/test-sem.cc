#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "test-macros.h"

void * thread_a (void *ctx)
{
  sem_t *sems = (sem_t *)ctx;
  for (int i = 0; i < 100; i++)
    {
      int status = sem_post (&sems[0]);
      TEST_ASSERT_EQUAL (status, 0);
      status = sem_wait (&sems[1]);
      TEST_ASSERT_EQUAL (status, 0);
    }
  return 0;
}

void * thread_b (void *ctx)
{
  sem_t *sems = (sem_t *)ctx;
  for (int i = 0; i < 100; i++)
    {
      int status = sem_wait (&sems[0]);
      TEST_ASSERT_EQUAL (status, 0);
      status = sem_post (&sems[1]);
      TEST_ASSERT_EQUAL (status, 0);
    }
  return 0;
}

int main (int argc, char *argv[])
{
  // basic tests
  sem_t sem;
  struct timespec tm;
  int status = sem_init (&sem, 0, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_destroy (&sem);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_destroy (&sem);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);
  status = sem_init (&sem, 1, 0);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOSYS);
  status = sem_init (&sem, 0, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_post (&sem);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_wait (&sem);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_post (&sem);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_trywait (&sem);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_post (&sem);
  TEST_ASSERT_EQUAL (status, 0);
  tm.tv_sec = 1;
  tm.tv_nsec = 0;
  status = sem_timedwait (&sem, &tm);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_trywait (&sem);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);
  tm.tv_sec = 0;
  tm.tv_nsec = 0;
  status = sem_timedwait (&sem, &tm);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ETIMEDOUT);
  status = sem_destroy (&sem);
  TEST_ASSERT_EQUAL (status, 0);

  // perform ping pong between two threads.
  sem_t sems[2];
  pthread_t threads[2];
  status = sem_init (&sems[0], 0, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_init (&sems[1], 0, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_create (&threads[0], 0,
                           &thread_a, sems);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_create (&threads[1], 0,
                           &thread_b, sems);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_join (threads[0], 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_join (threads[1], 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_destroy (&sems[0]);
  TEST_ASSERT_EQUAL (status, 0);
  status = sem_destroy (&sems[1]);
  TEST_ASSERT_EQUAL (status, 0);

  return 0;
}
