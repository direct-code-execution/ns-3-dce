#include "dce-semaphore.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include <stdint.h>
#include <errno.h>
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/fatal-error.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE ("DceSemaphore");

using namespace ns3;


static uint32_t AllocateSid (struct Process *process)
{
  // check that semaphore structure is big enough to store our semaphore id
  NS_ASSERT (sizeof (sem_t) > sizeof(uint16_t));
  uint32_t sid = process->nextSid;
  process->nextSid++;
  return sid;
}
static void SidToSem (uint32_t sid, sem_t *sem)
{
  uint32_t *psid = (uint32_t *)sem;
  *psid = sid;
}
static uint32_t SemToSid (const sem_t *sem)
{
  if (sem == 0)
    {
      return 0;
    }
  const uint32_t *psid = (const uint32_t *)sem;
  return *psid;
}
static struct Semaphore *
SearchSemaphore (const sem_t *sem)
{
  Thread *current = Current ();
  if (sem == 0)
    {
      return 0;
    }
  uint32_t sid = SemToSid (sem);
  for (uint32_t i = 0; i < current->process->semaphores.size (); ++i)
    {
      struct Semaphore *semaphore = current->process->semaphores[i];
      if (semaphore->sid == sid)
        {
          return semaphore;
        }
    }
  return 0;
}

int dce_sem_init (sem_t *sem, int pshared, unsigned int value)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << sem << pshared << value);
  NS_ASSERT (current != 0);

  if (pshared != 0)
    {
      current->err = ENOSYS;
      return -1;
    }
  Semaphore *semaphore = new Semaphore ();
  semaphore->sid = AllocateSid (current->process);
  semaphore->count = value;
  semaphore->waiting.clear ();
  current->process->semaphores.push_back (semaphore);
  SidToSem (semaphore->sid, sem);
  return 0;
}
int dce_sem_destroy (sem_t *sem)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sem);
  NS_ASSERT (current != 0);
  struct Semaphore *semaphore = SearchSemaphore (sem);
  if (semaphore == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  if (!semaphore->waiting.empty ())
    {
      NS_FATAL_ERROR ("Trying to destroy a semaphore on which someone else is waiting.");
    }
  for (std::vector<struct Semaphore *>::iterator i = current->process->semaphores.begin ();
       i != current->process->semaphores.end (); ++i)
    {
      if (semaphore == *i)
        {
          delete semaphore;
          semaphore = 0;
          current->process->semaphores.erase (i);
          break;
        }
    }
  NS_ASSERT (semaphore == 0);
  SidToSem (2, sem);
  return 0;
}
int dce_sem_post (sem_t *sem)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sem);
  NS_ASSERT (current != 0);

  struct Semaphore *semaphore = SearchSemaphore (sem);
  if (semaphore == 0)
    {
      current->err = EINVAL;
      return -1;
    }

  semaphore->count++;

  if (!semaphore->waiting.empty ())
    {
      // FIFO order for threads blocked on the semaphore waiting for it.
      Thread *waiting = semaphore->waiting.front ();
      current->process->manager->Wakeup (waiting);
      // give them a chance to run.
      current->process->manager->Yield ();
    }

  return 0;
}
int dce_sem_wait (sem_t *sem)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sem);
  NS_ASSERT (current != 0);

  struct Semaphore *semaphore = SearchSemaphore (sem);
  if (semaphore == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  while (semaphore->count == 0)
    {
      semaphore->waiting.push_back (current);
      current->process->manager->Wait ();
      semaphore->waiting.remove (current);
    }
  semaphore->count--;
  return 0;
}
int dce_sem_trywait (sem_t *sem)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sem);
  NS_ASSERT (current != 0);

  struct Semaphore *semaphore = SearchSemaphore (sem);
  if (semaphore == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  if (semaphore->count == 0)
    {
      current->err = EAGAIN;
      return -1;
    }
  semaphore->count--;
  return 0;
}
int dce_sem_timedwait (sem_t *sem, const struct timespec *abs_timeout)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sem);
  NS_ASSERT (current != 0);

  struct Semaphore *semaphore = SearchSemaphore (sem);
  if (semaphore == 0)
    {
      current->err = EINVAL;
      return -1;
    }

  if (semaphore->count > 0)
    {
      // fast path
      semaphore->count--;
      return 0;
    }
  Time expirationTime = UtilsTimeToSimulationTime (UtilsTimespecToTime (*abs_timeout));
  if (expirationTime <= Simulator::Now ())
    {
      // timer already expired when we start.
      current->err = ETIMEDOUT;
      return -1;
    }
  // setup timer.
  Time timeoutLeft = expirationTime - Simulator::Now ();
  while (semaphore->count == 0)
    {
      semaphore->waiting.push_back (current);
      timeoutLeft = current->process->manager->Wait (timeoutLeft);
      semaphore->waiting.remove (current);
      if (timeoutLeft.IsZero ())
        {
          // timer expired
          current->err = ETIMEDOUT;
          return -1;
        }
    }
  semaphore->count--;
  return 0;
}
int dce_sem_getvalue (sem_t *sem, int *sval)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sem);
  NS_ASSERT (current != 0);

  struct Semaphore *semaphore = SearchSemaphore (sem);
  if (semaphore == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  *sval = semaphore->count;
  return 0;
}
