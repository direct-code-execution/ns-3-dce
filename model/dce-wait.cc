#include "dce-wait.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "waiter.h"
#include "dce-stdlib.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <errno.h>
#include <vector>

NS_LOG_COMPONENT_DEFINE ("DceWait");

using namespace ns3;

pid_t dce_wait (void *status)
{
  return dce_waitpid (-1, (int*)status, 0);
}

pid_t dce_waitpid (pid_t pid, int *status, int options)
{
  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread);

  if ((options & WUNTRACED) || (options & WCONTINUED))
    {
      thread->err = EINVAL;
      return -1;
    }
  if ((pid == 0) || (pid < -1)) // Process Group not yet supported.
    {
      thread->err = EINVAL;
      return -1;
    }

  while (true)
    {
      // Seek an ended child
      int childCount = 0;

      for (std::set<uint16_t>::iterator it = thread->process->children.begin ();
           it != thread->process->children.end (); it++)
        {
          if ((*it) > 1)
            {
              Process *p = thread->process->manager->SearchProcess (*it);

              if (p)
                {
                  if ((pid == -1) || (pid == p->pid))
                    {
                      childCount++;
                      if (!p->alloc && !p->loader)  // process zombie ?
                        {
                          pid_t id = p->pid;
                          int exitCode = p->timing.exitValue;

                          std::set<uint16_t>::iterator it = thread->process->children.find (id);

                          if (it != thread->process->children.end ())
                            {
                              thread->process->children.erase (it);
                            }
                          if (status)
                            {
                              *status = exitCode;
                            }
                          thread->process->manager->FinishChild (id);

                          return id;
                        }
                    }
                }
            }
        }
      if (childCount)
        {
          if (options & WNOHANG)
            {
              return 0;
            }
          // I need wait
          Waiter *waiter = new Waiter ();

          thread->childWaiter = waiter;

          if (waiter->WaitDoSignal ())
            {
              // loop retry
              delete waiter;
              thread->childWaiter = 0;
            }
          else
            {
              delete waiter;
              thread->childWaiter = 0;
              return -1;
            }
        }
      else
        {
          thread->err = ECHILD;
          return -1;
        }
    }

  return -1;
}
