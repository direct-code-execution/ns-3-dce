/* -*-	Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include "ns3/object.h"

namespace ns3 {

class Task;

/**
 * \brief maintains the list of active (eligible to run) taskes.
 */
class TaskScheduler : public Object
{
public:
  static TypeId GetTypeId (void);

  /**
   * \returns the task which will be removed by
   *          TaskScheduler::DequeueNext
   */
  virtual struct Task * PeekNext (void) = 0;
  /**
   * Remove the 'active' task most eligible to become
   * the new 'current' task.
   */
  virtual void DequeueNext (void) = 0;
  /**
   * \param task a task to add to the list of 'active' tasks.
   */
  virtual void Enqueue (struct Task *task) = 0;

  virtual void Dequeue (struct Task *task) = 0;

};

} // namespace ns3

#endif /* TASK_SCHEDULER_H */
