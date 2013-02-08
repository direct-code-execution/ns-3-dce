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
#include <link.h>

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
};
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
  : m_notifySwitch (0)
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

  return fiber;
}

struct Fiber *
UcontextFiberManager::CreateFromCaller (void)
{
  struct UcontextFiber *fiber = new struct UcontextFiber ();
  fiber->stack = 0;
  fiber->stackSize = 0;
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

void
UcontextFiberManager::SetSwitchNotification (void (*fn)(void))
{
  m_notifySwitch = fn;
}



} // namespace ns3
