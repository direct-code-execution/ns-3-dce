/*
 * Copyright (c) 2008 Sam Jansen, INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Sam Jansen <sam.jansen@gmail.com>
 *          Mathieu Lacage <mathieu.lacage@inria.fr>
 */
#include "pthread-fiber-manager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <setjmp.h>
#include <algorithm>
#include <unistd.h>
#include <signal.h>

#ifdef HAVE_VALGRIND_H
# include "valgrind/valgrind.h"
# include "valgrind/memcheck.h"
#else
# define VALGRIND_STACK_REGISTER(start,end) (0)
# define VALGRIND_STACK_DEREGISTER(id)
# define VALGRIND_MAKE_MEM_DEFINED(start,sz)
#endif


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DcePthreadFiberManager");

enum PthreadFiberState
{
  RUNNING,
  SLEEP,
  DESTROY
};

class MemoryBounds
{
public:
  MemoryBounds ()
    : m_min (~0),
      m_max (0)
  {
  }
  void AddBound (void *address)
  {
    unsigned long v = (unsigned long) address;
    m_min = std::min (v, m_min);
    m_max = std::max (v, m_max);
  }
  void * GetStart (void) const
  {
    int size = getpagesize ();
    long start = m_min - (m_min % size);
    return (void*)start;
  }
  size_t GetSize (void) const
  {
    int size = getpagesize ();
    unsigned long start = m_min - (m_min % size);
    unsigned long end = ((m_max % size) == 0) ? m_max : (m_max + (size - (m_max % size)));
    return end - start;
  }
private:
  unsigned long m_min;
  unsigned long m_max;
};


struct PthreadFiberThread;
class StackTrampoline;

struct PthreadFiber : public Fiber
{
  struct PthreadFiberThread *thread;
  enum PthreadFiberState state;
  void *stack_copy;
  jmp_buf yield_env;
  size_t yield_stack_size;
  MemoryBounds stack_bounds;
};

struct PthreadFiberThread
{
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t condvar;
  uint32_t refcount;
  bool thread_started;
  jmp_buf initial_env;
  void (*func)(void *);
  void *context;
  size_t stack_size;
  StackTrampoline *trampoline;
  struct PthreadFiber *previous;
  struct PthreadFiber *next;
  MemoryBounds stack_bounds;
};

class StackTrampoline
{
public:
  StackTrampoline ()
  {
    int status = 0;
    status = pthread_mutex_lock (&g_mutex);
    g_current = this;
    NS_ASSERT_MSG (status == 0, "lock failed");
    m_stack = malloc (SIGSTKSZ);
    stack_t newstack;
    stack_t oldstack;
    newstack.ss_sp = m_stack;
    newstack.ss_size = SIGSTKSZ;
    newstack.ss_flags = 0;
    m_vgId = VALGRIND_STACK_REGISTER (m_stack,((unsigned long)m_stack) + SIGSTKSZ);
    status = sigaltstack (&newstack, &oldstack);
    NS_ASSERT_MSG (status == 0, "first sigaltstack failed stack=" << m_stack <<
                   " stacksize=" << SIGSTKSZ);
    struct sigaction newact;
    struct sigaction oldact;
    newact.sa_handler = &StackTrampoline::SignalHandler;
    newact.sa_flags = SA_ONSTACK | SA_RESETHAND;
    sigemptyset (&newact.sa_mask);
    status = sigaction (SIGUSR1, &newact, &oldact);
    NS_ASSERT_MSG (status == 0, "first sigaction failed");
    status = raise (SIGUSR1);
    NS_ASSERT_MSG (status == 0, "raise failed");
    status = sigaltstack (&oldstack, 0);
    NS_ASSERT_MSG (status == 0, "second sigaltstack failed");
    g_current = 0;
    status = pthread_mutex_unlock (&g_mutex);
    NS_ASSERT_MSG (status == 0, "unlock failed");
  }
  ~StackTrampoline ()
  {
    VALGRIND_STACK_DEREGISTER (m_vgId);
    free (m_stack);
  }

