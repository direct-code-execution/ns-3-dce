/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "lkl-kernel.h"
#include "task-manager.h"
#include "utils.h"
#include "exec-utils.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/event-impl.h"
#include "ns3/make-event.h"
#include <lkl/asm/host_ops.h>
#include <list>
#include <map>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <csetjmp>
#include <sys/mman.h>
#include <dlfcn.h>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

NS_LOG_COMPONENT_DEFINE ("DceLklKernel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LklKernel);

namespace {

// Stack size of the tasks running kernel threads.
const uint32_t LKL_TASK_STACK = 1 << 18;

LklKernel *
Kernel (void)
{
  LklKernel *kernel = LklKernel::Current ();
  NS_ASSERT_MSG (kernel, "LKL host operation called outside of a node with an LKL kernel");
  return kernel;
}

TaskManager *
Manager (void)
{
  return TaskManager::Current ();
}

/* Threads ---------------------------------------------------------------- */

struct LklThread
{
  Task *task;
  void (*fn)(void *);
  void *arg;
  bool done;
  bool detached;
  std::list<Task *> joiners;
};

// Tasks of all nodes are distinct, so a single map is enough.
std::map<Task *, LklThread *> g_threads;

LklThread *
ThreadOf (Task *task)
{
  std::map<Task *, LklThread *>::iterator i = g_threads.find (task);
  if (i != g_threads.end ())
    {
      return i->second;
    }
  // A task that was not created by the kernel (an application or a timer
  // task) calls into the kernel: give it an identity.
  LklThread *thread = new LklThread ();
  thread->task = task;
  thread->fn = 0;
  thread->arg = 0;
  thread->done = false;
  thread->detached = true;
  g_threads[task] = thread;
  return thread;
}

void
ThreadFinish (void)
{
  Task *task = Manager ()->RunningTask ();
  LklThread *thread = ThreadOf (task);
  thread->done = true;
  for (std::list<Task *>::iterator i = thread->joiners.begin (); i != thread->joiners.end (); ++i)
    {
      Manager ()->Wakeup (*i);
    }
  thread->joiners.clear ();
  g_threads.erase (task);
  if (thread->detached)
    {
      delete thread;
    }
  else
    {
      thread->task = 0;
    }
  Manager ()->Exit ();
}

void
ThreadTrampoline (void *context)
{
  LklThread *thread = (LklThread *)context;
  thread->fn (thread->arg);
  ThreadFinish ();
}

lkl_thread_t
ThreadCreate (void (*fn)(void *), void *arg)
{
  LklKernel *kernel = Kernel ();
  LklThread *thread = new LklThread ();
  thread->fn = fn;
  thread->arg = arg;
  thread->done = false;
  thread->detached = false;
  thread->task = kernel->GetTaskManager ()->Start (&ThreadTrampoline, thread, LKL_TASK_STACK);
  g_threads[thread->task] = thread;
  return (lkl_thread_t)thread;
}

void
ThreadDetach (void)
{
  ThreadOf (Manager ()->RunningTask ())->detached = true;
}

void
ThreadExit (void)
{
  ThreadFinish ();
}

int
ThreadJoin (lkl_thread_t tid)
{
  LklThread *thread = (LklThread *)tid;
  while (!thread->done)
    {
      thread->joiners.push_back (Manager ()->RunningTask ());
      Manager ()->Sleep ();
    }
  delete thread;
  return 0;
}

lkl_thread_t
ThreadSelf (void)
{
  return (lkl_thread_t)ThreadOf (Manager ()->RunningTask ());
}

int
ThreadEqual (lkl_thread_t a, lkl_thread_t b)
{
  return a == b;
}

/* Semaphores and mutexes -------------------------------------------------- */

struct LklWaitList
{
  std::list<Task *> waiters;
  void Wait (void)
  {
    waiters.push_back (Manager ()->RunningTask ());
    Manager ()->Sleep ();
  }
  void WakeOne (void)
  {
    if (!waiters.empty ())
      {
        Task *task = waiters.front ();
        waiters.pop_front ();
        Manager ()->Wakeup (task);
      }
  }
};

struct LklSem
{
  int count;
  LklWaitList wait;
};

struct lkl_sem *
SemAlloc (int count)
{
  LklSem *sem = new LklSem ();
  sem->count = count;
  return (struct lkl_sem *)sem;
}

void
SemFree (struct lkl_sem *s)
{
  delete (LklSem *)s;
}

void
SemUp (struct lkl_sem *s)
{
  LklSem *sem = (LklSem *)s;
  sem->count++;
  sem->wait.WakeOne ();
}

void
SemDown (struct lkl_sem *s)
{
  LklSem *sem = (LklSem *)s;
  while (sem->count <= 0)
    {
      sem->wait.Wait ();
    }
  sem->count--;
}

struct LklMutex
{
  bool recursive;
  Task *owner;
  int depth;
  LklWaitList wait;
};

struct lkl_mutex *
MutexAlloc (int recursive)
{
  LklMutex *mutex = new LklMutex ();
  mutex->recursive = recursive;
  mutex->owner = 0;
  mutex->depth = 0;
  return (struct lkl_mutex *)mutex;
}

void
MutexFree (struct lkl_mutex *m)
{
  delete (LklMutex *)m;
}

void
MutexLock (struct lkl_mutex *m)
{
  LklMutex *mutex = (LklMutex *)m;
  Task *self = Manager ()->RunningTask ();
  if (mutex->recursive && mutex->owner == self)
    {
      mutex->depth++;
      return;
    }
  while (mutex->owner != 0)
    {
      mutex->wait.Wait ();
    }
  mutex->owner = self;
  mutex->depth = 1;
}

void
MutexUnlock (struct lkl_mutex *m)
{
  LklMutex *mutex = (LklMutex *)m;
  if (--mutex->depth > 0)
    {
      return;
    }
  mutex->owner = 0;
  mutex->wait.WakeOne ();
}

/* Thread-local storage ---------------------------------------------------- */

struct LklTlsKey
{
  void (*destructor)(void *);
  std::map<Task *, void *> values;
};

struct lkl_tls_key *
TlsAlloc (void (*destructor)(void *))
{
  LklTlsKey *key = new LklTlsKey ();
  key->destructor = destructor;
  Kernel ()->GetTlsKeys ().insert (key);
  return (struct lkl_tls_key *)key;
}

void
TlsFree (struct lkl_tls_key *k)
{
  Kernel ()->GetTlsKeys ().erase (k);
  delete (LklTlsKey *)k;
}

int
TlsSet (struct lkl_tls_key *k, void *data)
{
  ((LklTlsKey *)k)->values[Manager ()->RunningTask ()] = data;
  return 0;
}

void *
TlsGet (struct lkl_tls_key *k)
{
  LklTlsKey *key = (LklTlsKey *)k;
  std::map<Task *, void *>::iterator i = key->values.find (Manager ()->RunningTask ());
  return i == key->values.end () ? 0 : i->second;
}

/* Time and timers --------------------------------------------------------- */

unsigned long long
Time_ (void)
{
  return Simulator::Now ().GetNanoSeconds ();
}

struct LklTimer
{
  void (*fn)(void);
  LklKernel *kernel;
  EventId event;
};

void
TimerTaskTrampoline (void *context)
{
  LklTimer *timer = (LklTimer *)context;
  timer->fn ();
  Manager ()->Exit ();
}

// Runs the timer callback in its own task: it raises the timer interrupt,
// and kernel code reached from there may block on the CPU lock.
void
TimerExpire (LklTimer *timer)
{
  timer->kernel->GetTaskManager ()->Start (&TimerTaskTrampoline, timer, LKL_TASK_STACK);
}

void *
TimerAlloc (void (*fn)(void))
{
  LklTimer *timer = new LklTimer ();
  timer->fn = fn;
  timer->kernel = Kernel ();
  return timer;
}

int
TimerSetOneshot (void *t, unsigned long delta)
{
  LklTimer *timer = (LklTimer *)t;
  timer->event.Cancel ();
  // Called from the node's own tasks, which may run on threads of their own
  // (PthreadFiberManager): schedule from the simulator's thread. The event
  // keeps the node context.
  timer->event = Manager ()->ScheduleMain (NanoSeconds (delta), MakeEvent (&TimerExpire, timer));
  return 0;
}

void
TimerFree (void *t)
{
  LklTimer *timer = (LklTimer *)t;
  timer->event.Cancel ();
  delete timer;
}

/* Memory and misc --------------------------------------------------------- */

void
Print (const char *str, int len)
{
  fwrite (str, 1, len, stdout);
}

void
Panic (void)
{
  NS_FATAL_ERROR ("LKL kernel panic on node " << Kernel ()->GetNodeId ());
}

void *
MemAlloc (unsigned long size)
{
  return malloc (size);
}

void
MemFree (void *p)
{
  free (p);
}

void *
PageAlloc (unsigned long size)
{
  // Anonymous mappings are committed lazily: idle kernel memory costs nothing.
  void *p = mmap (0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED)
    {
      return 0;
    }
  Kernel ()->GetPages ()[p] = size;
  return p;
}

void
PageFree (void *addr, unsigned long size)
{
  Kernel ()->GetPages ().erase (addr);
  munmap (addr, size);
}

void
JmpBufSet (struct lkl_jmp_buf *jmpb, void (*f)(void))
{
  static_assert (sizeof (jmpb->buf) >= sizeof (jmp_buf), "lkl_jmp_buf too small");
  if (!setjmp (*((jmp_buf *)jmpb->buf)))
    {
      f ();
    }
}

void
JmpBufLongjmp (struct lkl_jmp_buf *jmpb, int val)
{
  longjmp (*((jmp_buf *)jmpb->buf), val);
}

void *
Memcpy (void *dest, const void *src, unsigned long count)
{
  return memcpy (dest, src, count);
}

void *
Memset (void *s, int c, unsigned long count)
{
  return memset (s, c, count);
}

void *
Memmove (void *dest, const void *src, unsigned long count)
{
  return memmove (dest, src, count);
}

/* Files ------------------------------------------------------------------ */

void
CreateDirectory (const std::string &path)
{
  for (std::string::size_type i = path.find ('/'); ; i = path.find ('/', i + 1))
    {
      mkdir (path.substr (0, i).c_str (), S_IRWXU);
      if (i == std::string::npos)
        {
          break;
        }
    }
}

// Copy unless an identical-looking copy (same size, not older) is there.
void
CopyFile (const std::string &from, const std::string &to)
{
  struct stat src, dst;
  NS_ASSERT_MSG (stat (from.c_str (), &src) == 0, "Cannot stat " << from);
  if (stat (to.c_str (), &dst) == 0 && dst.st_size == src.st_size && dst.st_mtime >= src.st_mtime)
    {
      return;
    }
  std::ifstream in (from, std::ios::binary);
  std::ofstream out (to, std::ios::binary | std::ios::trunc);
  out << in.rdbuf ();
  NS_ASSERT_MSG (in && out, "Cannot copy " << from << " to " << to);
}

} // namespace

TypeId
LklKernel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LklKernel")
    .SetParent<Object> ()
    .AddConstructor<LklKernel> ()
    .AddAttribute ("Library", "The LKL shared library to load.",
                   StringValue ("liblkl.so"),
                   MakeStringAccessor (&LklKernel::m_library),
                   MakeStringChecker ())
    .AddAttribute ("CommandLine", "The kernel command line.",
                   StringValue ("mem=64M loglevel=4"),
                   MakeStringAccessor (&LklKernel::m_cmdline),
                   MakeStringChecker ())
  ;
  return tid;
}

