/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 NICT
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
 * Author: Hajime Tazaki <tazaki@nict.go.jp>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/dce-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceCradleSimple");

std::map<std::string,std::string> proto_sw;
std::string m_proto = "udp";
std::string m_rate = "100Bps";
bool m_dual = false;
std::string m_ccid = "2";
bool m_bulk = false;

int
main (int argc, char *argv[])
{
  // LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

  proto_sw.insert (std::make_pair ("icmp", "ns3::LinuxIpv4RawSocketFactory"));
  proto_sw.insert (std::make_pair ("udp", "ns3::LinuxUdpSocketFactory"));
  proto_sw.insert (std::make_pair ("tcp", "ns3::LinuxTcpSocketFactory"));
  proto_sw.insert (std::make_pair ("dccp", "ns3::LinuxDccpSocketFactory"));
  proto_sw.insert (std::make_pair ("sctp", "ns3::LinuxSctpSocketFactory"));
  proto_sw.insert (std::make_pair ("icmp6", "ns3::LinuxIpv6RawSocketFactory"));
  proto_sw.insert (std::make_pair ("udp6", "ns3::LinuxUdp6SocketFactory"));
  proto_sw.insert (std::make_pair ("tcp6", "ns3::LinuxTcp6SocketFactory"));
  proto_sw.insert (std::make_pair ("dccp6", "ns3::LinuxDccp6SocketFactory"));
  proto_sw.insert (std::make_pair ("sctp6", "ns3::LinuxSctp6SocketFactory"));

  CommandLine cmd;
  cmd.AddValue ("proto", "choose protocol socket factory", m_proto);
  cmd.AddValue ("rate", "tx rate", m_rate);
  cmd.AddValue ("dual", "dual flow or not (default: uni-directional)", m_dual);
  cmd.AddValue ("ccid", "CCID if dccp (default: 2)", m_ccid);
  cmd.AddValue ("bulk", "use BulkSendApp instead of OnOffApp", m_bulk);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("100ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));
  dceManager.Install (nodes);

  LinuxStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Ipv6AddressHelper address6;
  address6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer interfaces6 = address6.Assign (devices);

  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.2), "link show");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.3), "route show table all");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.4), "addr list");
  
  stack.SysctlSet (nodes, ".net.dccp.default.rx_ccid", m_ccid);
  stack.SysctlSet (nodes, ".net.dccp.default.tx_ccid", m_ccid);

  ApplicationContainer apps;
  OnOffHelper onoff = OnOffHelper (proto_sw[m_proto],
                                   InetSocketAddress (interfaces.GetAddress (1), 9));
  PacketSinkHelper sink = PacketSinkHelper (proto_sw[m_proto],
                                            InetSocketAddress (Ipv4Address::GetAny (), 9));

  if (m_proto.find ("6", 0) != std::string::npos)
    {
      onoff.SetAttribute ("Remote", AddressValue (Inet6SocketAddress (interfaces6.GetAddress (1, 1), 9)));
      sink.SetAttribute ("Local", AddressValue (Inet6SocketAddress (Ipv6Address::GetAny (), 9)));
    }

  if (!m_bulk)
    {
      onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      onoff.SetAttribute ("PacketSize", StringValue ("1024"));
      onoff.SetAttribute ("DataRate", StringValue (m_rate));
      apps = onoff.Install (nodes.Get (0));
      apps.Start (Seconds (4.0));
      if (m_dual)
        {
          if (m_proto.find ("6", 0) != std::string::npos)
            {
              onoff.SetAttribute ("Remote", AddressValue (Inet6SocketAddress (interfaces6.GetAddress (0, 1), 9)));
            }
          else
            {
              onoff.SetAttribute ("Remote", AddressValue (InetSocketAddress (interfaces.GetAddress (0), 9)));
            }
          apps = onoff.Install (nodes.Get (1));
          apps.Start (Seconds (4.1));
        }
    }
  else
    {
      BulkSendHelper bulk (proto_sw[m_proto],
                           InetSocketAddress (interfaces.GetAddress (1), 9));
      if (m_proto.find ("6", 0) != std::string::npos)
        {
          bulk.SetAttribute ("Remote", AddressValue (Inet6SocketAddress (interfaces6.GetAddress (0, 1), 9)));
        }

      // Set the amount of data to send in bytes.  Zero is unlimited.
      bulk.SetAttribute ("MaxBytes", UintegerValue (1024));
      apps = bulk.Install (nodes.Get (0));
      apps.Start (Seconds (4.0));
      if (m_dual)
        {
          if (m_proto.find ("6", 0) != std::string::npos)
            {
              bulk.SetAttribute ("Remote", AddressValue (Inet6SocketAddress (interfaces6.GetAddress (0, 1), 9)));
            }
          else
            {
              bulk.SetAttribute ("Remote", AddressValue (InetSocketAddress (interfaces.GetAddress (0), 9)));
            }
          apps = bulk.Install (nodes.Get (1));
          apps.Start (Seconds (4.1));
        }
    }

  apps = sink.Install (nodes.Get (1));
  apps.Start (Seconds (3.9999));
  if (m_dual)
    {
      ApplicationContainer tmp = sink.Install (nodes.Get (0));
      tmp.Start (Seconds (3.9999));
      apps.Add (tmp);
    }

  pointToPoint.EnablePcapAll ("dce-cradle-simple");
  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();

  Ptr<PacketSink> pktsink;
  pktsink = apps.Get (0)->GetObject<PacketSink> ();
  std::cout << "Total Rx(0) = " << pktsink->GetTotalRx ()
            << " bytes";
  if (m_dual)
    {
      pktsink = apps.Get (1)->GetObject<PacketSink> ();
      std::cout << " Total Rx(1) = " << pktsink->GetTotalRx ()
                << " bytes";
    }
  std::cout << std::endl;
  Simulator::Destroy ();
  return 0;
}
