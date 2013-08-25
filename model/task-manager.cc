#include "task-manager.h"
#include "fiber-manager.h"
#include "ucontext-fiber-manager.h"
#include "pthread-fiber-manager.h"
#include "task-scheduler.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "dce-manager.h"
#include "dce-stdio.h"
#include "process.h"
#include "utils.h"
#include "process-delay-model.h"
#include "dce-cxa.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TaskManager");
NS_OBJECT_ENSURE_REGISTERED (TaskManager);



bool
Task::IsActive (void) const
{
  return m_state == Task::ACTIVE;
}
bool
Task::IsRunning (void) const
{
  return m_state == Task::RUNNING;
}
bool
Task::IsBlocked (void) const
{
  return m_state == Task::BLOCKED;
}
bool
Task::IsDead (void) const
{
  return m_state == Task::DEAD;
}
void
Task::SetExtraContext (void *ctx)
{
  m_extraContext = ctx;
}
void
Task::SetContext (void *ctx)
{
  m_context = ctx;
}
void *
Task::GetExtraContext (void) const
{
  return m_extraContext;
}
void *
Task::GetContext (void) const
{
  return m_context;
}

void
Task::SetSwitchNotifier (void (*fn)(enum SwitchType, void *), void *context)
{
  m_switchNotifier = fn;
  m_switchNotifierContext = context;
}

Task::~Task ()
{
}


TypeId
TaskManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TaskManager")
    .SetParent<Object> ()
    .AddConstructor<TaskManager> ()
    .AddAttribute ("DefaultStackSize",
                   "The default size of the stack of every task created by this manager.",
                   UintegerValue (8192),
                   MakeUintegerAccessor (&TaskManager::m_defaultStackSize),
                   MakeUintegerChecker<uint32_t> (4096))
    .AddAttribute ("FiberManagerType",
                   "The type of FiberManager implementation to use to allocate, "
                   "deallocate and switch among fibers.",
                   TypeId::ATTR_CONSTRUCT,
                   EnumValue (PTHREAD_FIBER_MANAGER),
                   MakeEnumAccessor (&TaskManager::SetFiberManagerType),
                   MakeEnumChecker (PTHREAD_FIBER_MANAGER, "PthreadFiberManager",
                                    UCONTEXT_FIBER_MANAGER, "UcontextFiberManager"))
  ;
  return tid;
}

TaskManager::TaskManager ()
  : m_current (0),
    m_scheduler (0),
    m_fiberManager (0),
    m_reSchedule (0),
    m_disposing (0),
    m_todoOnMain (0),
    m_noSignal (0),
    m_hightask (0)
{
  NS_LOG_FUNCTION (this);
}
TaskManager::~TaskManager ()
{
  NS_LOG_FUNCTION (this);
  GarbageCollectDeadTasks ();
  m_fiberManager->Delete (m_mainFiber);
  delete m_fiberManager;

  m_mainFiber = 0;
  m_fiberManager = 0;
  m_scheduler = 0;
}

void TaskManager::DoDispose (void)
{
  if (m_disposing)
    {
      return;
    }
  m_disposing = 1;

  // Flush every FILEs in every processes.
  Ptr<DceManager> dceManager = this->GetObject<DceManager> ();

  if (0 != dceManager)
    {
      std::map<uint16_t, Process *> procs = dceManager->GetProcs ();
      std::map<uint16_t, Process *>::iterator it;

      for (it = procs.begin (); it != procs.end (); it++)
        {
          if (0 != it->second)
            {
              gDisposingThreadContext = it->second->threads.back ();

              // call atexit handler for linux kernel
              dce___cxa_finalize (0);
              if (0 != gDisposingThreadContext)
                {
                  Process *p = it->second;
                  for (std::vector<FILE *>::const_iterator i = p->openStreams.begin ();
                       i != p->openStreams.end (); ++i)
                    {
                      fflush (*i);
                    }
                }
              gDisposingThreadContext = 0;
            }
        }
    }
  Object::DoDispose ();
}