  void Jump (PthreadFiberThread *thread)
  {
    NS_LOG_FUNCTION (this << thread);
    m_jumpTarget = thread;
    struct PthreadFiber *next = thread->next;
    struct PthreadFiber *previous = thread->previous;
    // this is a CPU optimization: we make sure that
    // there is indeed data to save and restore before
    // switching to a temporary stack to do save/restore.
    if (previous != next)
      {
        VALGRIND_MAKE_MEM_DEFINED (m_stack, SIGSTKSZ);
        longjmp (m_buf, 1);
      }
  }

private:
  // This is the handler that gets invoked the first time
  // we trigger the SIGUSR1 signal. The first time this function
  // enters, we return immediately (setjmp returns 0)
  // Every other time that setjmp returns because someone called
  // longjmp in StackTrampoline::Jump, we call instead DoWork
  // to save the current stack frame and restore the stack of the
  // thread that is going to be executed.
  static void SignalHandler (int signo)
  {
    StackTrampoline *self = g_current;
    if (setjmp (self->m_buf) == 0)
      {
        return;
      }
    VALGRIND_MAKE_MEM_DEFINED (self->m_stack, SIGSTKSZ);
    DoWork (self->m_jumpTarget);
  }
  static void DoWork (PthreadFiberThread *thread)
  {
    NS_LOG_DEBUG ("alternate trampoline stack");
    struct PthreadFiber *next = thread->next;
    struct PthreadFiber *previous = thread->previous;
    if (previous != next)
      {
        if (previous != 0)
          {
            // first, we save the stack of previous.
            if (previous->stack_copy == 0)
              {
                previous->stack_copy = malloc (previous->thread->stack_size);
              }
            VALGRIND_MAKE_MEM_DEFINED (previous->stack_bounds.GetStart (),
                                       previous->stack_bounds.GetSize ());
            NS_LOG_DEBUG ("save start=" << previous->stack_bounds.GetStart () <<
                          " size=" << previous->stack_bounds.GetSize ());
            memcpy (previous->stack_copy,
                    previous->stack_bounds.GetStart (),
                    previous->stack_bounds.GetSize ());
          }
        // then, we restore the stack of next
        VALGRIND_MAKE_MEM_DEFINED (next->stack_bounds.GetStart (),
                                   next->stack_bounds.GetSize ());
        NS_LOG_DEBUG ("restore start=" << next->stack_bounds.GetStart () <<
                      " size=" << next->stack_bounds.GetSize ());
        memcpy (next->stack_bounds.GetStart (),
                next->stack_copy,
                next->stack_bounds.GetSize ());
      }
    // Finally, we can go back to the thread's last suspend point
    // which was either in Yield or in Clone.
    longjmp (next->yield_env, 1);
  }
  void *m_stack;
  jmp_buf m_buf;
  PthreadFiberThread *m_jumpTarget;
  unsigned int m_vgId;
  static pthread_mutex_t g_mutex;
  static StackTrampoline *g_current;
};

pthread_mutex_t StackTrampoline::g_mutex = PTHREAD_MUTEX_INITIALIZER;
StackTrampoline *StackTrampoline::g_current = 0;




PthreadFiberManager::PthreadFiberManager ()
  : m_notifySwitch (0)
{
  NS_LOG_FUNCTION (this);
  m_trampoline = new StackTrampoline ();
}
PthreadFiberManager::~PthreadFiberManager ()
{
  NS_LOG_FUNCTION (this);
  delete m_trampoline;
}

