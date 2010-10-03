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
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <setjmp.h>
#include <algorithm>
#include <unistd.h>

namespace ns3 {

enum PthreadFiberState {
  RUNNING,
  SLEEP,
  DESTROY
};

class MemoryBounds
{
public:
  MemoryBounds () 
    : m_min (~0), m_max (0) {}
  void AddBound (void *address)
  {
    unsigned long v = (unsigned long) address;
    m_min = std::min (v, m_min);
    m_max = std::max (v, m_max);
  }
  void *GetStart (void) const
  {
    int size = getpagesize ();
    long start = m_min - (m_min % size);
    return (void*)start;
  }
  size_t GetSize (void) const
  {
    int size = getpagesize ();
    unsigned long start = m_min - (m_min % size);
    unsigned long end = ((m_max % size) == 0)?m_max:(m_max+(size-(m_max %size)));
    return end - start;
  }
private:
  unsigned long m_min;
  unsigned long m_max;
};


struct PthreadFiberThread;

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
  void (*func) (void *);
  void *context;
  size_t stack_size;
  jmp_buf *yield_env;
  struct PthreadFiber *previous;
  MemoryBounds stack_bounds;
};



PthreadFiberManager::PthreadFiberManager ()
  : m_notifySwitch (0)
{}
PthreadFiberManager::~PthreadFiberManager ()
{}

struct Fiber *
PthreadFiberManager::Clone (struct Fiber *fib)
{
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
  memcpy (clone->stack_copy, 
	  clone->stack_bounds.GetStart (),
	  clone->stack_bounds.GetSize ());
  // save the current state in jmp_buf so that the next call to 
  // SwitchTo on the clone comes back here.
  int status = setjmp (clone->yield_env);
  if (status == 0)
    {
      // returning directly. parent
      return clone;
    }
  else
    {
      // child
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
PthreadFiberManager::RestoreFiber (struct PthreadFiber *fiber)
{
  // before we wakeup the thread, we need to
  // make sure that we have the right jmp_buf
  // to jump to when the call to cond_wait returns in Yield.
  fiber->thread->yield_env = &fiber->yield_env;
  // we also need to make sure that we return to the
  // right stack.
  struct PthreadFiber *previous = fiber->thread->previous;
  if (previous != fiber)
    {
      // first, we save the stack of previous.
      if (previous->stack_copy == 0)
	{
	  previous->stack_copy = malloc (previous->thread->stack_size);
	}
      memcpy (previous->stack_copy,
	      previous->stack_bounds.GetStart (),
	      previous->stack_bounds.GetSize ());
      // then, we restore the stack of next
      memcpy (fiber->stack_bounds.GetStart (), 
	      fiber->stack_copy,
	      fiber->stack_bounds.GetSize ());
    }
}

void
PthreadFiberManager::Wakeup (struct PthreadFiber *fiber)
{
  pthread_mutex_lock (&fiber->thread->mutex);
  fiber->state = RUNNING;
  if (fiber->thread->thread_started) 
    {
      RestoreFiber (fiber);
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
	  int status = setjmp (fiber->yield_env);
	  if (status == 0)
	    {
	      // wait for the master thread to re-schedule us.
	      pthread_cond_wait (&fiber->thread->condvar, &fiber->thread->mutex);
	      // go back to the setjmp above or the setjmp in ::Clone
	      longjmp (*(fiber->thread->yield_env), 1);
	    }
        }
    }
}

void *
PthreadFiberManager::Run (void *arg)
{
  struct PthreadFiber *fiber = (struct PthreadFiber *) arg;
  fiber->thread->stack_bounds.AddBound (__builtin_frame_address (0));
  fiber->thread->stack_bounds.AddBound (SelfStackBottom ());
  pthread_mutex_lock (&fiber->thread->mutex);
  if (setjmp (fiber->thread->initial_env) == 0)
    {
      fiber->thread->func (fiber->thread->context);
      fiber->state = DESTROY;
      pthread_cond_signal (&fiber->thread->condvar);
      pthread_mutex_unlock (&fiber->thread->mutex);
    }
  else
    {
      // oops, we are returning from a Delete
      // we can easily return and we are done !
    }
  pthread_detach (fiber->thread->thread);
  return 0;
}

struct Fiber *
PthreadFiberManager::Create (void (*callback) (void *),
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
	      RestoreFiber (fiber);
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
PthreadFiberManager::SetSwitchNotification (void (*fn) (void))
{
  m_notifySwitch = fn;
}


} // namespace ns3
