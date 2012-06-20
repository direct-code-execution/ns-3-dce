/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 INRIA
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
 * Author: Frédéric Urbani
 */
#include "linux-stack-helper.h"
#include "ipv4-linux.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/names.h"

namespace ns3 {
void
LinuxStackHelper::Install (Ptr<Node> node)
{
  Ipv4Linux::InstallNode (node);
}
void
LinuxStackHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  Install (node);
}
void
LinuxStackHelper::Install (NodeContainer c)
{
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Install (*i);
    }
}
void
LinuxStackHelper::InstallAll (void)
{
  Install (NodeContainer::GetGlobal ());
}
} // namespace ns3
