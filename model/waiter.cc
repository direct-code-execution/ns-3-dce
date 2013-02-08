/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "waiter.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "task-manager.h"
#include "ns3/simulator.h"
#include "ns3/event-id.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("Waiter");

namespace ns3 {

Waiter::Waiter ()
  : m_waiting (0),
    m_timeout (Seconds (0.0))
{
  NS_LOG_FUNCTION (this);
}

Waiter::~Waiter ()
{
  NS_LOG_FUNCTION (this);
}

void
Waiter::SetTimeout (Time timeout)
{
  m_timeout = timeout;
}
Time
Waiter::GetTimeoutLeft (void) const
{
  return m_timeout;
}

Waiter::Result
Waiter::Wait (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  m_waiting = current;
  Time left = current->process->manager->Wait (m_timeout);
  m_waiting = 0;
  if (HasPendingSignal ())
    {
      m_timeout = Seconds (0.0);
      NS_LOG_DEBUG ("interrupted self=" << this << " current=" << current);
      return Waiter::INTERRUPTED;
    }
  if (!m_timeout.IsZero () && left.IsZero ())
    {
      m_timeout = Seconds (0.0);
      NS_LOG_DEBUG ("timeout self=" << this << " current=" << current);
      return Waiter::TIMEOUT;
    }

  m_timeout = left;
  NS_LOG_DEBUG ("ok self=" << this << " current=" << current);
  return Waiter::OK;
}
bool
Waiter::WaitDoSignal (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  Waiter::Result result = Wait ();
  switch (result)
    {
    case Waiter::INTERRUPTED:
      UtilsDoSignal ();
      current->err = EINTR;
      return false;
      break;
    case Waiter::TIMEOUT:
      current->err = EAGAIN;
      return false;
      break;
    case Waiter::OK:
      break;
    }
  return true;
}

void
Waiter::Wakeup (void)
{
  NS_LOG_FUNCTION (this << m_waiting);
  if (m_waiting != 0)
    {
      /* The waiting thread could well be active because, it could have been
       * unblocked but not scheduled yet which means that the assignment of
       * m_waiting to zero has not been done yet in Waiter::Wait.
       */
      if (m_waiting->task->IsBlocked ())
        {
          m_waiting->process->manager->Wakeup (m_waiting);
        }
      else
        {
          NS_ASSERT (m_waiting->task->IsActive ());
        }
    }
}

} // namespace ns3
