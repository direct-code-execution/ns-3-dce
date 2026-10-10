#include "dce-signal.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <vector>
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("DceSignal");

using namespace ns3;

sighandler_t dce_signal (int signum, sighandler_t handler)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << signum << handler);
  NS_ASSERT (Current () != 0);
  struct sigaction action, old_action;
  action.sa_handler = handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  int status = dce_sigaction (signum, &action, &old_action);
  if (status != 0)
    {
      return SIG_ERR;
    }
  if (old_action.sa_flags & SA_SIGINFO)
    {
      return (sighandler_t)old_action.sa_sigaction;
    }
  else
    {
      return old_action.sa_handler;
    }
}
int dce_sigaction (int signum, const struct sigaction *act,
                   struct sigaction *oldact)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << signum << act << oldact);
  NS_ASSERT (current != 0);

  for (std::vector<SignalHandler>::iterator i = current->process->signalHandlers.begin ();
       i != current->process->signalHandlers.end (); ++i)
    {
      if (i->signal == signum)
        {
          if (oldact != 0)
            {
              oldact->sa_flags = i->flags;
              oldact->sa_mask = i->mask;
              if (oldact->sa_flags & SA_SIGINFO)
                {
                  oldact->sa_sigaction = i->sigaction;
                }
              else
                {
                  oldact->sa_handler = i->handler;
                }
            }
          if (act != 0)
            {
              i->flags = act->sa_flags;
              i->mask = act->sa_mask;
              if (act->sa_flags & SA_SIGINFO)
                {
                  i->sigaction = act->sa_sigaction;
                }
              else
                {
                  i->handler = act->sa_handler;
                }
            }
          return 0;
        }
    }
  if (act != 0)
    {
      struct SignalHandler handler;
      handler.signal = signum;
      handler.flags = act->sa_flags;
      handler.mask = act->sa_mask;
      if (act->sa_flags & SA_SIGINFO)
        {
          handler.sigaction = act->sa_sigaction;
        }
      else
        {
          handler.handler = act->sa_handler;
        }
      current->process->signalHandlers.push_back (handler);
    }
  if (oldact != 0)
    {
      oldact->sa_handler = SIG_IGN;
      oldact->sa_flags = 0;
      sigemptyset (&oldact->sa_mask);
    }

  return 0;
}

int dce_sigwait (const sigset_t *set, int *sig)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << set << sig);
  NS_ASSERT (current != 0);

  while (true)
    {
      for (int s = 1; s < NSIG; s++)
        {
          if (sigismember (set, s) != 1)
            {
              continue;
            }
          if (sigismember (&current->pendingSignals, s) == 1
              || sigismember (&current->process->pendingSignals, s) == 1)
            {
              // Consume one occurrence: the thread-directed one first.
              if (sigismember (&current->pendingSignals, s) == 1)
                {
                  sigdelset (&current->pendingSignals, s);
                }
              else
                {
                  sigdelset (&current->process->pendingSignals, s);
                }
              sigemptyset (&current->sigwaitSet);
              *sig = s;
              return 0;
            }
        }
      current->sigwaitSet = *set;
      current->process->manager->Wait ();
    }
}
int dce_sigprocmask (int how, const sigset_t *set, sigset_t *oldset)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << how);
  NS_ASSERT (current != 0);

  if (0 != oldset)
    {
      *oldset = current->signalMask;
    }

  switch (how)
    {
    case SIG_BLOCK:
      {
        if (set)
          {
            sigorset (&current->signalMask, &current->signalMask, set);
          }
      } break;

    case SIG_UNBLOCK:
      {
        if (set)
          {
            for (int s = SIGINT; s <= SIGRTMAX ; s++)
              {
                if (sigismember (set, s))
                  {
                    sigdelset (&current->signalMask, s);
                  }
              }
          }
      } break;

    case SIG_SETMASK:
      {
        if (set)
          {
            current->signalMask = *set;
          }
      } break;

    default:
      {
        current->err = EINVAL;
        return -1;
      }
    }
  return 0;
}

// __sysv_signal is what glibc's signal() resolves to in strict ISO C /
// POSIX mode (no _GNU_SOURCE/_DEFAULT_SOURCE). SysV reset-to-default
// semantics are not modelled; DCE's own signal() behaviour is used.
sighandler_t dce___sysv_signal (int signum, sighandler_t handler)
{
  return dce_signal (signum, handler);
}

int dce_raise (int sig)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  return dce_kill (current->process->pid, sig);
}

// pthread_atfork() registration used by libraries (libbsd): DCE processes
// do not fork the simulator, so there is nothing to register.
int dce___register_atfork (void (*prepare) (void), void (*parent) (void), void (*child) (void), void *dso_handle)
{
  return 0;
}