LklKernel::LklKernel ()
  : m_manager (0),
    m_handle (0),
    m_init (0),
    m_start (0),
    m_syscall (0),
    m_booted (false)
{
}

LklKernel::~LklKernel ()
{
  // The simulation is over: nothing runs the kernel any more.
  for (std::map<void *, unsigned long>::iterator i = m_pages.begin (); i != m_pages.end (); ++i)
    {
      munmap (i->first, i->second);
    }
  if (m_handle)
    {
      dlclose (m_handle);
    }
}

std::map<void *, unsigned long> &
LklKernel::GetPages (void)
{
  return m_pages;
}

void
LklKernel::DoDispose (void)
{
  m_manager = 0;
  Object::DoDispose ();
}

void
LklKernel::NotifyNewAggregate (void)
{
  Ptr<TaskManager> taskManager = GetObject<TaskManager> ();
  if (taskManager)
    {
      m_manager = PeekPointer (taskManager);
    }
  Object::NotifyNewAggregate ();
}

LklKernel *
LklKernel::Current (void)
{
  TaskManager *manager = TaskManager::Current ();
  if (!manager)
    {
      return 0;
    }
  return PeekPointer (manager->GetObject<LklKernel> ());
}

TaskManager *
LklKernel::GetTaskManager (void) const
{
  return m_manager;
}

