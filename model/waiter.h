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
#ifndef WAITER_H
#define WAITER_H

#include "ns3/nstime.h"
#include "ns3/callback.h"
#include "ns3/event-id.h"

namespace ns3 {

struct Thread;

class Waiter
{
public:
  typedef enum
  {
    INTERRUPTED,
    TIMEOUT,
    OK
  } Result;

  Waiter ();
  ~Waiter ();

  void SetTimeout (Time timeout);
  Time GetTimeoutLeft (void) const;
  // wait for a call to Wakeup, return OK if Wakeup is called
  // and no signal is pending.
  // return TIMEOUT if timeout expired
  // return INTERRUPTED if signal pending.
  Waiter::Result Wait (void);
  // wait for a call to Wakeup, handle signals and stop timeout
  // if any are pending. Return true only if Wakeup is called
  // and no signal is pending.
  bool WaitDoSignal (void);

  // wakeup someone waiting on Wait.
  void Wakeup (void);
private:
  Thread *m_waiting;
  Time m_timeout;
};

} // namespace ns3

#endif /* WAITER_H */
