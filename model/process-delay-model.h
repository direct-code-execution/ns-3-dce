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
#ifndef PROCESS_DELAY_MODEL_H
#define PROCESS_DELAY_MODEL_H

#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"

namespace ns3 {

class ProcessDelayModel : public Object
{
public:
  static TypeId GetTypeId (void);

  virtual void RecordStart (void) = 0;
  virtual Time RecordEnd (void) = 0;
};

class RandomProcessDelayModel : public ProcessDelayModel
{
public:
  static TypeId GetTypeId (void);

  RandomProcessDelayModel ();

  void SetVariable (Ptr<RandomVariableStream> variable);

  virtual void RecordStart (void);
  virtual Time RecordEnd (void);

private:
  Ptr<RandomVariableStream> m_variable;
};

class TimeOfDayProcessDelayModel : public ProcessDelayModel
{
public:
  static TypeId GetTypeId (void);

  TimeOfDayProcessDelayModel ();

  virtual void RecordStart (void);
  virtual Time RecordEnd (void);
private:
  Time GetTimeOfDay (void) const;
  Time m_start;
};

} // namespace ns3

#endif /* PROCESS_DELAY_MODEL_H */