uint32_t
LklKernel::GetNodeId (void) const
{
  return GetObject<Node> ()->GetId ();
}

void
LklKernel::InstallHostOps (struct lkl_host_operations *ops)
{
  // The library's own host code (virtio, network devices) calls this table
  // directly, so replace every POSIX operation with a DCE one.
  ops->print = &Print;
  ops->panic = &Panic;
  ops->sem_alloc = &SemAlloc;
  ops->sem_free = &SemFree;
  ops->sem_up = &SemUp;
  ops->sem_down = &SemDown;
  ops->mutex_alloc = &MutexAlloc;
  ops->mutex_free = &MutexFree;
  ops->mutex_lock = &MutexLock;
  ops->mutex_unlock = &MutexUnlock;
  ops->thread_create = &ThreadCreate;
  ops->thread_detach = &ThreadDetach;
  ops->thread_exit = &ThreadExit;
  ops->thread_join = &ThreadJoin;
  ops->thread_self = &ThreadSelf;
  ops->thread_equal = &ThreadEqual;
  ops->thread_stack = 0;
  ops->tls_alloc = &TlsAlloc;
  ops->tls_free = &TlsFree;
  ops->tls_set = &TlsSet;
  ops->tls_get = &TlsGet;
  ops->mem_alloc = &MemAlloc;
  ops->mem_free = &MemFree;
  ops->page_alloc = &PageAlloc;
  ops->page_free = &PageFree;
  ops->time = &Time_;
  ops->timer_alloc = &TimerAlloc;
  ops->timer_set_oneshot = &TimerSetOneshot;
  ops->timer_free = &TimerFree;
  ops->ioremap = (void * (*)(long, int)) dlsym (m_handle, "lkl_ioremap");
  ops->iomem_access = (int (*)(const volatile void *, void *, int, int))
    dlsym (m_handle, "lkl_iomem_access");
  ops->jmp_buf_set = &JmpBufSet;
  ops->jmp_buf_longjmp = &JmpBufLongjmp;
  ops->memcpy = &Memcpy;
  ops->memset = &Memset;
  ops->memmove = &Memmove;
  ops->mmap = 0;
  ops->munmap = 0;
  ops->shmem_init = 0;
  ops->shmem_mmap = 0;
  ops->pci_ops = 0;
}

