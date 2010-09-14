#include <pthread.h>
#include "test-macros.h"

static int g_once = 0;

static void once_fn (void)
{
  g_once++;
}
int main (int argc, char *argv[])
{
  /* This is a really very simple check. The conformance of
     the pthread_once function relies on the caller's unicity
     of the once variable which, in our codebase, is ensured with
     the ElfLoader and that is tested for elsewhere (most notably
     in test-elf-loader.cc)
   */
  pthread_once_t once = PTHREAD_ONCE_INIT;
  TEST_ASSERT_EQUAL (g_once, 0);
  int status = pthread_once (&once, &once_fn);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (g_once, 1);
  status = pthread_once (&once, &once_fn);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (g_once, 1);

  return 0;
}
