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
#ifndef RR_TASK_SCHEDULER_H
#define RR_TASK_SCHEDULER_H

#include "task-scheduler.h"
#include <list>

namespace ns3 {

/**
 * \brief Round Robin scheduler
 */
class RrTaskScheduler : public TaskScheduler
{
public:
  static TypeId GetTypeId (void);
  RrTaskScheduler ();

  virtual Task * PeekNext (void);
  virtual void DequeueNext (void);
  virtual void Enqueue (Task *task);
  virtual void Dequeue (Task *task);
private:
  std::list<struct Task *> m_active;
};

} // namespace ns3

#endif /* RR_TASK_SCHEDULER_H */
