#include "dce-pthread.h"
#include "dce-signal.h"
#include "dce-unistd.h"
#include "dce-manager.h"
#include "dce-misc.h"
#include "process.h"
#include "utils.h"
//#include "dce-cxa.h"
#include "dce-stdio.h"
#include "dce-stdlib.h"
#include "process.h"
#include "loader-factory.h"
#include "task-manager.h"
#include "kingsley-alloc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <errno.h>
#include <signal.h>
#include <list>

NS_LOG_COMPONENT_DEFINE ("DcePthread");


using namespace ns3;

static uint16_t
PthreadToTid (pthread_t thread_handle)
{
  NS_ASSERT (sizeof (pthread_t) >= 4);
  return (thread_handle >> 16) & 0xffff;
}

static uint16_t
PthreadToPid (pthread_t thread_handle)
{
  NS_ASSERT (sizeof (pthread_t) >= 4);
  return thread_handle & 0xffff;
}

static pthread_t
PidTidToPthread (uint16_t pid, uint16_t tid)
{
  NS_ASSERT (sizeof (pthread_t) >= 4);
  uint32_t th = tid;
  th <<= 16;
  th |= pid;
  return th;
}



struct PthreadStartContext
{
  void *(*start_routine)(void*);
  void *arg;
};

static void pthread_do_start (void *context)
{
  struct PthreadStartContext *pctx = (struct PthreadStartContext *)context;
  struct PthreadStartContext ctx = *pctx;
  delete pctx;
  void *retval = ctx.start_routine (ctx.arg);
  dce_pthread_exit (retval);
}

static void PthreadTaskSwitch (enum ns3::Task::SwitchType type, void *context)
{
  ns3::Process *process = (ns3::Process *) context;
  switch (type)
    {
    case ns3::Task::TO:
      process->loader->NotifyStartExecute ();
      process->alloc->SwitchTo ();
      break;
    case ns3::Task::FROM:
      process->loader->NotifyEndExecute ();
      break;
    }
}

int dce_pthread_create (pthread_t *thread_handle,
                        const pthread_attr_t *attr,
                        void *(*start_routine)(void*),
                        void *arg) noexcept
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << ns3::UtilsGetNodeId () << arg);
  NS_ASSERT (current != 0);
  ns3::Thread *thread = current->process->manager->CreateThread (current->process);
  *thread_handle = PidTidToPthread (thread->process->pid, thread->tid);
  ns3::TaskManager *manager = TaskManager::Current ();
  PthreadStartContext *startContext = new PthreadStartContext ();
  startContext->start_routine = start_routine;
  startContext->arg = arg;
  uint32_t mainStackSize = manager->GetStackSize (current->process->threads[0]->task);
  ns3::Task *task = manager->Start (&pthread_do_start, startContext, mainStackSize);
  task->SetContext (thread);
  task->SetSwitchNotifier (&PthreadTaskSwitch, current->process);
  thread->task = task;
  manager->Yield ();
  return 0;
}
void dce_pthread_exit (void *arg)
{
  ns3::Thread *current = ns3::Current ();
  NS_LOG_FUNCTION (current << ns3::UtilsGetNodeId () << arg);
  NS_ASSERT (current != 0);
  ns3::Process *process = current->process;
  if (process->threads.size () == 1)
    {
      // call below does not return
      dce_exit (0);
    }
  CleanupPthreadKeys ();
  if (!current->isDetached)
    {
      current->hasExitValue = true;
      current->exitValue = arg;
      if (current->joinWaiter != 0)
        {
          current->process->manager->Wakeup (current->joinWaiter);
        }
      // thread will be deleted by joining thread.
      // but we clear this up to make sure that DeleteThread
      // does not try to 'Stop' the task because the call to
      // Exit below will effectively delete the task.
      current->task = 0;
    }
  else
    {
      current->process->manager->DeleteThread (current);
    }
  ns3::TaskManager::Current ()->Exit ();
}

int dce_pthread_join (pthread_t thread_handle, void **value_ptr)
{
  ns3::Thread *current = ns3::Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << PthreadToPid (thread_handle) << PthreadToTid (thread_handle));
  NS_ASSERT (current != 0);
  NS_ASSERT (current->process->pid == PthreadToPid (thread_handle));
  if (current->tid == PthreadToTid (thread_handle))
    {
      return EDEADLK;
    }

  ns3::Thread *thread = current->process->manager->SearchThread (PthreadToPid (thread_handle),
                                                            PthreadToTid (thread_handle));
  if (thread == 0)
    {
      return ESRCH;
    }
  if (thread->isDetached || thread->joinWaiter != 0)
    {
      /* If someone has already joined this thread, we do not
       * allow joining it again.
       */
      return EINVAL;
    }

  while (!thread->hasExitValue)
    {
      thread->joinWaiter = current;
      current->process->manager->Wait ();
      thread->joinWaiter = 0;
    }
  NS_ASSERT (!thread->isDetached);
  NS_ASSERT (thread->hasExitValue);
  if (value_ptr != NULL)
    {
      *value_ptr = thread->exitValue;
    }
  thread->isDetached = true;
  current->process->manager->DeleteThread (thread);
  return 0;
}
int dce_pthread_detach (pthread_t thread_handle) noexcept
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << PthreadToPid (thread_handle) << PthreadToTid (thread_handle));
  NS_ASSERT (current != 0);
  ns3::Thread *thread = current->process->manager->SearchThread (PthreadToPid (thread_handle),
                                                            PthreadToTid (thread_handle));
  if (thread == 0)
    {
      return ESRCH;
    }
  if (thread->isDetached)
    {
      return EINVAL;
    }
  if (thread->joinWaiter != 0)
    {
      // the standard does not specify what should happen in this case
      // but this is the behavior chosen by nptl so, we mimick it
      // to minimize application behavior changes.
      return 0;
    }
  thread->isDetached = true;
  return 0;
}
int dce_pthread_cancel (pthread_t thread)
{
  // XXX
  return 0;
}
pthread_t dce_pthread_self (void) noexcept
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  return PidTidToPthread (current->process->pid, current->tid);
}

