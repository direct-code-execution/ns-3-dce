#include "dce-pthread.h"
#include "dce-signal.h"
#include "dce-unistd.h"
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include "dce-cxa.h"
#include "dce-stdio.h"
#include "loader-factory.h"
#include "task-manager.h"
#include "kingsley-alloc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <errno.h>
#include <signal.h>
#include <list>

NS_LOG_COMPONENT_DEFINE ("SimuPthread");

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

static void
CleanupPthreadKeys (void)
{
  Thread *current = Current ();
  // From this function, we perform process cleanup which _requires_
  // a user context. here delete the keys of each thread which might
  // require calling a key destructor in the process.
  for (std::list<ThreadKeyValue>::iterator j = current->keyValues.begin ();
       j != current->keyValues.end (); ++j)
    {
      NS_LOG_DEBUG ("destroy key " << j->key << " " << j->destructor << " " << j->value);
      if (j->destructor != 0 && j->value != 0)
        {
          void *v = j->value;
          // according to the posix spec, we must
          // set the value to zero before invoking the
          // destructor.
          j->value = 0;
          j->destructor (v);
        }
    }
  current->keyValues.clear ();
}

void dce_exit (int status)
{
  Thread *current = Current ();
  std::ostringstream oss;
  std::string line;

  NS_LOG_FUNCTION (current << UtilsGetNodeId () << status);
  NS_ASSERT (current != 0);
  CleanupPthreadKeys ();
  dce___cxa_finalize (0);
  dce_fflush (0);
  current->process->timing.exitValue = __W_EXITCODE (status,  WTERMSIG (current->process->timing.exitValue));
  current->process->timing.ns3End = Now ().GetNanoSeconds ();
  current->process->timing.realEnd = time (0);

  current->task->SetSwitchNotifier (0, 0);
  current->process->loader->UnloadAll ();

  oss << "Exit (" << status << ")";
  line = oss.str ();
  DceManager::AppendStatusFile (current->process->pid, current->process->nodeId, line);
  DceManager::AppendProcFile (current->process);

  current->process->manager->DeleteProcess (current->process, DceManager::PEC_EXIT);
  TaskManager::Current ()->Exit ();
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

static void PthreadTaskSwitch (enum Task::SwitchType type, void *context)
{
  Process *process = (Process *) context;
  switch (type)
    {
    case Task::TO:
      process->loader->NotifyStartExecute ();
      process->alloc->SwitchTo ();
      break;
    case Task::FROM:
      process->loader->NotifyEndExecute ();
      break;
    }
}

int dce_pthread_create (pthread_t *thread_handle,
                        const pthread_attr_t *attr,
                        void *(*start_routine)(void*),
                        void *arg)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << arg);
  NS_ASSERT (current != 0);
  Thread *thread = current->process->manager->CreateThread (current->process);
  *thread_handle = PidTidToPthread (thread->process->pid, thread->tid);
  TaskManager *manager = TaskManager::Current ();
  PthreadStartContext *startContext = new PthreadStartContext ();
  startContext->start_routine = start_routine;
  startContext->arg = arg;
  uint32_t mainStackSize = manager->GetStackSize (current->process->threads[0]->task);
  Task *task = manager->Start (&pthread_do_start, startContext, mainStackSize);
  task->SetContext (thread);
  task->SetSwitchNotifier (&PthreadTaskSwitch, current->process);
  thread->task = task;
  manager->Yield ();
  return 0;
}
void dce_pthread_exit (void *arg)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << arg);
  NS_ASSERT (current != 0);
  Process *process = current->process;
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
  TaskManager::Current ()->Exit ();
}
int dce_pthread_join (pthread_t thread_handle, void **value_ptr)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << PthreadToPid (thread_handle) << PthreadToTid (thread_handle));
  NS_ASSERT (current != 0);
  NS_ASSERT (current->process->pid == PthreadToPid (thread_handle));
  if (current->tid == PthreadToTid (thread_handle))
    {
      return EDEADLK;
    }

  Thread *thread = current->process->manager->SearchThread (PthreadToPid (thread_handle),
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
int dce_pthread_detach (pthread_t thread_handle)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << PthreadToPid (thread_handle) << PthreadToTid (thread_handle));
  NS_ASSERT (current != 0);
  Thread *thread = current->process->manager->SearchThread (PthreadToPid (thread_handle),
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
pthread_t dce_pthread_self (void)
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
  Thread *current = Current ();
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
void * dce_pthread_getspecific (pthread_key_t key)
{
  Thread *current = Current ();
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
int dce_pthread_setspecific (pthread_key_t key, const void *value)
{
  Thread *current = Current ();
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
int dce_pthread_key_create (pthread_key_t *key, void (*destructor)(void*))
{
  Thread *current = Current ();
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
int dce_pthread_key_delete (pthread_key_t key)
{
  Thread *current = Current ();
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
int dce_pthread_kill (pthread_t th, int sig)
{
  Thread *current = Current ();
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
