/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
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
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 */
#include "file-usage.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "unix-fd.h"
#include "dce-poll.h"
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include <map>

NS_LOG_COMPONENT_DEFINE ("DceWaitQueue");

namespace ns3 {
WaitQueueEntry::WaitQueueEntry ()
{
}

WaitQueueEntry::~WaitQueueEntry ()
{
}

WaitQueueEntryPoll::WaitQueueEntryPoll (Callback<void> cb) : m_func (cb)
{
}

void
WaitQueueEntryPoll::SetPollTableEntry (PollTableEntry* p)
{
  m_pollTableEntry = p;
}

void
WaitQueueEntryPoll::WakeUp (void *key)
{
  short event = (0 != key) ? (*((short*)key)) : 0;  // Deference a short * if not 0

  if ((m_pollTableEntry) && (m_pollTableEntry->IsEventMatch (event)))
    {
      m_func ();
    }
}
PollTableEntry::PollTableEntry () :  m_file (0),
                                     m_wait (0),
                                     m_eventMask (0)
{
}
PollTableEntry::PollTableEntry (UnixFd *file, WaitQueueEntryPoll *wait, short em)
  : m_file (file),
    m_wait (wait),
    m_eventMask (em)
{
}
PollTableEntry::~PollTableEntry ()
{
  if (m_wait)
    {
      delete m_wait;
    }
}
void
PollTableEntry::FreeWait ()
{
  m_file->RemoveWaitQueue (m_wait, false);
}

int
PollTableEntry::IsEventMatch (short e) const
{
  return e & m_eventMask;
}
//WaitPoint
WaitPoint::WaitPoint () : m_waitTask (0)
{
}
WaitPoint::Result
WaitPoint::Wait (Time to)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  m_waitTask = current;
  Time left = current->process->manager->Wait (to);
  m_waitTask = 0;
  if (HasPendingSignal ())
    {
      return PollTable::INTERRUPTED;
    }
  if (!to.IsZero () && left.IsZero ())
    {
      return PollTable::TIMEOUT;
    }
  return PollTable::OK;
}

void
WaitPoint::WakeUpCallback ()
{
  NS_LOG_FUNCTION (this << m_waitTask);
  if (m_waitTask != 0)
    {
      /* The waiting thread could well be active because, it could have been
       * unblocked but not scheduled yet which means that the assignment of
       * m_waiting to zero has not been done yet in Waiter::Wait.
       */
      if (m_waitTask->task->IsBlocked ())
        {
          // NS_ASSERT (Simulator::GetContext () == m_waitTask->process->nodeId); TODO Fix this assert or Remove it :)
          m_waitTask->process->manager->Wakeup (m_waitTask);
        }
      else
        {
          NS_ASSERT (m_waitTask->task->IsActive ());
        }
    }
}


PollTable::PollTable ()
{
}

PollTable::~PollTable ()
{
  for (std::list <PollTableEntry*>::iterator i = m_pollEntryList.begin ();
       i != m_pollEntryList.end (); ++i)
    {
      delete (*i);
    }
  m_pollEntryList.clear ();
}

void
PollTable::PollWait (UnixFd* file)
{
  WaitQueueEntryPoll* we = new WaitQueueEntryPoll (MakeCallback (&PollTable::WakeUpCallback, this));
  PollTableEntry* e = new PollTableEntry (file, we, m_eventMask);
  we->SetPollTableEntry (e);
  m_pollEntryList.push_back (e);
  file->AddWaitQueue (we, false);
}
void
PollTable::PollWait (void *ref, Callback<void, void*> cb)
{
  m_pollEntryList.push_back (new PollTableEntryLinux (ref, cb));
}
void
PollTable::FreeWait ()
{
  for (std::list <PollTableEntry*> :: iterator i = m_pollEntryList.begin ();
       i != m_pollEntryList.end (); ++i)
    {
      (*i)->FreeWait ();
    }
}
void
PollTable::SetEventMask (short e)
{
  m_eventMask = e;
}
short
PollTable::GetEventMask () const
{
  return m_eventMask;
}
WaitQueueEntryTimeout::WaitQueueEntryTimeout (short em, Time to) : m_waitTask (0),
                                                                   m_eventMask (em)
{
  if (to.IsZero ())
    {
      m_lastTime = to;
    }
  else
    {
      m_lastTime = Simulator::Now () + to;
    }
}

void
WaitQueueEntryTimeout::WakeUp (void *key)
{
  short event = (0 != key) ? (*((short*)key)) : 0; // Deference a short * if not 0

  if (event & m_eventMask)
    {
      WakeUpCallback ();
    }
}
WaitPoint::Result
WaitQueueEntryTimeout::Wait ()
{
  NS_LOG_FUNCTION (m_lastTime);
  if (m_lastTime.IsNegative ())
    {
      return WaitPoint::Wait (Seconds (0));
    }
  else
    {
      Time rest = m_lastTime - Simulator::Now ();

      if (rest.IsNegative ())
        {
          return TIMEOUT;
        }
      return WaitPoint::Wait (rest);
    }
}

PollTableEntryLinux::PollTableEntryLinux (void *kernelReference, Callback<void, void*> cb)
  : m_kernelRef (kernelReference),
    m_freeCb (cb)
{
}
void
PollTableEntryLinux::FreeWait ()
{
  m_freeCb (m_kernelRef);
}
}