int dce_pthread_once (pthread_once_t *once_control, void (*init_routine)(void))
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << once_control << init_routine);
  NS_ASSERT (Current () != 0);
  if (once_control == 0 || init_routine == 0)
    {
      return EINVAL;
    }
  if (*once_control == 1)
    {
      return 0;
    }
  *once_control = 1;
  (*init_routine)();
  return 0;
}

static bool
IsKeyValid (pthread_key_t key)
{
  ns3::Thread *current = Current ();
  for (std::list<struct ThreadKeyValue>::const_iterator i = current->keyValues.begin ();
       i != current->keyValues.end (); ++i)
    {
      if (i->key == key)
        {
          return true;
        }
    }
  return false;
}
void * dce_pthread_getspecific (pthread_key_t key) noexcept
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << key);
  NS_ASSERT (current != 0);
  for (std::list<struct ThreadKeyValue>::const_iterator i = current->keyValues.begin ();
       i != current->keyValues.end (); ++i)
    {
      if (i->key == key)
        {
          return i->value;
        }
    }
  return 0;
}
int dce_pthread_setspecific (pthread_key_t key, const void *value) noexcept
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << key << value);
  NS_ASSERT (current != 0);
  for (std::list<struct ThreadKeyValue>::iterator i = current->keyValues.begin ();
       i != current->keyValues.end (); ++i)
    {
      if (i->key == key)
        {
          i->value = const_cast<void *> (value);
          return 0;
        }
    }
  // invalid key
  return EINVAL;
}
int dce_pthread_key_create (pthread_key_t *key, void (*destructor)(void*)) noexcept
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << key << destructor);
  NS_ASSERT (current != 0);
  pthread_key_t tmp = 2;
  // this is a totally arbitrary limit on the total number of thread keys per process.
  while (tmp < 100000)
    {
      if (!IsKeyValid (tmp))
        {
          struct ThreadKeyValue value;
          value.key = tmp;
          value.destructor = destructor;
          value.value = 0;
          // store the key in each thread of the process.
          struct Process *process = current->process;
          for (std::vector<struct Thread *>::const_iterator i = process->threads.begin ();
               i != process->threads.end (); ++i)
            {
              struct Thread *thread = *i;
              thread->keyValues.push_back (value);
            }
          *key = tmp;
          return 0;
        }
      tmp++;
    }
  return EAGAIN;
}
int dce_pthread_key_delete (pthread_key_t key) noexcept
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << key);
  NS_ASSERT (current != 0);
  if (!IsKeyValid (key))
    {
      return EINVAL;
    }
  struct Process *process = current->process;
  for (std::vector<struct Thread *>::const_iterator i = process->threads.begin ();
       i != process->threads.end (); ++i)
    {
      struct Thread *thread = *i;
      bool found = false;
      for (std::list<struct ThreadKeyValue>::iterator j = thread->keyValues.begin ();
           j != thread->keyValues.end (); ++j)
        {
          if (j->key == key)
            {
              found = true;
              thread->keyValues.erase (j);
              break;
            }
        }
      NS_ASSERT (found);
    }
  return 0;
}
int dce_pthread_kill (pthread_t th, int sig) noexcept
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << PthreadToPid (th) << PthreadToTid (th) << sig);
  NS_ASSERT (current != 0);
  Thread *thread = current->process->manager->SearchThread (PthreadToPid (th),
                                                            PthreadToTid (th));
  if (thread == 0)
    {
      return ESRCH;
    }

  sigaddset (&thread->pendingSignals, sig);
  if (sigismember (&thread->signalMask, sig) == 0)
    {
      // signal not blocked by thread.
      if (thread->task->IsBlocked ())
        {
          thread->process->manager->Wakeup (thread);
        }
    }

  return 0;
}
#if 0
int dce_pthread_sigmask (int how, const sigset_t *restrict set,
                         sigset_t *restrict oset)
{
  // XXX implement
  return 0;
}
int dce_sigprocmask (int how, const sigset_t *restrict set,
                     sigset_t *restrict oset)
{
  // XXX implement
  return 0;
}
#endif
