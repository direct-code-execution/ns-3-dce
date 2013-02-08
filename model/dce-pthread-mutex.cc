#include "dce-pthread.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include <stdint.h>
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("PthreadMutex");

using namespace ns3;

struct PthreadMutexAttr
{
  uint8_t type;
};

static uint32_t
MutexToMid (const pthread_mutex_t *mutex)
{
  NS_ASSERT (sizeof (pthread_mutex_t) >= 4);
  if (mutex != 0)
    {
      const uint32_t *pmid = (const uint32_t *)mutex;
      return *pmid;
    }
  return 0;
}
static void
MidToMutex (uint32_t mid, pthread_mutex_t *mutex)
{
  uint32_t *pmid = (uint32_t *)mutex;
  *pmid = mid;
}

static void
PthreadMutexInitStatic (pthread_mutex_t *mutex)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (mutex);
  // This method initializes the mutex fully when it has been
  // initialized with PTHREAD_MUTEX_INITIALIZER.
  struct Mutex *mtx = new Mutex ();
  switch (mutex->__data.__kind)
    {
    case PTHREAD_MUTEX_RECURSIVE:
      NS_LOG_DEBUG ("recursive");
      mtx->type = Mutex::RECURSIVE;
      break;
    default:
      NS_LOG_DEBUG ("normal");
      mtx->type = Mutex::NORMAL;
      break;
    }
  mtx->mid = current->process->nextMid;
  current->process->nextMid++;
  mtx->count = 0;
  mtx->waiting.clear ();
  mtx->current = 0;
  current->process->mutexes.push_back (mtx);
  MidToMutex (mtx->mid, mutex);
}

static struct Mutex *
SearchMutex (pthread_mutex_t *mutex)
{
  Thread *current = Current ();
  if (mutex == 0)
    {
      return 0;
    }
  if (MutexToMid (mutex) == 0)
    {
      // this is a mutex initialized with PTHREAD_MUTEX_INITIALIZER
      PthreadMutexInitStatic (mutex);
    }
  uint32_t mid = MutexToMid (mutex);
  for (uint32_t i = 0; i < current->process->mutexes.size (); ++i)
    {
      struct Mutex *mtx = current->process->mutexes[i];
      if (mtx->mid == mid)
        {
          return mtx;
        }
    }
  return 0;
}

