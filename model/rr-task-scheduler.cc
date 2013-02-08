/* -*-	Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2008 INRIA
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
#include "rr-task-scheduler.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("RrTaskScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RrTaskScheduler);

TypeId
RrTaskScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RrTaskScheduler")
    .SetParent<TaskScheduler> ()
    .AddConstructor<RrTaskScheduler> ()
  ;
  return tid;
}
RrTaskScheduler::RrTaskScheduler ()
{
}

struct Task *
RrTaskScheduler::PeekNext (void)
{
  if (m_active.empty ())
    {
      return 0;
    }
  struct Task *task = m_active.front ();
  NS_LOG_DEBUG ("next=" << task);
  return task;
}
void
RrTaskScheduler::DequeueNext (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (!m_active.empty ());
  m_active.pop_front ();
}
void
RrTaskScheduler::Enqueue (struct Task *task)
{
  NS_LOG_FUNCTION (this << task);
  m_active.push_back (task);
}
void
RrTaskScheduler::Dequeue (struct Task *task)
{
  NS_LOG_FUNCTION (this << task);
  m_active.remove (task);
}

} // namespace ns3