void
TaskManager::GarbageCollectDeadTasks (void)
{
  NS_LOG_FUNCTION (this);
  while (!m_deadTasks.empty ())
    {
      Task *task = m_deadTasks.front ();
      m_deadTasks.pop_front ();
      NS_LOG_DEBUG ("delete " << task);
      if (task->m_fiber)
        {
          m_fiberManager->Delete (task->m_fiber);
        }
      task->m_waitTimer.Cancel ();
      task->m_fiber = 0;
      delete task;
    }
  m_deadTasks.clear ();
}


void
TaskManager::SetScheduler (Ptr<TaskScheduler> scheduler)
{
  m_scheduler = scheduler;
}

void
TaskManager::SetDelayModel (Ptr<ProcessDelayModel> model)
{
  m_delayModel = model;
}

Task *
TaskManager::Start (void (*fn)(void*), void *context)
{
  return Start (fn, context, m_defaultStackSize);
}
// At every switch of context this method is called and it runs the signal handlers of pending signals.
static void SwitchNotifEatSignal (void)
{
  TaskManager *manager = TaskManager::Current ();
  if (manager == 0)
    {
      return;
    }
  if (!manager->CurrentTask ())
    {
      return;
    }
  Thread *current = Current ();
  if (0 == current)
    {
      return;
    }
  if (manager->GetNoSignal ())
    {
      return;
    }
  UtilsDoSignal () ;
}
Task *
TaskManager::Start (void (*fn)(void*), void *context, uint32_t stackSize)
{
  NS_LOG_FUNCTION (this << fn << context << stackSize);
  Task *task = new Task ();
  struct StartTaskContext *ctx = new StartTaskContext ();
  ctx->function = fn;
  ctx->context = context;
  task->m_fiber = m_fiberManager->Create (&TaskManager::Trampoline, ctx, stackSize);
  NS_LOG_DEBUG ("create " << task << " fiber=" << task->m_fiber);
  task->m_state = Task::BLOCKED; // must call Wakeup on task later.
  task->m_context = 0;
  task->m_extraContext = 0;
  task->m_switchNotifier = 0;
  task->m_switchNotifierContext = 0;
  Wakeup (task);
  return task;
}

Task *
TaskManager::Clone (Task *task)
{
  Task *clone = new Task ();
  clone->m_state = Task::BLOCKED; // must call Wakeup on task later.
  clone->m_context = 0;
  clone->m_extraContext = 0;
  clone->m_switchNotifier = 0;
  clone->m_switchNotifierContext = 0;
  struct Fiber *cloneFiber = m_fiberManager->Clone (task->m_fiber);
  NS_LOG_DEBUG ("clone " << clone << " fiber=" << cloneFiber);
  if (cloneFiber != 0)
    {
      // parent.
      clone->m_fiber = cloneFiber;
      Wakeup (clone);
      return clone;
    }
  return 0;
}

void
TaskManager::Trampoline (void *context)
{
  struct StartTaskContext *ctx = (struct StartTaskContext *)context;
  void (*fn)(void*) = ctx->function;
  void *fn_context = ctx->context;
  delete ctx;
  fn (fn_context);
  NS_FATAL_ERROR ("The user function must not return.");
}

void
TaskManager::Stop (Task *task)
{
  NS_LOG_FUNCTION (this << task);
  if (m_current == task)
    {
      // we ignore Stop on self.
      return;
    }

  // we can delete the task immediately.
  NS_LOG_DEBUG ("delete " << task << " fiber=" << task->m_fiber);
  m_scheduler->Dequeue (task);
  if (task->m_fiber)
    {
      m_fiberManager->Delete (task->m_fiber);
    }
  task->m_state = Task::DEAD;
  task->m_waitTimer.Cancel ();
  task->m_fiber = 0;
  delete task;
}

