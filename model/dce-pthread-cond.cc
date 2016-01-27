#include "dce-pthread.h"
#include "process.h"
#include "utils.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("DcePthreadCond");

using namespace ns3;


static uint32_t
CondToCid (const pthread_cond_t *cond)
{
  NS_ASSERT (sizeof (pthread_cond_t) >= 4);
  if (cond != 0)
    {
      const uint32_t *pcid = (const uint32_t *)cond;
      return *pcid;
    }
  return 0;
}
static void
CidToCond (uint32_t cid, pthread_cond_t *cond)
{
  uint32_t *pcid = (uint32_t *)cond;
  *pcid = cid;
}
static struct Condition *
PthreadCondInitStatic (pthread_cond_t *cond)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (cond);
  // This method initializes the condition variable fully when it has been
  // initialized with PTHREAD_COND_INITIALIZER.
  struct Condition *condition = new Condition ();
  condition->cid = current->process->nextCid;
  current->process->nextCid++;
  current->process->conditions.push_back (condition);
  CidToCond (condition->cid, cond);
  return condition;
}

static struct Condition *
SearchCondition (pthread_cond_t *cond)
{
  Thread *current = Current ();
  if (cond == 0)
    {
      return 0;
    }
  uint32_t cid = CondToCid (cond);
  if (cid == 0)
    {
      struct Condition *condition = PthreadCondInitStatic (cond);
      return condition;
    }
  for (uint32_t i = 0; i < current->process->conditions.size (); ++i)
    {
      struct Condition *condition = current->process->conditions[i];
      if (condition->cid == cid)
        {
          return condition;
        }
    }
  return 0;
}


int dce_pthread_cond_init (pthread_cond_t *cond,
                           const pthread_condattr_t *attr)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << cond << attr);
  NS_ASSERT (Current () != 0);

  if (cond == 0)
    {
      return EINVAL;
    }
  PthreadCondInitStatic (cond);

  return 0;
}
int dce_pthread_cond_destroy (pthread_cond_t *cond)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << cond);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  struct Condition *condition = SearchCondition (cond);
  if (condition == 0)
    {
      return EINVAL;
    }

  for (std::vector<struct Condition *>::iterator i = current->process->conditions.begin ();
       i != current->process->conditions.end (); ++i)
    {
      if (condition == *i)
        {
          delete condition;
          condition = 0;
          current->process->conditions.erase (i);
          break;
        }
    }
  NS_ASSERT (condition == 0);
  CidToCond (2, cond);

  return 0;
}
int dce_pthread_cond_broadcast (pthread_cond_t *cond)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << cond);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  struct Condition *condition = SearchCondition (cond);
  if (condition == 0)
    {
      return EINVAL;
    }

  for (std::list<Thread *>::const_iterator i = condition->waiting.begin ();
       i != condition->waiting.end (); i++)
    {
      current->process->manager->Wakeup (*i);
    }
  condition->waiting.clear ();
  return 0;
}
int dce_pthread_cond_signal (pthread_cond_t *cond)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << cond);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  struct Condition *condition = SearchCondition (cond);
  if (condition == 0)
    {
      return EINVAL;
    }
  if (!condition->waiting.empty ())
    {
      Thread *thread = condition->waiting.front ();
      current->process->manager->Wakeup (thread);
      condition->waiting.pop_front ();
    }
  return 0;
}
int dce_pthread_cond_timedwait (pthread_cond_t * cond,
                                pthread_mutex_t * mutex,
                                const struct timespec * abstime)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << cond << mutex);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  struct Condition *condition = SearchCondition (cond);
  if (condition == 0)
    {
      return EINVAL;
    }
  Time timeout = UtilsTimeToSimulationTime (UtilsTimespecToTime (*abstime));
  timeout -= Simulator::Now ();
  timeout = Max (Seconds (0.0), timeout);

  dce_pthread_mutex_unlock (mutex);
  condition->waiting.push_back (current);
  Time timeLeft = current->process->manager->Wait (timeout);
  dce_pthread_mutex_lock (mutex);
  if (timeLeft.IsZero ())
    {
      return ETIMEDOUT;
    }
  return 0;
}
int dce_pthread_cond_wait (pthread_cond_t * cond,
                           pthread_mutex_t * mutex)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << cond << mutex);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  struct Condition *condition = SearchCondition (cond);
  if (condition == 0)
    {
      return EINVAL;
    }
  dce_pthread_mutex_unlock (mutex);
  condition->waiting.push_back (current);
  current->process->manager->Wait ();
  dce_pthread_mutex_lock (mutex);
  return 0;
}

// we don't implement any attribute for condition variables
int dce_pthread_condattr_init (pthread_condattr_t *attr)
{
  return 0;
}
int dce_pthread_condattr_destroy (pthread_condattr_t *attr)
{
  return 0;
}
