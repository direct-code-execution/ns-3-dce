/* -*-  Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 * Author:  Frederic Urbani <frederic.urbani@inria.fr>
 */

#ifndef DCE_NODE_CONTEXT_H
#define DCE_NODE_CONTEXT_H

#include <string>
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/simulator.h"
#include <sys/utsname.h>

extern "C" struct Libc;

namespace ns3 {

struct Process;
struct Thread;

/**
 * \brief Manages data attached to a Node usable by DCE such as uname result random context ...
  */
class DceNodeContext : public Object
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  DceNodeContext ();
  virtual ~DceNodeContext ();

  int UName (struct utsname *buf);

private:
  std::string sysName;
  std::string nodeName;
  std::string release;
  std::string version;
  std::string hardId;

};

} // namespace ns3

#endif /* DCE_NODE_CONTEXT_H */