struct Fiber *
PthreadFiberManager::Clone (struct Fiber *fib)
{
  NS_LOG_FUNCTION (this << fib);
  struct PthreadFiber *fiber = (struct PthreadFiber *)fib;
  // check that this is not the main thread that
  // we are cloning.
  NS_ASSERT (fiber->thread->stack_size != 0);
  struct PthreadFiber *clone = new PthreadFiber ();
  clone->thread = fiber->thread;
  clone->thread->refcount++;
  clone->state = SLEEP;
  clone->stack_bounds = clone->thread->stack_bounds;
  clone->stack_bounds.AddBound (__builtin_frame_address (0));
  clone->stack_bounds.AddBound (SelfStackBottom ());

  clone->stack_copy = malloc (fiber->thread->stack_size);
  // save current stack so that the next call to SwitchTo
  // on the clone restores the stack before jumping back
  // to the below.
  {
    void *src = clone->stack_bounds.GetStart ();
    void *dst = clone->stack_copy;
    size_t sz = clone->stack_bounds.GetSize ();
    VALGRIND_MAKE_MEM_DEFINED (src, sz);
    NS_LOG_DEBUG ("save start=" << clone->stack_bounds.GetStart () <<
                  " size=" << clone->stack_bounds.GetSize ());

//    dst = clone->stack_copy = malloc (sz);

    memcpy (dst, src, sz);

//    NS_ASSERT (sz >=  fiber->thread->stack_size);

  }
  // save the current state in jmp_buf so that the next call to
  // SwitchTo on the clone comes back here.
  if (setjmp (clone->yield_env) == 0)
    {
      // returning directly. parent
      NS_LOG_DEBUG ("created clone " << clone);
      return clone;
    }
  else
    {
      // child
      NS_LOG_DEBUG ("returning from clone " << clone);
      return 0;
    }
}

void *
PthreadFiberManager::SelfStackBottom (void)
{
  return __builtin_frame_address (0);
}

void
PthreadFiberManager::Start (struct PthreadFiber *fiber)
{
  int error;
  pthread_attr_t attr;
  error = pthread_attr_init (&attr);
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  error = pthread_attr_setstacksize (&attr, std::max (fiber->thread->stack_size,
                                                      (size_t)PTHREAD_STACK_MIN));
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  error = pthread_create (&fiber->thread->thread, &attr, &PthreadFiberManager::Run,
                          (void*) fiber);
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  error = pthread_attr_destroy (&attr);
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  fiber->thread->thread_started = true;
}

void
PthreadFiberManager::Wakeup (struct PthreadFiber *fiber)
{
  NS_LOG_FUNCTION (this << fiber);
  pthread_mutex_lock (&fiber->thread->mutex);
  fiber->state = RUNNING;
  fiber->thread->next = fiber;
  if (fiber->thread->thread_started)
    {
      // and now we can wakup the target thread. yay !
      pthread_cond_signal (&fiber->thread->condvar);
    }
  else
    {
      Start (fiber);
    }

  while (fiber->state == RUNNING)
    {
      // and wait until the target thread completes.
      pthread_cond_wait (&fiber->thread->condvar, &fiber->thread->mutex);
    }

  pthread_mutex_unlock (&fiber->thread->mutex);
}

void
PthreadFiberManager::Yield (struct PthreadFiber *fiber)
{
  NS_LOG_FUNCTION (this << fiber);
  fiber->state = SLEEP;
  pthread_cond_signal (&fiber->thread->condvar);
  while (fiber->state != RUNNING)
    {
      fiber->thread->previous = fiber;
      if (fiber->state == DESTROY)
        {
          pthread_mutex_unlock (&fiber->thread->mutex);
          // now, we jump back to the creator of the thread
          // i.e., we unwind the stack without invoking the
          // destructors of its local variables
          longjmp (fiber->thread->initial_env, 1);
        }
      else
        {
          fiber->stack_bounds = fiber->thread->stack_bounds;
          fiber->stack_bounds.AddBound (__builtin_frame_address (0));
          fiber->stack_bounds.AddBound (SelfStackBottom ());
          if (setjmp (fiber->yield_env) == 0)
            {
              // force the thread variable to be stored on the stack.
              volatile PthreadFiberThread *thread = fiber->thread;
              NS_LOG_DEBUG ("Yield after setjmp before wait");
              // wait for the master thread to re-schedule us.
              pthread_cond_wait (&((PthreadFiberThread *)thread)->condvar,
                                 &((PthreadFiberThread *)thread)->mutex);
              NS_LOG_DEBUG ("Yield after wait");
              // finally, jump back where we want to go within this thread
              ((PthreadFiberThread *)thread)->trampoline->Jump ((PthreadFiberThread *)thread);
            }
          NS_LOG_DEBUG ("Yield after setjmp");
        }
    }
}

