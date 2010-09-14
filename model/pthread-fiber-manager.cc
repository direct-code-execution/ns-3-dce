/*
 * Copyright (c) 2008 Sam Jansen
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
 * Author: Sam Jansen <sam.jansen@gmail.com>
 */
#include "pthread-fiber-manager.h"
#include "ns3/assert.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <setjmp.h>

namespace ns3 {

enum PthreadFiberState {
  RUNNING,
  SLEEP,
  DESTROY
};

struct PthreadFiber : public Fiber
{
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t condvar;
  enum PthreadFiberState state;
  bool thread_started;
  size_t stack_size;
  void (*func) (void *);
  void *context;
  jmp_buf initial_env;
};


PthreadFiberManager::PthreadFiberManager ()
  : m_notifySwitch (0)
{}
PthreadFiberManager::~PthreadFiberManager ()
{}

void
PthreadFiberManager::Start (struct PthreadFiber *fiber)
{
  int error;
  pthread_attr_t attr;
  error = pthread_attr_init (&attr);
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  error = pthread_attr_setstacksize (&attr, std::max (fiber->stack_size, 
						      (size_t)PTHREAD_STACK_MIN));
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  error = pthread_create (&fiber->thread, &attr, &PthreadFiberManager::Run, 
                          (void*) fiber);
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  error = pthread_attr_destroy (&attr);
  NS_ASSERT_MSG (error == 0, "error=" << strerror (error));
  fiber->thread_started = true;
}

void
PthreadFiberManager::Wakeup (struct PthreadFiber *fiber)
{
  pthread_mutex_lock (&fiber->mutex);
  fiber->state = RUNNING;

  if (fiber->thread_started) 
    {
      pthread_cond_signal (&fiber->condvar);
    } 
  else 
    {
      Start (fiber);
    }

  while (fiber->state == RUNNING)
    {
      pthread_cond_wait (&fiber->condvar, &fiber->mutex);
    }

  pthread_mutex_unlock (&fiber->mutex);
}

void
PthreadFiberManager::Yield (struct PthreadFiber *fiber)
{
  fiber->state = SLEEP;
  pthread_cond_signal (&fiber->condvar);
  while (fiber->state != RUNNING)
    {
      if (fiber->state == DESTROY)
        {
          pthread_mutex_unlock (&fiber->mutex);
          // now, we jump back to the creator of the thread
          // i.e., we unwind the stack without invoking the
          // destructors of its local variables
          longjmp (fiber->initial_env, 1);
        }
      else
        {
          pthread_cond_wait (&fiber->condvar, &fiber->mutex);
        }
    }
}

void *
PthreadFiberManager::Run (void *arg)
{
  struct PthreadFiber *fiber = (struct PthreadFiber *) arg;
  pthread_mutex_lock (&fiber->mutex);
  if (setjmp (fiber->initial_env) == 0)
    {
      fiber->func (fiber->context);
      fiber->state = DESTROY;
      pthread_cond_signal (&fiber->condvar);
      pthread_mutex_unlock (&fiber->mutex);
    }
  else
    {
      // oops, we are returning from a Delete
      // we can easily return and we are done !
    }
  pthread_detach (fiber->thread);
  return 0;
}

struct Fiber *
PthreadFiberManager::Create (void (*callback) (void *),
			     void *context,
			     uint32_t stackSize)
{
  struct PthreadFiber *fiber = (struct PthreadFiber *)CreateFromCaller ();
  fiber->func = callback;
  fiber->context = context;
  fiber->state = SLEEP;
  fiber->thread_started = false;
  fiber->stack_size = stackSize;
  return fiber;
}
struct Fiber *
PthreadFiberManager::CreateFromCaller (void)
{
  struct PthreadFiber *fiber = new PthreadFiber ();
  pthread_mutex_init (&fiber->mutex, NULL);
  pthread_cond_init (&fiber->condvar, NULL);
  fiber->state = RUNNING;
  fiber->thread_started = true;
  fiber->func = NULL;
  fiber->stack_size = 0;
  return fiber;
}
void 
PthreadFiberManager::Delete (struct Fiber *fib)
{
  struct PthreadFiber *fiber = (struct PthreadFiber *)fib;
  if (fiber->func != 0)
    {
      pthread_mutex_lock (&fiber->mutex);
      if (fiber->state != DESTROY && fiber->thread_started)
        {
	  fiber->state = DESTROY;
          pthread_cond_signal (&fiber->condvar);
          pthread_mutex_unlock (&fiber->mutex);
          pthread_join (fiber->thread, 0);
        }
      else
        {
          pthread_mutex_unlock (&fiber->mutex);
        }
    }
  int status = pthread_mutex_destroy (&fiber->mutex);
  NS_ASSERT (status == 0);
  status = pthread_cond_destroy (&fiber->condvar);
  NS_ASSERT (status == 0);
  delete fiber;
}
void 
PthreadFiberManager::SwitchTo (struct Fiber *fromFiber,
			       const struct Fiber *toFiber)
{
  struct PthreadFiber *from = (struct PthreadFiber *)fromFiber;
  struct PthreadFiber *to = (struct PthreadFiber *)toFiber;
  if (from->func != NULL) 
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
  return fiber->stack_size;
}
void 
PthreadFiberManager::SetSwitchNotification (void (*fn) (void))
{
  m_notifySwitch = fn;
}


} // namespace ns3
