#include "dce-signal.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <vector>
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("SimuSignal");

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

  int ret = 0;
  // NEED TO WORK!!
  // XXX: we need to add signal num notiifcation
  current->process->manager->Wait ();
  //  sigdelset (&current->pendingSignals, numnum);

  return ret;
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
