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
#include "ipv6-linux.h"
#include "linux-socket-fd-factory.h"
#include "dce-application-helper.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

namespace ns3 {
LinuxStackHelper::LinuxStackHelper ()
  : m_routing (0)
{
  Initialize ();
}

// private method called by both constructor and Reset ()
void
LinuxStackHelper::Initialize ()
{
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4GlobalRoutingHelper globalRouting;
  Ipv4ListRoutingHelper listRouting;
  listRouting.Add (staticRouting, 0);
  listRouting.Add (globalRouting, -10);
  SetRoutingHelper (listRouting);
}

LinuxStackHelper::~LinuxStackHelper ()
{
  delete m_routing;
}

void 
LinuxStackHelper::SetRoutingHelper (const Ipv4RoutingHelper &routing)
{
  delete m_routing;
  m_routing = routing.Copy ();
}

void
LinuxStackHelper::Install (Ptr<Node> node)
{
#ifdef KERNEL_STACK
  Ipv4Linux::InstallNode (node);
#if 0
  // Set routing
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  Ptr<Ipv4RoutingProtocol> ipv4Routing = m_routing->Create (node);
  ipv4->SetRoutingProtocol (ipv4Routing);
#endif
  Ipv6Linux::InstallNode (node);
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
LinuxStackHelper::PopulateRoutingTables ()
{
#ifdef KERNEL_STACK
  NodeContainer c =  NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<Ipv4Linux> ipv4 = node->GetObject<Ipv4Linux> ();
      ipv4->PopulateRoutingTable ();
    }
#endif
}

void
LinuxStackHelper::RunIp (Ptr<Node> node, Time at, std::string str)
{
#ifdef KERNEL_STACK
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1 << 16);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
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
      NS_ASSERT_MSG (0, "No LinuxSocketFdFactory is installed. "
                     "You may need to do it via DceManagerHelper::Install ()");
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
      NS_ASSERT_MSG (0, "No LinuxSocketFdFactory is installed. "
                     "You may need to do it via DceManagerHelper::Install ()");
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
          NS_ASSERT_MSG (0, "No LinuxSocketFdFactory is installed. "
                         "You may need to do it via DceManagerHelper::Install ()");
        }
      // i.e., TaskManager::Current() needs it.
      Simulator::ScheduleWithContext (node->GetId (), Seconds (0.1),
                                      MakeEvent (&LinuxSocketFdFactory::Set, sock,
                                                 path, value));
    }
#endif
}

} // namespace ns3
