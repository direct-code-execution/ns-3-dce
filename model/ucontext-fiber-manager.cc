#define _GNU_SOURCE 1
#include "ucontext-fiber-manager.h"
#include "ns3/fatal-error.h"
#include "ns3/assert.h"
#include <ucontext.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/auxv.h>
#include <sys/syscall.h>
#include <link.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <asm/prctl.h>
#include <linux/futex.h>
#include <vector>

#ifdef HAVE_VALGRIND_H
# include "valgrind/valgrind.h"
#else
# define VALGRIND_STACK_REGISTER(start,end) (0)
# define VALGRIND_STACK_DEREGISTER(id)
#endif


namespace ns3 {

void *UcontextFiberManager::g_alternateSignalStack = 0;
std::list<unsigned long> UcontextFiberManager::g_guardPages;

struct UcontextFiber : public Fiber
{
  uint8_t *stack;
  uint32_t stackSize;
  ucontext_t context;
  unsigned int vgId;
  void *tp; // thread pointer (%fs base), 0 for the simulator thread's
};

// Thread local storage of the fibers
//
// All the fibers run on the simulator's thread and so share its thread local
// variables: errno, the C++ exception globals, the malloc caches, and the
// thread local variables of the programs (a browser keeps the task runner of
// each of its threads there). With per-fiber TLS, every fiber runs on the
// thread control block and static TLS of a host thread of its own, parked in
// the kernel for good as soon as it has started: glibc set its TLS up and
// keeps it up to date (dlopen() initializes the static TLS of a new module in
// every thread), and switching to the fiber only loads its address in %fs.
// The thread control block keeps the simulator thread's identity, its
// pthread_self() and the tid with which glibc's locks record their owner, as
// when all the fibers shared it.

#ifndef HWCAP2_FSGSBASE
# define HWCAP2_FSGSBASE (1 << 1)
#endif

extern "C" void __ctype_init (void);

// Every fiber, for debuggers (utils/dce-fibers-gdb.py prints their stacks).
extern "C" {
struct UcontextFiber *g_dceFibers[8192];
int g_dceFiberCount = 0;
}

static bool g_wrfsbase = false;
static void *g_mainTp = 0;
static size_t g_tidOffset = 0;
static std::vector<void *> g_freeTls;
static int g_neverWakes = 0;

static void *
GetThreadPointer (void)
{
  void *tp;
  asm volatile ("mov %%fs:0, %0" : "=r" (tp));
  return tp;
}

static void
SetThreadPointer (void *tp)
{
  if (g_wrfsbase)
    {
      asm volatile ("wrfsbase %0" : : "r" (tp) : "memory");
    }
  else
    {
      syscall (SYS_arch_prctl, ARCH_SET_FS, tp);
    }
}

static void *
ParkedThread (void *arg)
{
  __atomic_store_n ((pid_t *)arg, (pid_t) syscall (SYS_gettid), __ATOMIC_RELEASE);
  for (;;)
    {
      // a raw system call: from now on the fibers own this thread's TLS
      register long r10 asm ("r10") = 0;
      long ret = SYS_futex;
      asm volatile ("syscall"
                    : "+a" (ret)
                    : "D" (&g_neverWakes), "S" (FUTEX_WAIT_PRIVATE), "d" (0), "r" (r10)
                    : "rcx", "r11", "memory");
    }
  return 0;
}

static bool
IsParked (pid_t tid)
{
  char path[64];
  snprintf (path, sizeof (path), "/proc/self/task/%d/stat", tid);
  int fd = open (path, O_RDONLY);
  if (fd == -1)
    {
      return false;
    }
  char buffer[256];
  ssize_t n = read (fd, buffer, sizeof (buffer) - 1);
  close (fd);
  if (n <= 0)
    {
      return false;
    }
  buffer[n] = 0;
  // pid (comm) state ...: comm may hold spaces and parentheses
  char *end = strrchr (buffer, ')');
  return end != 0 && end[1] == ' ' && end[2] == 'S';
}

static int
ResetModuleTls (struct dl_phdr_info *info, size_t size, void *data)
{
  if (info->dlpi_tls_data == 0)
    {
      return 0;
    }
  for (int i = 0; i < info->dlpi_phnum; i++)
    {
      const ElfW (Phdr) *phdr = &info->dlpi_phdr[i];
      if (phdr->p_type == PT_TLS)
        {
          memcpy (info->dlpi_tls_data, (void *)(info->dlpi_addr + phdr->p_vaddr), phdr->p_filesz);
          memset ((uint8_t *)info->dlpi_tls_data + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
        }
    }
  return 0;
}

static void *
AllocateTls (void)
{
  if (!g_freeTls.empty ())
    {
      // a fiber's old TLS: back to the initial values of every module
      void *tp = g_freeTls.back ();
      g_freeTls.pop_back ();
      void *current = GetThreadPointer ();
      SetThreadPointer (tp);
      dl_iterate_phdr (&ResetModuleTls, 0);
      // what glibc sets up when a thread starts: its ctype tables
      __ctype_init ();
      SetThreadPointer (current);
      return tp;
    }
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  sigset_t all;
  sigfillset (&all);
  pthread_attr_setsigmask_np (&attr, &all);
  // the static TLS is at the top of the thread's stack
  size_t stackSize = 256 * 1024;
  pthread_t thread;
  pid_t tid = 0;
  int status;
  do
    {
      pthread_attr_setstacksize (&attr, stackSize);
      status = pthread_create (&thread, &attr, &ParkedThread, &tid);
      stackSize *= 2;
    }
  while (status == EINVAL && stackSize <= 64 * 1024 * 1024);
  pthread_attr_destroy (&attr);
  if (status != 0)
    {
      NS_FATAL_ERROR ("Unable to create the thread of a fiber's TLS: " << strerror (status));
    }
  pthread_detach (thread);
  while (__atomic_load_n (&tid, __ATOMIC_ACQUIRE) == 0 || !IsParked (tid))
    {
      sched_yield ();
    }
  // On x86_64, pthread_t is the address of the thread control block, the
  // thread pointer: tcbhead_t is { tcb, dtv, self, ... }.
  void *tp = (void *) thread;
  ((void **) tp)[2] = ((void **) g_mainTp)[2];
  *(pid_t *)((uint8_t *) tp + g_tidOffset) = *(pid_t *)((uint8_t *) g_mainTp + g_tidOffset);
  return tp;
}
void
UcontextFiberManager::SegfaultHandler (int sig, siginfo_t *si, void *unused)
{
  int pagesize = sysconf (_SC_PAGE_SIZE);
  if (pagesize == -1)
    {
      NS_FATAL_ERROR ("Unable to query page size");
    }
  unsigned long page = (unsigned long) si->si_addr;
  page = page - (page % pagesize);
  for (std::list<unsigned long>::iterator i = g_guardPages.begin ();
       i != g_guardPages.end (); ++i)
    {
      if (*i == page)
        {
          // This is a stack overflow: all we can do is print some error message
          {
            char message[] = "Stack overflow !";
            write (2, message, strlen (message));
          }
          break;
        }
    }
}

void
UcontextFiberManager::FreeAlternateSignalStack (void)
{
  free (g_alternateSignalStack);
}

void
UcontextFiberManager::SetupSignalHandler (void)
{
  static bool alreadySetup = false;
  if (alreadySetup)
    {
      return;
    }
  alreadySetup = true;

  stack_t ss;

  /**
   * We _need_ to setup an alternate signal stack because the kernel will
   * refuse to deliver a SIGSEGV signal to our process while it is on
   * the stack which triggered this same error. This kind of makes sense
   * so, we cannot blame the kernel for this.
   */
  ss.ss_sp = malloc (SIGSTKSZ);
  ss.ss_size = SIGSTKSZ;
  ss.ss_flags = 0;
  int status = sigaltstack (&ss, NULL);
  if (status == -1)
    {
      NS_FATAL_ERROR ("Unable to setup an alternate signal stack handler, errno="
                      << strerror (errno));
    }
  g_alternateSignalStack = ss.ss_sp;

  atexit (&FreeAlternateSignalStack);

  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
  sigemptyset (&sa.sa_mask);
  sa.sa_sigaction = &UcontextFiberManager::SegfaultHandler;
  status = sigaction (SIGSEGV, &sa, NULL);
  if (status == -1)
    {
      NS_FATAL_ERROR ("Unable to setup page fault handler, errno="
                      << strerror (errno));
    }
}

uint32_t
UcontextFiberManager::CalcStackSize (uint32_t size)
{
  int pagesize = sysconf (_SC_PAGE_SIZE);
  if (pagesize == -1)
    {
      NS_FATAL_ERROR ("Unable to query page size");
    }

  if ((size % pagesize) == 0)
    {
      return size + 2 * pagesize;
    }
  else
    {
      return size + (pagesize - (size % pagesize)) + 2 * pagesize;
    }
}

uint8_t *
UcontextFiberManager::AllocateStack (uint32_t size)
{
  int pagesize = sysconf (_SC_PAGE_SIZE);
  if (pagesize == -1)
    {
      NS_FATAL_ERROR ("Unable to query page size");
    }

  SetupSignalHandler ();

  uint32_t realSize = CalcStackSize (size);
  void *map = mmap (0, realSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (map == MAP_FAILED)
    {
      NS_FATAL_ERROR ("Unable to allocate stack pages: size=" << size <<
                      ", alloced=" << realSize <<
                      ", errno=" << strerror (errno));
    }
  uint8_t *stack = (uint8_t *)map;
  int status = mprotect (stack + pagesize, realSize - 2 * pagesize, PROT_READ | PROT_WRITE);
  if (status == -1)
    {
      NS_FATAL_ERROR ("Unable to protect bottom of stack space, errno=" << strerror (errno));
    }
  g_guardPages.push_back ((unsigned long)stack);
  return stack + pagesize;
}
void
UcontextFiberManager::DeallocateStack (uint8_t *buffer,
                                       uint32_t stackSize)
{
  int pagesize = sysconf (_SC_PAGE_SIZE);
  if (pagesize == -1)
    {
      NS_FATAL_ERROR ("Unable to query page size, errno=" << strerror (errno));
    }
  uint32_t realSize = CalcStackSize (stackSize);
  int status = munmap (buffer - pagesize, realSize);
  if (status == -1)
    {
      NS_FATAL_ERROR ("Unable to unmap stack, errno=" << strerror (errno));
    }
  unsigned long guard = (unsigned long)(buffer - pagesize);
  g_guardPages.remove (guard);
}

UcontextFiberManager::UcontextFiberManager ()
  : m_notifySwitch (0),
    m_perFiberTls (false)
{
}
UcontextFiberManager::~UcontextFiberManager ()
{
}

void
UcontextFiberManager::Trampoline (int a0, int a1, int a2, int a3)
{
  NS_ASSERT (sizeof(int) >= 4);
  uint64_t fn = 0;
  fn |= (uint32_t)a0;
  fn <<= 32;
  fn |= (uint32_t)a1;
  uint64_t ctx = 0;
  ctx |= (uint32_t)a2;
  ctx <<= 32;
  ctx |= (uint32_t)a3;

  void (*cb)(void *) = (void (*) (void*))fn;
  cb ((void*)ctx);
}

struct Fiber *
UcontextFiberManager::Create (void (*callback)(void *),
                              void *context,
                              uint32_t stackSize)
{
  struct UcontextFiber *fiber = new struct UcontextFiber ();
  uint8_t *stack;
  int retval;

  fiber->tp = m_perFiberTls ? AllocateTls () : 0;

  stack = AllocateStack (stackSize);
  fiber->vgId = VALGRIND_STACK_REGISTER (stack,stack + stackSize);
  fiber->stack = stack;
  fiber->stackSize = stackSize;

  retval = getcontext (&fiber->context);
  NS_ASSERT (retval != -1);
  fiber->context.uc_stack.ss_sp = stack;
  fiber->context.uc_stack.ss_size = stackSize;
  // make sure the thread exits when it completes.
  fiber->context.uc_link = NULL;

  uint64_t cb = (uint64_t)callback;
  uint64_t ctx = (uint64_t)context;
  uint32_t a0 = cb >> 32;
  uint32_t a1 = cb & 0xffffffff;
  uint32_t a2 = ctx >> 32;
  uint32_t a3 = ctx & 0xffffffff;
  void (*fn)() = (void (*) ()) & UcontextFiberManager::Trampoline;
  makecontext (&fiber->context, fn, 4, a0, a1, a2, a3);

  if (g_dceFiberCount < (int)(sizeof (g_dceFibers) / sizeof (g_dceFibers[0])))
    {
      g_dceFibers[g_dceFiberCount++] = fiber;
    }
  return fiber;
}

struct Fiber *
UcontextFiberManager::CreateFromCaller (void)
{
  struct UcontextFiber *fiber = new struct UcontextFiber ();
  fiber->stack = 0;
  fiber->stackSize = 0;
  fiber->tp = 0;
  return fiber;
}

void
UcontextFiberManager::Delete (struct Fiber *fib)
{
  struct UcontextFiber *fiber = (struct UcontextFiber *)fib;
  VALGRIND_STACK_DEREGISTER (fiber->vgId);
  if (fiber->stack != 0)
    {
      DeallocateStack (fiber->stack, fiber->stackSize);
    }
  if (fiber->tp != 0)
    {
      g_freeTls.push_back (fiber->tp);
    }
  for (int i = 0; i < g_dceFiberCount; i++)
    {
      if (g_dceFibers[i] == fiber)
        {
          g_dceFibers[i] = g_dceFibers[--g_dceFiberCount];
          break;
        }
    }
  fiber->stack = 0;
  fiber->stackSize = 0xdeadbeaf;
  delete fiber;
}

void
UcontextFiberManager::SwitchTo (struct Fiber *fromFiber,
                                const struct Fiber *toFiber)
{
  struct UcontextFiber *from = (struct UcontextFiber *)fromFiber;
  struct UcontextFiber *to = (struct UcontextFiber *)toFiber;
  if (m_perFiberTls)
    {
      SetThreadPointer (to->tp != 0 ? to->tp : g_mainTp);
    }
  swapcontext (&from->context, &to->context);
  if (m_notifySwitch != 0)
    {
      m_notifySwitch ();
    }
}

uint32_t
UcontextFiberManager::GetStackSize (struct Fiber *fib) const
{
  struct UcontextFiber *fiber = (struct UcontextFiber *)fib;
  return fiber->stackSize;
}

void *
UcontextFiberManager::GetStackBottom (struct Fiber *fib) const
{
  struct UcontextFiber *fiber = (struct UcontextFiber *)fib;
  return fiber->stack;
}

void
UcontextFiberManager::SetSwitchNotification (void (*fn)(void))
{
  m_notifySwitch = fn;
}

bool
UcontextFiberManager::EnablePerFiberTls (void)
{
#if defined(__x86_64__) && defined(__GLIBC__)
  if (g_mainTp == 0)
    {
      // The offset of the tid in glibc's struct pthread, from the
      // descriptions glibc keeps for libthread_db: { bits, count, offset }.
      void *libc = dlopen ("libc.so.6", RTLD_LAZY | RTLD_NOLOAD);
      const uint32_t *tidDesc = libc ? (const uint32_t *) dlvsym (libc, "_thread_db_pthread_tid", "GLIBC_PRIVATE") : 0;
      if (tidDesc == 0 || tidDesc[0] != 8 * sizeof (pid_t)
          || *(pid_t *)((uint8_t *) GetThreadPointer () + tidDesc[2]) != (pid_t) syscall (SYS_gettid))
        {
          return false;
        }
      g_tidOffset = tidDesc[2];
      g_mainTp = GetThreadPointer ();
      g_wrfsbase = (getauxval (AT_HWCAP2) & HWCAP2_FSGSBASE) != 0;
    }
  NS_ASSERT_MSG (GetThreadPointer () == g_mainTp, "per-fiber TLS is enabled from the simulator thread");
  m_perFiberTls = true;
  return true;
#else
  return false;
#endif
}

bool
UcontextFiberManager::HasPerFiberTls (void) const
{
  return m_perFiberTls;
}



} // namespace ns3
