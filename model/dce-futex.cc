/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "dce-futex.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <list>
#include <map>

NS_LOG_COMPONENT_DEFINE ("DceFutex");

using namespace ns3;

namespace {

struct FutexWaiter
{
  DceManager *manager;
  Thread *thread;
};

// Waiters per futex word. Addresses are unique in the simulator, whatever
// the node or process, so one table is enough.
typedef std::map<void *, std::list<FutexWaiter> > FutexTable;
static FutexTable g_futexWaiters;

// Remove the current thread from the waiters of uaddr; true if it was there
// (i.e. nobody woke it up through the futex).
static bool
RemoveWaiter (void *uaddr, Thread *thread)
{
  FutexTable::iterator i = g_futexWaiters.find (uaddr);
  if (i == g_futexWaiters.end ())
    {
      return false;
    }
  for (std::list<FutexWaiter>::iterator w = i->second.begin (); w != i->second.end (); ++w)
    {
      if (w->thread == thread)
        {
          i->second.erase (w);
          if (i->second.empty ())
            {
              g_futexWaiters.erase (i);
            }
          return true;
        }
    }
  return false;
}

static int
FutexWake (void *uaddr, int count)
{
  int woken = 0;
  FutexTable::iterator i = g_futexWaiters.find (uaddr);
  if (i == g_futexWaiters.end ())
    {
      return 0;
    }
  while (woken < count && !i->second.empty ())
    {
      FutexWaiter w = i->second.front ();
      i->second.pop_front ();
      if (w.manager->ThreadExists (w.thread))
        {
          w.manager->Wakeup (w.thread);
          woken++;
        }
    }
  if (i->second.empty ())
    {
      g_futexWaiters.erase (i);
    }
  return woken;
}

static int
FutexRequeue (void *uaddr, void *uaddr2, int count)
{
  int moved = 0;
  FutexTable::iterator i = g_futexWaiters.find (uaddr);
  if (i == g_futexWaiters.end ())
    {
      return 0;
    }
  while (moved < count && !i->second.empty ())
    {
      g_futexWaiters[uaddr2].push_back (i->second.front ());
      i->second.pop_front ();
      moved++;
    }
  if (i->second.empty ())
    {
      g_futexWaiters.erase (i);
    }
  return moved;
}

// Block the current thread on uaddr. timeout: Time (0) for none.
// Returns 0 when woken up through the futex, or an errno value.
static int
FutexWait (void *uaddr, Time timeout)
{
  Thread *current = Current ();
  FutexWaiter w;
  w.manager = current->process->manager;
  w.thread = current;
  g_futexWaiters[uaddr].push_back (w);
  Time left = current->process->manager->Wait (timeout);
  if (!RemoveWaiter (uaddr, current))
    {
      return 0; // woken up by FUTEX_WAKE
    }
  if (!timeout.IsZero () && left.IsZero ())
    {
      return ETIMEDOUT;
    }
  if (!sigisemptyset (&current->pendingSignals) || !sigisemptyset (&current->process->pendingSignals))
    {
      return EINTR;
    }
  return 0; // spurious wake up: callers re-check the futex word
}

static long
DoFutex (uint32_t *uaddr, int op, uint32_t val, const struct timespec *timeout,
         uint32_t *uaddr2, uint32_t val3)
{
  Thread *current = Current ();
  int cmd = op & FUTEX_CMD_MASK;
  switch (cmd)
    {
    case FUTEX_WAIT:
    case FUTEX_WAIT_BITSET:
      {
        if (*uaddr != val)
          {
            current->err = EAGAIN;
            return -1;
          }
        Time to = Time (0);
        if (timeout != 0)
          {
            to = UtilsTimespecToTime (*timeout);
            if (cmd == FUTEX_WAIT_BITSET)
              {
                // absolute, in the clock the application reads with clock_gettime
                Time now = UtilsSimulationTimeToTime (Simulator::Now ());
                if (to <= now)
                  {
                    current->err = ETIMEDOUT;
                    return -1;
                  }
                to = to - now;
              }
            if (to.IsZero ())
              {
                to = TimeStep (1);
              }
          }
        int err = FutexWait (uaddr, to);
        if (err != 0)
          {
            current->err = err;
            return -1;
          }
        return 0;
      }
    case FUTEX_WAKE:
    case FUTEX_WAKE_BITSET:
      return FutexWake (uaddr, (int)val);
    case FUTEX_REQUEUE:
      {
        int woken = FutexWake (uaddr, (int)val);
        FutexRequeue (uaddr, uaddr2, (int)(long)timeout);
        return woken;
      }
    case FUTEX_CMP_REQUEUE:
      {
        if (*uaddr != val3)
          {
            current->err = EAGAIN;
            return -1;
          }
        int woken = FutexWake (uaddr, (int)val);
        int moved = FutexRequeue (uaddr, uaddr2, (int)(long)timeout);
        return woken + moved;
      }
    default:
      NS_LOG_WARN ("futex operation " << cmd << " not supported");
      current->err = ENOSYS;
      return -1;
    }
}

} // anonymous namespace

long dce_syscall (long number, ...)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  va_list ap;
  va_start (ap, number);
  long a[6];
  for (int i = 0; i < 6; i++)
    {
      a[i] = va_arg (ap, long);
    }
  va_end (ap);
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << number);

  if (number == SYS_futex)
    {
      return DoFutex ((uint32_t *)a[0], (int)a[1], (uint32_t)a[2],
                      (const struct timespec *)a[3], (uint32_t *)a[4], (uint32_t)a[5]);
    }
  long r = ::syscall (number, a[0], a[1], a[2], a[3], a[4], a[5]);
  if (r == -1)
    {
      current->err = errno;
    }
  return r;
}
