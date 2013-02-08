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
#ifdef KERNEL_STACK
  Ipv4Linux::InstallNode (node);
#endif
}
void
LinuxStackHelper::Install (std::string nodeName)
{
#ifdef KERNEL_STACK
  Ptr<Node> node = Names::Find<Node> (nodeName);
  Install (node);
#endif
}
void
LinuxStackHelper::Install (NodeContainer c)
{
#ifdef KERNEL_STACK
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Install (*i);
    }
#endif
}
void
LinuxStackHelper::InstallAll (void)
{
#ifdef KERNEL_STACK
  Install (NodeContainer::GetGlobal ());
#endif
}


void
LinuxStackHelper::SysctlGetCallback (Ptr<Node> node, std::string path,
                                     void (*callback)(std::string, std::string))
{
#ifdef KERNEL_STACK
  Ptr<LinuxSocketFdFactory> sock = node->GetObject<LinuxSocketFdFactory> ();
  if (!sock)
    {
      callback ("error", "no socket factory");
      return;
    }

  std::string value = sock->Get (path);
  callback (path, value);
  return;
#endif
}

void
LinuxStackHelper::SysctlGet (Ptr<Node> node, Time at, std::string path,
                             void (*callback)(std::string, std::string))
{
#ifdef KERNEL_STACK
  Ptr<LinuxSocketFdFactory> sock = node->GetObject<LinuxSocketFdFactory> ();
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
#endif
}
void
LinuxStackHelper::SysctlSet (NodeContainer c, std::string path, std::string value)
{
#ifdef KERNEL_STACK
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<LinuxSocketFdFactory> sock = node->GetObject<LinuxSocketFdFactory> ();
      if (!sock)
        {
          continue;
        }
      Simulator::ScheduleWithContext (node->GetId (), Seconds (0.0),
                                      MakeEvent (&LinuxSocketFdFactory::Set, sock,
                                                 path, value));
    }
#endif
}

} // namespace ns3