void
LklKernel::Load (void)
{
  std::string path = SearchExecFile ("DCE_PATH", m_library, 0);
  NS_ASSERT_MSG (path.length () > 0, "LKL library '" << m_library
                 << "' not found; check the DCE_PATH environment variable.");
  // Each node needs its own instance of the kernel, with its own global
  // data. The dynamic linker only loads a file once, so give each kernel
  // its own copy of the library; a node of a later simulation in the same
  // process (as in test suites) gets a new one.
  static std::map<uint32_t, uint32_t> loads;
  uint32_t generation = loads[GetNodeId ()]++;
  std::ostringstream dir;
  dir << "elf-cache/lkl/" << GetNodeId ();
  if (generation > 0)
    {
      dir << "-" << generation;
    }
  std::string copy = dir.str () + "/" + m_library;
  CreateDirectory (dir.str ());
  CopyFile (path, copy);
  m_handle = dlopen (copy.c_str (), RTLD_NOW | RTLD_LOCAL);
  NS_ASSERT_MSG (m_handle, "Cannot load " << copy << ": " << dlerror ());
  m_init = (int (*)(struct lkl_host_operations *)) dlsym (m_handle, "lkl_init");
  m_start = (int (*)(const char *, ...)) dlsym (m_handle, "lkl_start_kernel");
  m_syscall = (long (*)(long, long *)) dlsym (m_handle, "lkl_syscall");
  NS_ASSERT_MSG (m_init && m_start && m_syscall, "Missing LKL entry points in " << path);
}