void
TaskManager::Wakeup (Task *task)
{
  NS_LOG_FUNCTION (this << task << task->m_state);
  if (task->m_state == Task::ACTIVE
      || task->m_state == Task::RUNNING)
    {
      return;
    }
  task->m_state = Task::ACTIVE;
  m_scheduler->Enqueue (task);
  if ((0 == m_current) && (!m_nextSchedule.IsRunning ()))
    {
      m_nextSchedule = Simulator::ScheduleNow (&TaskManager::Schedule, this);
    }
}

void
TaskManager::Sleep (void)
{
  NS_LOG_FUNCTION (this << m_current);
  NS_ASSERT (m_current != 0);
  NS_ASSERT (m_current->m_state == Task::RUNNING);
  Task *current = m_current;
  current->m_state = Task::BLOCKED;
  Schedule ();
}

Time
TaskManager::Sleep (Time timeout)
{
  NS_LOG_FUNCTION (this << m_current);
  NS_ASSERT (m_current != 0);
  NS_ASSERT (m_current->m_state == Task::RUNNING);
  Time expectedEnd = Simulator::Now () + timeout;
  Task *current = m_current;
  current->m_state = Task::BLOCKED;
  if (!timeout.IsZero ())
    {
      m_waitQueue.push_back (Sleeper (current,timeout));
    }
  Schedule ();
  current->m_waitTimer.Cancel ();
  if (!timeout.IsZero ()
      && Simulator::Now () <= expectedEnd)
    {
      return expectedEnd - Simulator::Now ();
    }
  return Seconds (0.0);
}
void
TaskManager::Yield (void)
{
  NS_LOG_FUNCTION (this << m_current);
  NS_ASSERT (m_current != 0);
  NS_ASSERT (m_current->m_state == Task::RUNNING);
  // re-queue to make sure it will be handled.
  m_current->m_state = Task::ACTIVE;
  m_scheduler->Enqueue (m_current);
  Schedule ();
}
void
TaskManager::Exit (void)
{
  NS_LOG_FUNCTION (this << m_current);
  NS_ASSERT (m_current != 0);
  NS_ASSERT (m_current->m_state == Task::RUNNING);
  Task *current = m_current;
  current->m_state = Task::DEAD;
  current->m_waitTimer.Cancel ();
  m_deadTasks.push_back (current);
  Schedule ();
}

void
TaskManager::EnterHiTask (Task *task)
{
  NS_LOG_FUNCTION (this << task << m_hightask);
  NS_ASSERT (m_hightask == NULL);
  m_hightask = task;
}
void
TaskManager::LeaveHiTask (Task *task)
{
  NS_LOG_FUNCTION (this << task << m_hightask);
  NS_ASSERT (m_hightask == task);
  m_hightask = NULL;
}

Task *
TaskManager::CurrentTask (void)
{
  if (m_hightask)
    {
      return m_hightask;
    }
  return m_current;
}
TaskManager *
TaskManager::Current (void)
{
  uint32_t nodeId = Simulator::GetContext ();
  if (nodeId == 0xffffffff)
    {
      return 0;
    }
  Ptr<Node> node = NodeList::GetNode (nodeId);
  Ptr<TaskManager> manager = node->GetObject<TaskManager> ();
  return PeekPointer (manager);
}

