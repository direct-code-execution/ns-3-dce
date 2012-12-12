#include <pthread.h>
#include "test-macros.h"
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

struct SharedData
{
  uint32_t count;
  pthread_mutex_t mutex;
};


static void * thread_fn (void *ctx)
{
  struct SharedData *data = (struct SharedData *)ctx;
  int status;
  do
    {
      status = sleep (2);
      TEST_ASSERT_EQUAL (status, 0);
      status = pthread_mutex_lock (&data->mutex);
      TEST_ASSERT_EQUAL (status, 0);
      data->count++;
      uint32_t a = data->count;
      status = sleep (2);
      TEST_ASSERT_EQUAL (status, 0);
      TEST_ASSERT_EQUAL (data->count, a);
      status = pthread_mutex_unlock (&data->mutex);
      TEST_ASSERT_EQUAL (status, 0);
    }
  while (data->count < 1000);

  return 0;
}


int main (int argc, char *argv[])
{
  int status;
  pthread_mutex_t mutex;

  // check basic initialization/destruction
  status = pthread_mutex_init (&mutex, NULL);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_destroy (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_destroy (&mutex);
  TEST_ASSERT_EQUAL (status, EINVAL);

  // check basic lock error checking
  // for normal mutexes.
  pthread_mutexattr_t attr;
  status = pthread_mutexattr_init (&attr);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_NORMAL);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_init (&mutex, &attr);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutexattr_destroy (&attr);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, EPERM);
  status = pthread_mutex_lock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_lock (&mutex);
  TEST_ASSERT_EQUAL (status, EDEADLK);
  status = pthread_mutex_trylock (&mutex);
  TEST_ASSERT_EQUAL (status, EBUSY);
  status = pthread_mutex_destroy (&mutex);
  TEST_ASSERT_EQUAL (status, EBUSY);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, EPERM);
  status = pthread_mutex_destroy (&mutex);
  TEST_ASSERT_EQUAL (status, 0);

  // check basic lock error checking
  // for recursive mutexes.
  status = pthread_mutexattr_init (&attr);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_init (&mutex, &attr);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutexattr_destroy (&attr);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, EPERM);
  status = pthread_mutex_lock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_trylock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_lock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_destroy (&mutex);
  TEST_ASSERT_EQUAL (status, EBUSY);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&mutex);
  TEST_ASSERT_EQUAL (status, EPERM);
  status = pthread_mutex_destroy (&mutex);
  TEST_ASSERT_EQUAL (status, 0);

  struct SharedData data;
  data.count = 0;
  status = pthread_mutex_init (&data.mutex, NULL);
  TEST_ASSERT_EQUAL (status, 0);
  pthread_t thread;
  status = pthread_create (&thread, NULL,
                           &thread_fn,
                           (void*)&data);
  TEST_ASSERT_EQUAL (status, 0);

  do
    {
      status = sleep (5);
      TEST_ASSERT_EQUAL (status, 0);
      status = pthread_mutex_lock (&data.mutex);
      TEST_ASSERT_EQUAL (status, 0);
      data.count++;
      uint32_t a = data.count;
      TEST_ASSERT_EQUAL (status, 0);
      TEST_ASSERT_EQUAL (data.count, a);
      status = sleep (10);
      TEST_ASSERT_EQUAL (status, 0);
      TEST_ASSERT_EQUAL (data.count, a);
      status = pthread_mutex_unlock (&data.mutex);
      TEST_ASSERT_EQUAL (status, 0);
      status = sleep (5);
      TEST_ASSERT_EQUAL (status, 0);
    }
  while (data.count < 1000);

  status = pthread_join (thread, NULL);
  TEST_ASSERT_EQUAL (status, 0);

  status = pthread_mutex_destroy (&data.mutex);
  TEST_ASSERT_EQUAL (status, 0);

  pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
  status = pthread_mutex_destroy (&a);
  TEST_ASSERT_EQUAL (status, 0);

  pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;
  status = pthread_mutex_lock (&b);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_lock (&b);
  TEST_ASSERT_EQUAL (status, EDEADLOCK);
  status = pthread_mutex_unlock (&b);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_destroy (&b);
  TEST_ASSERT_EQUAL (status, 0);

  pthread_mutex_t c = PTHREAD_MUTEX_INITIALIZER;
  status = pthread_mutex_trylock (&c);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_trylock (&c);
  TEST_ASSERT_EQUAL (status, EBUSY);
  status = pthread_mutex_unlock (&c);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_destroy (&c);
  TEST_ASSERT_EQUAL (status, 0);

  pthread_mutex_t d = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  status = pthread_mutex_trylock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_trylock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_lock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_lock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_unlock (&d);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_mutex_destroy (&d);
  TEST_ASSERT_EQUAL (status, 0);

  return 0;
}