void *
PthreadFiberManager::Run (void *arg)
{
  struct PthreadFiber *fiber = (struct PthreadFiber *) arg;
  struct PthreadFiberThread *thread = (struct PthreadFiberThread *) fiber->thread;
  thread->stack_bounds.AddBound (__builtin_frame_address (0));
  thread->stack_bounds.AddBound (SelfStackBottom ());
  pthread_mutex_lock (&thread->mutex);
  if (setjmp (thread->initial_env) == 0)
    {
      thread->func (thread->context);
      fiber->state = DESTROY;
      pthread_cond_signal (&thread->condvar);
      pthread_mutex_unlock (&thread->mutex);
      pthread_detach (thread->thread);
    }
  else
    {
      // oops, we are returning from a Delete
      // we can easily return and we are done !
    }
  return 0;
}

struct Fiber *
PthreadFiberManager::Create (void (*callback)(void *),
                             void *context,
                             uint32_t stackSize)
{
  struct PthreadFiber *fiber = (struct PthreadFiber *)CreateFromCaller ();
  fiber->thread->func = callback;
  fiber->thread->context = context;
  fiber->thread->thread_started = false;
  fiber->thread->stack_size = stackSize;
  fiber->state = SLEEP;
  return fiber;
}
struct Fiber *
PthreadFiberManager::CreateFromCaller (void)
{
  struct PthreadFiberThread *thread = new PthreadFiberThread ();
  thread->trampoline = m_trampoline;
  thread->refcount = 1;
  thread->thread_started = true;
  thread->func = NULL;
  thread->stack_size = 0;
  thread->previous = 0;
  pthread_mutex_init (&thread->mutex, NULL);
  pthread_cond_init (&thread->condvar, NULL);
  struct PthreadFiber *fiber = new PthreadFiber ();
  fiber->thread = thread;
  fiber->state = RUNNING;
  fiber->stack_copy = 0;
  return fiber;
}
void
PthreadFiberManager::Delete (struct Fiber *fib)
{
  struct PthreadFiber *fiber = (struct PthreadFiber *)fib;
  fiber->thread->refcount--;
  if (fiber->thread->refcount == 0)
    {
      if (fiber->thread->func != 0)
        {
          pthread_mutex_lock (&fiber->thread->mutex);
          if (fiber->state != DESTROY && fiber->thread->thread_started)
            {
              fiber->state = DESTROY;
              pthread_cond_signal (&fiber->thread->condvar);
              pthread_mutex_unlock (&fiber->thread->mutex);
              pthread_join (fiber->thread->thread, 0);
            }
          else
            {
              pthread_mutex_unlock (&fiber->thread->mutex);
            }
        }
      int status = pthread_mutex_destroy (&fiber->thread->mutex);
      NS_ASSERT (status == 0);
      status = pthread_cond_destroy (&fiber->thread->condvar);
      NS_ASSERT (status == 0);
      delete fiber->thread;
    }
  else if (fiber->thread->previous == fiber)
    {
      fiber->thread->previous = 0;
    }
  if (fiber->stack_copy != 0)
    {
      free (fiber->stack_copy);
    }
  delete fiber;
}
void
PthreadFiberManager::SwitchTo (struct Fiber *fromFiber,
                               const struct Fiber *toFiber)
{
  struct PthreadFiber *from = (struct PthreadFiber *)fromFiber;
  struct PthreadFiber *to = (struct PthreadFiber *)toFiber;
  if (from->thread->func != NULL)
    {
      // We're in an application thread, and we know our mutexes are locked
      Yield (from);
    }
  else
    {
      // We're the controller (main) thread
      Wakeup (to);
    }
  if (m_notifySwitch != 0)
    {
      m_notifySwitch ();
    }
}
uint32_t
PthreadFiberManager::GetStackSize (struct Fiber *fib) const
{
  struct PthreadFiber *fiber = (struct PthreadFiber *)fib;
  return fiber->thread->stack_size;
}
void
PthreadFiberManager::SetSwitchNotification (void (*fn)(void))
{
  m_notifySwitch = fn;
}


} // namespace ns3
