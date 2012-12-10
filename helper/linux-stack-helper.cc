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
#include "linux-socket-fd-factory.h"
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


void
LinuxStackHelper::SysctlGetCallback (Ptr<Node> node, std::string path, 
                                     void (*callback)(std::string, std::string))
{
  Ptr<LinuxSocketFdFactory> sock = node->GetObject<LinuxSocketFdFactory>();
  if (!sock)
    {
      callback ("error", "no socket factory");
      return;
    }

  std::string value = sock->Get (path);
  callback (path, value);
  return;
}

void
LinuxStackHelper::SysctlGet (Ptr<Node> node, Time at, std::string path, 
                             void (*callback)(std::string, std::string))
{
  Ptr<LinuxSocketFdFactory> sock = node->GetObject<LinuxSocketFdFactory>();
  if (!sock)
    {
      callback ("error", "no socket factory");
      return;
    }
  Simulator::ScheduleWithContext (node->GetId (), at,
                                  &LinuxSocketFdFactory::ScheduleTask, sock,
                                  MakeEvent (&LinuxStackHelper::SysctlGetCallback, 
                                  node, path, callback));
  return;
}
void
LinuxStackHelper::SysctlSet (NodeContainer c, std::string path, std::string value)
{
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LinuxSocketFdFactory> sock = node->GetObject<LinuxSocketFdFactory>();
      if (!sock)
        {
          continue;
        }
      sock->Set (path, value);      
    }
}

} // namespace ns3
