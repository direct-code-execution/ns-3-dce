#include <pthread.h>
#include "test-macros.h"

void test_attr_init (void)
{
  pthread_condattr_t attr;
  int status = pthread_condattr_init (&attr);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_condattr_destroy (&attr);
  TEST_ASSERT_EQUAL (status, 0);
}

void test_init (void)
{
  pthread_cond_t cond;
  int status = pthread_cond_init (&cond, 0);
  TEST_ASSERT_EQUAL (status, 0);
  status = pthread_cond_destroy (&cond);
  TEST_ASSERT_EQUAL (status, 0);
  pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
  status = pthread_cond_destroy (&cond2);
  TEST_ASSERT_EQUAL (status, 0);
}

struct BarrierContext
{
  BarrierContext ();
  int sense;
};
class Barrier
{
public:
  Barrier (int expected);
  ~Barrier ();
  void Wait (struct BarrierContext *context);
private:
  pthread_cond_t m_cond;
  pthread_mutex_t m_mutex;
  int m_expected;
  int m_reached;
  int m_release;
};

BarrierContext::BarrierContext ()
  : sense (1)
{
}

Barrier::Barrier (int expected)
  : m_expected (expected),
    m_reached (0),
    m_release (0)
{
  pthread_cond_init (&m_cond, 0);
  pthread_mutex_init (&m_mutex, 0);
}
Barrier::~Barrier ()
{
  pthread_cond_destroy (&m_cond);
  pthread_mutex_destroy (&m_mutex);
}
void
Barrier::Wait (struct BarrierContext *context)
{
  pthread_mutex_lock (&m_mutex);
  context->sense = -context->sense;
  m_reached++;
  if (m_reached == m_expected)
    {
      m_reached = 0;
      m_release = context->sense;
      pthread_cond_broadcast (&m_cond);
    }
  else
    {
      while (m_release != context->sense)
        {
          pthread_cond_wait (&m_cond, &m_mutex);
        }
    }
  pthread_mutex_unlock (&m_mutex);
}

int g_nIterations = 1000;

void * thread (void *context)
{
  Barrier *barrier = (Barrier *)context;
  BarrierContext ctx;
  for (int i = 0; i < g_nIterations; i++)
    {
      barrier->Wait (&ctx);
    }
  return 0;
}

void test_cond (void)
{
  int nThreads = 4;

  Barrier *barrier = new Barrier (nThreads);
  pthread_t * threads = new pthread_t [nThreads] ();
  for (int i = 0; i < nThreads; i++)
    {
      pthread_create (&threads[i], 0, thread, barrier);
    }
  for (int i = 0; i < nThreads; i++)
    {
      pthread_join (threads[i], 0);
    }

  delete barrier;
  delete [] threads;
}

int main (int argc, char *argv[])
{
  test_attr_init ();
  test_init ();
  test_cond ();
  return 0;
}
