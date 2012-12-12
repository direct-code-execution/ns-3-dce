#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "test-macros.h"

static void *
thread0 (void *arg)
{
  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}
static void *
thread1 (void *arg)
{
  sleep (10);
  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}
static void *
thread2 (void *arg)
{
  int status = pthread_detach (pthread_self ());
  TEST_ASSERT_EQUAL (status, 0);
  status = sleep (30);
  TEST_ASSERT_EQUAL (status, 0);
  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}
static void *
thread3 (void *arg)
{
  int status = pthread_detach (pthread_self ());
  TEST_ASSERT_EQUAL (status, 0);
  // the result of this test is unspecified by the posix spec but
  // we have specified that multiple calls to pthread_detach are not
  // harmful.
  status = pthread_detach (pthread_self ());
  TEST_ASSERT_EQUAL (status, EINVAL);
  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}
static void *
thread4 (void *arg)
{
  sleep (1);
  // thread is joined from main thread already
  int status = pthread_detach (pthread_self ());
  TEST_ASSERT_EQUAL (status, 0);
  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}
static void *
thread5 (void *arg)
{
  sleep (1);
  // the target thread is already joined by the main thread.
  pthread_t *thread = (pthread_t *)arg;
  void *return_value;
  int status = pthread_join (*thread, &return_value);
  TEST_ASSERT_EQUAL (status, EINVAL);
  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}
static void *
thread6 (void *arg)
{
  sleep (2);
  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}


int main (int argc, char *argv[])
{
  int status;
  pthread_t thread;

  // try to join after the thread exits.
  status = pthread_create (&thread, NULL,
                           &thread0,
                           (void*)-3);
  TEST_ASSERT_EQUAL (status, 0);
  status = sleep (20);
  TEST_ASSERT_EQUAL (status, 0);
  void *return_value;
  status = pthread_join (thread, &return_value);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (return_value, (void*)-3);


  // try to join before the thread exits.
  status = pthread_create (&thread, NULL,
                           &thread1,
                           (void*)-4);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_join (thread, &return_value);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (return_value, (void*)-4);

  // ensure that we cannot join ourselves.
  status = pthread_join (pthread_self (), &return_value);
  TEST_ASSERT_EQUAL (status, EDEADLK);

  // try to join a thread in detached state.
  status = pthread_create (&thread, NULL,
                           &thread2,
                           (void*)-5);
  TEST_ASSERT_EQUAL (status, 0);
  status = sleep (20);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_join (thread, &return_value);
  TEST_ASSERT_UNEQUAL (status, 0);
  TEST_ASSERT_EQUAL (status, EINVAL);
  status = sleep (11); // make sure thread2 is dead.
  TEST_ASSERT_EQUAL (status, 0);

  // try to join a thread in detached state which is dead.
  status = pthread_create (&thread, NULL,
                           &thread3,
                           (void*)-6);
  TEST_ASSERT_EQUAL (status, 0);
  status = sleep (20);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_join (thread, NULL);
  TEST_ASSERT_UNEQUAL (status, 0);
  // in the libc implementation, the check for a valid
  // thread descriptor does not always work correctly.
  // The only case in which it works correctly is when the
  // thread has been joined and the associated thread descriptor
  // has not been yet reused for another newly created thread.
  // So, theoretically, ESRCH is the correct return value here
  // but EINVAL is what we get on nptl
  TEST_ASSERT (status == ESRCH
               || status == EINVAL);

  // try to detach a thread which is joined.
  status = pthread_create (&thread, NULL,
                           &thread4,
                           (void*)-7);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_join (thread, &return_value);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (return_value, (void*)-7);

  // try to join the same thread twice
  status = pthread_create (&thread, NULL,
                           &thread5,
                           (void*)&thread);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_create (&thread, NULL,
                           &thread6,
                           (void*)-82);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_join (thread, &return_value);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (return_value, (void*)-82);

  return 0;
}