void
TaskManager::Schedule (void)
{
  NS_LOG_FUNCTION (this);
  if (m_current == 0)
    {
      // we have nothing to schedule from
      struct Task *next = m_scheduler->PeekNext ();
      if (next != 0)
        {
          // and now, we have something to schedule to.
          NS_LOG_DEBUG ("Leaving main, entering " << next);
          m_scheduler->DequeueNext ();
          m_current = next;
          NS_ASSERT (next->m_state == Task::ACTIVE);
          next->m_state = Task::RUNNING;
          m_delayModel->RecordStart ();
          if (next->m_switchNotifier != 0)
            {
              next->m_switchNotifier (Task::TO, next->m_switchNotifierContext);
            }
again:
          if (next->m_fiber)
            {
              m_fiberManager->SwitchTo (m_mainFiber, next->m_fiber);
            }
          if (0 != m_todoOnMain)
            {
              m_current = next;
              m_todoOnMain->Invoke ();
              delete  m_todoOnMain;
              m_todoOnMain = 0;
              goto again;
            }
          if (m_reSchedule)
            {
              NS_LOG_DEBUG ("Delayed schedule " << m_reScheduleTime);
              m_reSchedule = false;
              if (m_reScheduleTime > Time (0))
                {
                  Simulator::Schedule (m_reScheduleTime, &TaskManager::Schedule, this);
                }
              else
                {
                  Simulator::ScheduleNow (&TaskManager::Schedule, this);
                }
            }
        }
      else
        {
          // but, we have nothing to schedule to.
        }
      while (m_waitQueue.size () > 0)
        {
          Sleeper s = m_waitQueue.front ();
          s.m_task->m_waitTimer = Simulator::Schedule (s.m_timeout, &TaskManager::EndWait, this, s.m_task);
          m_waitQueue.pop_front ();
        }
      GarbageCollectDeadTasks ();
    }
  else
    {
      // we have something to schedule from.
      // but, we have nothing to schedule to so, we go back to the main task.
      Time delay = m_delayModel->RecordEnd ();
      struct Task *next = m_scheduler->PeekNext ();
      NS_LOG_DEBUG ("Leaving " << m_current << ", delay " << delay << " next = " << next << " entering main");
      if (next != 0)
        {
          // but before leaving, we check if we have further processes active, and,
          // if so, make sure we will schedule them later.
          m_reSchedule = true;
          m_reScheduleTime = delay;
        }
      struct Fiber *fiber = m_current->m_fiber;
      if (m_current->m_switchNotifier != 0)
        {
          m_current->m_switchNotifier (Task::FROM, m_current->m_switchNotifierContext);
        }
      m_current = 0;
      if (fiber)
        {
          m_fiberManager->SwitchTo (fiber, m_mainFiber);
        }
    }
}

void
TaskManager::SetFiberManagerType (enum FiberManagerType type)
{
  NS_LOG_FUNCTION (this << type);
  switch (type)
    {
    case UCONTEXT_FIBER_MANAGER:
      m_fiberManager = new UcontextFiberManager ();
      break;
    case PTHREAD_FIBER_MANAGER:
      m_fiberManager = new PthreadFiberManager ();
      break;
    default:
      NS_ASSERT (false);
      break;
    }
  m_fiberManager->SetSwitchNotification (SwitchNotifEatSignal);
  m_mainFiber = m_fiberManager->CreateFromCaller ();
}


void
TaskManager::EndWait (Task *task)
{
  if (task->m_state == Task::BLOCKED)
    {
      Wakeup (task);
    }
}

void
TaskManager::SetSwitchNotify (void (*fn)(void))
{
  m_fiberManager->SetSwitchNotification (fn);
}
uint32_t
TaskManager::GetStackSize (Task *task) const
{
  return m_fiberManager->GetStackSize (task->m_fiber);
}
void
TaskManager::ExecOnMain (EventImpl *e)
{
  if (m_current == 0)
    {
      e->Invoke ();
      delete e;
    }
  else
    {
      m_todoOnMain = e;
      struct Fiber *fiber = m_current->m_fiber;
      m_current = 0;
      m_noSignal = true;
      m_fiberManager->SwitchTo (fiber, m_mainFiber);
    }
}
EventId
TaskManager::ScheduleMain (Time const &time, EventImpl *e)
{
  EventId ret;

  ExecOnMain (MakeEvent (&TaskManager::MainSchedule, &ret, time,e));

  return ret;
}
void
TaskManager::MainSchedule (EventId *res,Time const &time, EventImpl *e)
{
  *res = Simulator::Schedule (time, e);
}
bool
TaskManager::GetNoSignal ()
{
  bool ret =  m_noSignal;
  m_noSignal = false;
  return ret;
}
} // namespace ns3