std::set<void *> &
LklKernel::GetTlsKeys (void)
{
  return m_tlsKeys;
}

namespace {
struct TlsDestructors
{
  std::list<std::pair<void (*)(void *), void *> > calls;
};
} // namespace

void
LklKernel::TlsDestructorsTrampoline (void *context)
{
  TlsDestructors *destructors = (TlsDestructors *)context;
  for (std::list<std::pair<void (*)(void *), void *> >::iterator i = destructors->calls.begin ();
       i != destructors->calls.end (); ++i)
    {
      i->first (i->second);
    }
  delete destructors;
  TaskManager::Current ()->Exit ();
}

// Like pthread thread-local storage: drop the task's values and, when the
// task exited, call their destructors. They run in a task of their own:
// the exiting task may belong to a process DCE already deleted.
void
LklKernel::TaskEnd (Task *task, bool running, void *context)
{
  LklKernel *kernel = (LklKernel *)context;
  TlsDestructors *destructors = new TlsDestructors ();
  for (std::set<void *>::iterator k = kernel->m_tlsKeys.begin (); k != kernel->m_tlsKeys.end (); ++k)
    {
      LklTlsKey *key = (LklTlsKey *)*k;
      std::map<Task *, void *>::iterator i = key->values.find (task);
      if (i == key->values.end ())
        {
          continue;
        }
      if (running && i->second && key->destructor)
        {
          destructors->calls.push_back (std::make_pair (key->destructor, i->second));
        }
      key->values.erase (i);
    }
  if (destructors->calls.empty ())
    {
      delete destructors;
      return;
    }
  kernel->m_manager->Start (&LklKernel::TlsDestructorsTrampoline, destructors, LKL_TASK_STACK);
}

void
LklKernel::Boot (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!m_booted);
  NS_ASSERT_MSG (TaskManager::Current () == m_manager && m_manager->CurrentTask (),
                 "LklKernel::Boot must run in a task of its node");
  Load ();
  struct lkl_host_operations *ops =
    (struct lkl_host_operations *) dlsym (m_handle, "lkl_host_ops");
  NS_ASSERT_MSG (ops, "Missing lkl_host_ops in " << m_library);
  InstallHostOps (ops);
  m_manager->SetTaskEndNotifier (&LklKernel::TaskEnd, this);
  int ret = m_init (ops);
  NS_ASSERT_MSG (ret == 0, "lkl_init failed: " << ret);
  ret = m_start (m_cmdline.c_str ());
  NS_ASSERT_MSG (ret == 0, "lkl_start_kernel failed: " << ret);
  m_booted = true;
}

bool
LklKernel::IsBooted (void) const
{
  return m_booted;
}

void *
LklKernel::Lookup (const char *symbol) const
{
  NS_ASSERT (m_handle);
  return dlsym (m_handle, symbol);
}

namespace {
struct MainSyscall
{
  LklKernel *kernel;
  long no;
  long *params;
  long ret;
  bool done;
};
} // namespace

void
LklKernel::MainSyscallTrampoline (void *context)
{
  MainSyscall *call = (MainSyscall *)context;
  call->ret = call->kernel->m_syscall (call->no, call->params);
  call->done = true;
  TaskManager::Current ()->Exit ();
}

long
LklKernel::Syscall (long no, long *params)
{
  NS_ASSERT (m_booted);
  if (m_manager->RunningTask ())
    {
      return m_syscall (no, params);
    }
  // From the main fiber, e.g. ns-3 sockets (LinuxSocketImpl): the call may
  // wait for kernel threads, so it runs in a task, now.
  NS_ASSERT (TaskManager::Current () == m_manager);
  MainSyscall call = { this, no, params, 0, false };
  m_manager->Start (&LklKernel::MainSyscallTrampoline, &call, LKL_TASK_STACK);
  bool done = m_manager->RunNow (&call.done);
  NS_LOG_DEBUG ("system call " << no << " from the main fiber returns " << call.ret);
  NS_ASSERT_MSG (done, "system call " << no << " from the main fiber needs the simulation time to advance");
  return call.ret;
}

} // namespace ns3
