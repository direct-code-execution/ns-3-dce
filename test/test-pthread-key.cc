#include <pthread.h>
#include <errno.h>
#include "test-macros.h"

static pthread_key_t a;

static void * thread_fn (void *v)
{
  void *tmp = pthread_getspecific (a);
  TEST_ASSERT_EQUAL (tmp, 0);
  int *av = (int *)malloc (4);
  int status = pthread_setspecific (a, av);
  TEST_ASSERT_EQUAL (status, 0);
  tmp = pthread_getspecific (a);
  TEST_ASSERT_EQUAL (tmp, av);
  return v;
}
static void destructor (void* value)
{
  void *new_value = pthread_getspecific (a);
  TEST_ASSERT_EQUAL (new_value, 0);
  free (value);
}

int main (int argc, char *argv[])
{
  pthread_key_t b;
  int status = pthread_key_create (&b, NULL);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_key_delete (b);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_key_delete (b);
  TEST_ASSERT_EQUAL (status, EINVAL);

  status = pthread_key_create (&a, &destructor);
  TEST_ASSERT_EQUAL (status, 0);
  void *tmp = pthread_getspecific (a);
  TEST_ASSERT_EQUAL (tmp, 0);
  int *av = (int*)malloc (4);
  status = pthread_setspecific (a, av);
  TEST_ASSERT_EQUAL (status, 0);
  tmp = pthread_getspecific (a);
  TEST_ASSERT_EQUAL (tmp, av);
  pthread_t thread;
  status = pthread_create (&thread, NULL,
                           &thread_fn,
                           (void*)-5);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_setspecific (a, 0);
  TEST_ASSERT_EQUAL (status, 0);
  free (av);

  void *retval;
  status = pthread_join (thread, &retval);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (retval, (void*)-5);

  return 0;
}
