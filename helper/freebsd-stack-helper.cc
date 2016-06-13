/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Hajime Tazaki
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
 * Author:  Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */
#include "freebsd-stack-helper.h"
#include "ipv4-freebsd.h"
#include "freebsd-socket-fd-factory.h"
#include "dce-application-helper.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/names.h"

namespace ns3 {
void
FreeBSDStackHelper::Install (Ptr<Node> node)
{
  Ipv4FreeBSD::InstallNode (node);
}
void
FreeBSDStackHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  Install (node);
}
void
FreeBSDStackHelper::Install (NodeContainer c)
{
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Install (*i);
    }
}
void
FreeBSDStackHelper::InstallAll (void)
{
  Install (NodeContainer::GetGlobal ());
}

void
FreeBSDStackHelper::PopulateRoutingTables ()
{
  NodeContainer c =  NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<Ipv4FreeBSD> ipv4 = node->GetObject<Ipv4FreeBSD> ();
      ipv4->PopulateRoutingTable ();
    }
}

void
FreeBSDStackHelper::RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("freebsd-iproute");
  process.SetStackSize (1 << 16);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

void
FreeBSDStackHelper::SysctlGetCallback (Ptr<Node> node, std::string path,
                                     void (*callback)(std::string, std::string))
{
  Ptr<FreeBSDSocketFdFactory> sock = node->GetObject<FreeBSDSocketFdFactory> ();
  if (!sock)
    {
      callback ("error", "no socket factory");
      NS_ASSERT_MSG (0, "No FreeBSDSocketFdFactory is installed. "
                     "You may need to do it via DceManagerHelper::Install ()");
      return;
    }

  std::string value = sock->Get (path);
  callback (path, value);
  return;
}

void
FreeBSDStackHelper::SysctlGet (Ptr<Node> node, Time at, std::string path,
                             void (*callback)(std::string, std::string))
{
  Ptr<FreeBSDSocketFdFactory> sock = node->GetObject<FreeBSDSocketFdFactory> ();
  if (!sock)
    {
      callback ("error", "no socket factory");
      NS_ASSERT_MSG (0, "No FreeBSDSocketFdFactory is installed. "
                     "You may need to do it via DceManagerHelper::Install ()");
      return;
    }
  Simulator::ScheduleWithContext (node->GetId (), at,
                                  &FreeBSDSocketFdFactory::ScheduleTask, sock,
                                  MakeEvent (&FreeBSDStackHelper::SysctlGetCallback,
                                             node, path, callback));
  return;
}
void
FreeBSDStackHelper::SysctlSet (NodeContainer c, std::string path, std::string value)
{
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<FreeBSDSocketFdFactory> sock = node->GetObject<FreeBSDSocketFdFactory> ();
      if (!sock)
        {
          NS_ASSERT_MSG (0, "No FreeBSDSocketFdFactory is installed. "
                         "You may need to do it via DceManagerHelper::Install ()");
        }
      Simulator::ScheduleWithContext (node->GetId (), Seconds (0.1),
                                      MakeEvent (&FreeBSDSocketFdFactory::Set, sock,
                                                 path, value));
    }
}

} // namespace ns3
