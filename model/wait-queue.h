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
#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

#include "ns3/callback.h"
#include "ns3/nstime.h"
#include <list>

namespace ns3 {

struct Thread;
class UnixFd;

/**
* \brief Wait queue are similar to linux kernel wait queues.
*
*  Wait queue are used by poll and in many Files implementation to tracks which task is waiting
*  for io occurs on this files.
*/

/**
 * Entry in the WaitQueue
 */
class WaitQueueEntry
{
public:
  WaitQueueEntry ();
  // virtual ~WaitQueueEntry ();

  // Wake Up the waiter of some events
  // \param key: generic parameter describing the event source
  virtual void WakeUp (void *key) = 0;
};

class PollTableEntry;

/**
 * WaitQueueEntry specialised for poll usage.
 */
class WaitQueueEntryPoll : public WaitQueueEntry
{
public:
  /**
   * \param: cb callback used to wake up the waiter task.
   */
  WaitQueueEntryPoll (Callback<void> cb);
//  virtual ~WaitQueueEntryPoll ();

  virtual void WakeUp (void *key);
  void SetPollTableEntry (PollTableEntry* p);

private:
  Callback<void> const m_func;
  PollTableEntry* m_pollTableEntry;
};

/**
 * Entry in the following Poll Table,
 * Each pollable file must subscribe itself in the PollTable by creating a PollTableEntry
 * when it is involved in a poll call
 */
class PollTableEntry
{
public:
  PollTableEntry ();
  /**
   * \param file: the involved file,
   * \param wait: the wait queue entry in the wait queue of the file,
   * \param eventMask: Set of wanted events
   */
  PollTableEntry (UnixFd *file, WaitQueueEntryPoll* wait, short eventMask);
  virtual ~PollTableEntry ();

  /**
   * Free the wait queue entry
   */
  virtual void FreeWait ();
  int IsEventMatch (short e) const;

private:
  UnixFd* const m_file;
  WaitQueueEntryPoll* const m_wait;
  short const m_eventMask;
};

/**
 * Utility to stop a thread and wake up it.
 */
class WaitPoint
{
public:
  typedef enum
  {
    INTERRUPTED,
    TIMEOUT,
    OK
  } Result;

  WaitPoint ();

  // Stop the thread until a wakeup or timeout reached .
  // \param: time max to wait or 0 for no max
  WaitPoint::Result Wait (Time to);
  void WakeUpCallback ();

private:
  Thread* m_waitTask;
};
/**
 * Poll table used to store WaitQueues of waiting files.
 */
class PollTable : public WaitPoint
{
public:
  PollTable ();
  ~PollTable ();

  // Remove from every wait queues
  void FreeWait ();
  // Add new file to Poll table and add corresponding poll table entry to file's wait queue.
  void PollWait (UnixFd* file);
  void PollWait (void *ref, Callback<void, void*> cb);
  void SetEventMask (short e);
  short GetEventMask () const;

private:
  std::list <PollTableEntry*> m_pollEntryList;
  short m_eventMask;
};


class PollTableEntryLinux : public PollTableEntry
{
public:
  /**
   * \param file: the involved file,
   * \param wait: the wait queue entry in the wait queue of the file,
   * \param eventMask: Set of wanted events
   */
  PollTableEntryLinux (void *kernelReference, Callback<void, void*> cb);


  virtual void FreeWait ();

private:
  void * const m_kernelRef;
  Callback<void, void*> const m_freeCb;
};


/**
 * Generic wait queue entry joined with a waitpoint
 */
class WaitQueueEntryTimeout : public WaitQueueEntry,
                              public WaitPoint
{
public:
  WaitQueueEntryTimeout (short eventMask, Time timeout);
  virtual void WakeUp (void *key);
  WaitPoint::Result Wait ();

private:
  Thread* m_waitTask;
  short m_eventMask;
  Time m_lastTime;
};
#define RETURNFREE(A) { if (wq) { delete wq; wq = 0; } return A; }
}
#endif /* WAIT_QUEUE_H */