int dce_pthread_mutex_init (pthread_mutex_t *mutex,
                            const pthread_mutexattr_t *attribute)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << mutex << attribute);
  NS_ASSERT (current != 0);
  if (mutex == 0)
    {
      return EINVAL;
    }
  struct PthreadMutexAttr *attr = (struct PthreadMutexAttr *)attribute;
  if (attr != 0
      && attr->type != PTHREAD_MUTEX_RECURSIVE
      && attr->type != PTHREAD_MUTEX_NORMAL)
    {
      return EINVAL;
    }
  /* Note: there is no way to detect a second attempt to initialize
   * a mutex because there is no way to detect the difference between
   * a mutex initialized correctly and a mutex un-initialized but filled
   * with random garbage which happens to look like correctly-initialized
   * data. So, we don't even try to return EBUSY.
   */
  struct Mutex *mtx = new Mutex ();
  mtx->mid = current->process->nextMid;
  current->process->nextMid++;
  if (attr == 0 || attr->type != PTHREAD_MUTEX_RECURSIVE)
    {
      mtx->type = Mutex::NORMAL;
    }
  else if (attr != 0 || attr->type == PTHREAD_MUTEX_RECURSIVE)
    {
      mtx->type = Mutex::RECURSIVE;
    }
  else
    {
      NS_ASSERT (false);
    }
  mtx->count = 0;
  mtx->waiting.clear ();
  mtx->current = 0;
  current->process->mutexes.push_back (mtx);

  MidToMutex (mtx->mid, mutex);

  return 0;
}
int dce_pthread_mutex_destroy (pthread_mutex_t *mutex)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << mutex);
  NS_ASSERT (current != 0);
  struct Mutex *mtx = SearchMutex (mutex);
  if (mtx == 0)
    {
      return EINVAL;
    }
  if (mtx->current != 0 || !mtx->waiting.empty ())
    {
      /* Someone (potentially us) is holding this mutex
       * or someone is waiting for this mutex.
       */
      return EBUSY;
    }
  // If no one is holding this mutex, its count should be zero.
  NS_ASSERT (mtx->count == 0);

  for (std::vector<struct Mutex *>::iterator i = current->process->mutexes.begin ();
       i != current->process->mutexes.end (); ++i)
    {
      if (mtx == *i)
        {
          delete mtx;
          mtx = 0;
          current->process->mutexes.erase (i);
          break;
        }
    }
  NS_ASSERT (mtx == 0);
  MidToMutex (2, mutex);

  return 0;
}
int dce_pthread_mutex_lock (pthread_mutex_t *mutex)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << mutex);
  NS_ASSERT (current != 0);
  struct Mutex *mtx = SearchMutex (mutex);
  if (mtx == 0)
    {
      return EINVAL;
    }
  if (current == mtx->current)
    {
      if (mtx->type == Mutex::RECURSIVE)
        {
          mtx->count++;
          return 0;
        }
      else if (mtx->type == Mutex::NORMAL)
        {
          return EDEADLK;
        }
      else
        {
          NS_ASSERT (false);
        }
    }
  while (mtx->current != 0)
    {
      mtx->waiting.push_back (current);
      current->process->manager->Wait ();
      mtx->waiting.remove (current);
    }
  NS_ASSERT (mtx->current == 0);
  mtx->current = current;
  mtx->count++;

  return 0;
}
int dce_pthread_mutex_trylock (pthread_mutex_t *mutex)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << mutex);
  NS_ASSERT (current != 0);
  struct Mutex *mtx = SearchMutex (mutex);
  if (mtx == 0)
    {
      return EINVAL;
    }
  if (mtx->type == Mutex::RECURSIVE)
    {
      if (mtx->current == 0)
        {
          mtx->current = current;
          mtx->count++;
          return 0;
        }
      else if (mtx->current == current)
        {
          mtx->count++;
          return 0;
        }
      else
        {
          return EBUSY;
        }
    }
  else if (mtx->type == Mutex::NORMAL)
    {
      if (mtx->current == 0)
        {
          mtx->count++;
          mtx->current = current;
          return 0;
        }
      else
        {
          // even if mtx->current == current
          // according to IEEE Std 1003.1, 2004
          return EBUSY;
        }
    }
  else
    {
      NS_ASSERT (false);
    }
  // quiet compiler.
  return 0;
}
int dce_pthread_mutex_unlock (pthread_mutex_t *mutex)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << mutex);
  NS_ASSERT (current != 0);
  struct Mutex *mtx = SearchMutex (mutex);
  if (mtx == 0)
    {
      return EINVAL;
    }
  if (mtx->current == 0
      || mtx->current != current)
    {
      return EPERM;
    }
  mtx->count--;
  if (mtx->count == 0)
    {
      mtx->current = 0;
      // Now, we tell the first waiting thread that
      // it can potentially take the lock. Note that
      // there are lots of different possible policies
      // here. We could wake up everybody and let the
      // process scheduler pick the highest priority
      // thread, we could attempt to find a better candidate
      // here locally to avoid waking up lots of threads only
      // to bring them back to sleep after they wake up because
      // they just realized that someone stole the lock from
      // them, etc. What we do, instead, is implement the simplest
      // "fair" policy by ensuring that every thread
      // is woken up in FIFO order.
      Thread *waiting = mtx->waiting.front ();
      if (waiting != 0)
        {
          current->process->manager->Wakeup (waiting);
          // give them a chance to run.
          current->process->manager->Yield ();
        }
    }
  return 0;
}
int dce_pthread_mutexattr_init (pthread_mutexattr_t *attribute)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << attribute);
  NS_ASSERT (Current () != 0);
  if (attribute == 0)
    {
      return EINVAL;
    }
  NS_ASSERT (sizeof(struct PthreadMutexAttr) <= sizeof (pthread_mutexattr_t));
  struct PthreadMutexAttr *attr = (struct PthreadMutexAttr *)attribute;
  attr->type = PTHREAD_MUTEX_NORMAL;
  return 0;
}
int dce_pthread_mutexattr_destroy (pthread_mutexattr_t *attribute)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << attribute);
  NS_ASSERT (Current () != 0);
  if (attribute == 0)
    {
      return EINVAL;
    }
  return 0;
}
int dce_pthread_mutexattr_settype (pthread_mutexattr_t *attribute, int kind)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << attribute << kind);
  NS_ASSERT (Current () != 0);
  if (attribute == 0)
    {
      return EINVAL;
    }
  struct PthreadMutexAttr *attr = (struct PthreadMutexAttr *)attribute;
  attr->type = kind;
  return 0;
}
