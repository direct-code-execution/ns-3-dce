/* -*-	Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "task-scheduler.h"
#include <list>
#include "process-delay-model.h"

namespace ns3 {

class TaskScheduler;
class Fiber;
class FiberManager;

class Task
{
public:
  enum SwitchType
  {
    TO,
    FROM
  };
  bool IsActive (void) const;
  bool IsRunning (void) const;
  bool IsBlocked (void) const;
  bool IsDead (void) const;

  void SetExtraContext (void *ctx);
  void SetContext (void *ctx);

  void * GetExtraContext (void) const;
  void * GetContext (void) const;

  void SetSwitchNotifier (void (*fn)(enum SwitchType, void *), void *context);
private:
  friend class TaskManager;
  ~Task ();
  enum State
  {
    RUNNING,
    ACTIVE,
    BLOCKED,
    DEAD
  };
  enum State m_state;
  Fiber *m_fiber;
  EventId m_waitTimer;
  void *m_context;
  void *m_extraContext;
  void (*m_switchNotifier)(enum SwitchType, void *);
  void *m_switchNotifierContext;
};

class Sleeper
{
public:
  Sleeper (Task *t, Time to) : m_task (t),
                               m_timeout (to)
  {
  }
  Task * const m_task;
  const Time m_timeout;
};

class TaskManager : public Object
{
public:
  static TypeId GetTypeId (void);

  TaskManager ();
  virtual ~TaskManager ();

  void SetScheduler (Ptr<TaskScheduler> scheduler);
  void SetDelayModel (Ptr<ProcessDelayModel> model);

  /**
   * Create a task and schedule it to run later.
   * The input context is forever associated to this task and
   * can be retrieved by a call to CurrentTaskContext.
   */
  Task *Start (void (*fn)(void*), void *context);
  Task *Start (void (*fn)(void*), void *context, uint32_t stackSize);

  Task * Clone (Task *task);

  /**
   * Stop and delete a task. This task will never run again.
   * This method returns immediately.
   */
  void Stop (Task *task);

  /**
   * Schedule the input task to run later.
   * This method returns immediately.
   */
  void Wakeup (Task *task);

  /**
   * This method blocks and returns only when someone calls Wakeup on
   * this task.
   */
  void Sleep (void);
  /**
   * This method blocks and returns when someone calls Wakeup on
   * this task or timeout elapsed. Returns the amount of time
   * left until the timeout elapses.
   */
  Time Sleep (Time timeout);
  /**
   * This method blocks if other tasks can be scheduled but it
   * will return immediately if the current task is the only
   * active task.
   */
  void Yield (void);
  /**
   * This method blocks and never returns. The current task
   * is asynchronously deleted at the next available opportunity.
   */
  void Exit (void);

  /**
   * Returns currently-executing task or 0 if no tasks are currently
   * running.
   */
  Task * CurrentTask (void);
  /**
   * Returns the 'current' task manager.
   * Note that this function does not return a reference (i.e., the caller does not
   * need to call Unref on the object returned). This is done on purpose to avoid
   * problems due to a task calling Current, storing the reference on the stack,
   * calling Exit, and the destructor of the caller stack frame never being called
   * during process unloading, hence resulting in a lost reference and a leak.
   * This is also why we don't return a Ptr<TaskManager> because it would increase
   * the reference count and thus lose references when the caller's stack is ripped
   * out.
   */
  static TaskManager * Current (void);

  /**
   * Preempt the task to schedule.
   */
  void EnterHiTask (Task *task);
  void LeaveHiTask (Task *task);

  void SetSwitchNotify (void (*fn)(void));
  uint32_t GetStackSize (Task *task) const;

  /**
   * NS-3 prohibits to post event from a thread which is not the main thread,
   * ExecOnMain call a callback using the main thread
   * ScheduleMain use the Main thread to Schedule an event.
   */
  void ExecOnMain (EventImpl *e);
  EventId ScheduleMain (Time const &time, EventImpl *e);

  bool GetNoSignal ();

private:
  enum FiberManagerType
  {
    UCONTEXT_FIBER_MANAGER,
    PTHREAD_FIBER_MANAGER,
  };
  struct StartTaskContext
  {
    void (*function)(void *);
    void *context;
  };

  virtual void DoDispose (void);
  void Schedule (void);
  void SetFiberManagerType (enum FiberManagerType type);
  void GarbageCollectDeadTasks (void);
  void EndWait (Task *task);
  static void Trampoline (void *context);
  static void MainSchedule (EventId *res,Time const &time, EventImpl *e);


  Task *m_current;
  Task *m_hightask;
  Ptr<TaskScheduler> m_scheduler;
  Ptr<ProcessDelayModel> m_delayModel;
  FiberManager *m_fiberManager;
  Fiber *m_mainFiber;
  uint32_t m_defaultStackSize;
  EventId m_nextSchedule;
  bool m_reSchedule;
  Time m_reScheduleTime;
  std::list<Sleeper> m_waitQueue;
  std::list<Task *> m_deadTasks;
  EventImpl *m_todoOnMain;
  bool m_noSignal; // I am not come back from a real thread interruption do not run signal ....
  bool m_disposing; // In order to never loop while disposing me.
};

} // namespace

#endif /* TASK_MANAGER_H */
