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
#include "process-delay-model.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <sys/time.h>
#include <time.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ProcessDelayModel);
NS_LOG_COMPONENT_DEFINE ("DceProcessDelayModel");

TypeId
ProcessDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ProcessDelayModel")
    .SetParent<Object> ()
  ;
  return tid;
}

NS_OBJECT_ENSURE_REGISTERED (RandomProcessDelayModel);

TypeId
RandomProcessDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RandomProcessDelayModel")
    .SetParent<ProcessDelayModel> ()
    .AddConstructor<RandomProcessDelayModel> ()
    .AddAttribute ("Variable", "Pick the process delay at random.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"),
                   MakePointerAccessor (&RandomProcessDelayModel::m_variable),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

RandomProcessDelayModel::RandomProcessDelayModel ()
{
}

void
RandomProcessDelayModel::SetVariable (Ptr<RandomVariableStream> variable)
{
  m_variable = variable;
}

void
RandomProcessDelayModel::RecordStart (void)
{
}
Time
RandomProcessDelayModel::RecordEnd (void)
{
  return Seconds (m_variable->GetValue ());
}

NS_OBJECT_ENSURE_REGISTERED (TimeOfDayProcessDelayModel);

TypeId
TimeOfDayProcessDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TimeOfDayProcessDelayModel")
    .SetParent<ProcessDelayModel> ()
    .AddConstructor<TimeOfDayProcessDelayModel> ()
  ;
  return tid;
}

TimeOfDayProcessDelayModel::TimeOfDayProcessDelayModel ()
{
}

Time
TimeOfDayProcessDelayModel::GetTimeOfDay (void) const
{
  struct timezone tz;
  struct timeval timeval;
  gettimeofday (&timeval, &tz);
  Time s = Seconds (timeval.tv_sec);
  Time us = MicroSeconds (timeval.tv_usec);
  return s + us;
}

void
TimeOfDayProcessDelayModel::RecordStart (void)
{
  NS_LOG_FUNCTION (this);
  m_start = GetTimeOfDay ();
}
Time
TimeOfDayProcessDelayModel::RecordEnd (void)
{
  NS_LOG_FUNCTION (this);
  Time delay = GetTimeOfDay () - m_start;
  if (delay.IsZero ())
    {
      delay = MicroSeconds (1);
    }
  return delay;
}


} // namespace ns3
