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

#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/dce-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceFirst");

std::string m_stack = "dce";
std::string sock_factory = "ns3::LinuxTcpSocketFactory";
int m_seed = 1;
double startTime = 4.0;
double stopTime = 20.0;
int m_nNodes = 2;
bool enablePcap = false;
std::string m_pktSize = "1024";
bool m_frag = false;
bool m_bulk = false;

int
main (int argc, char *argv[])
{
  // LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

  CommandLine cmd;
  cmd.AddValue ("stack", "choose network stack", m_stack);
  cmd.AddValue ("seed", "randomize seed", m_seed);
  cmd.AddValue ("nNodes", "the number of nodes in left side", m_nNodes);
  cmd.AddValue ("stopTime", "duration", stopTime);
  cmd.AddValue ("enablePcap", "pcap", enablePcap);
  cmd.AddValue ("pktSize", "packet size", m_pktSize);
  cmd.AddValue ("frag", "fragment", m_frag);
  cmd.AddValue ("bulk", "use BulkSendApp instead of OnOffApp", m_bulk);
  cmd.Parse (argc, argv);

  SeedManager::SetSeed (m_seed);

  NodeContainer lefts, routers, rights, nodes;
  lefts.Create (m_nNodes);
  routers.Create (2);
  rights.Create (m_nNodes);
  nodes = NodeContainer (lefts, routers, rights);

  DceManagerHelper dceManager;
  LinuxStackHelper stack;
  InternetStackHelper internetStack;

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  if (m_stack == "dce")
    {
      //      internetStack.Install (routers);
      sock_factory = "ns3::LinuxTcpSocketFactory";
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux.so"));
      stack.Install (routers);
      stack.Install (lefts);
      stack.Install (rights);
    }
  else if (m_stack == "dce-linux3")
    {
      //internetStack.Install (routers);
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux-stable.so"));
      stack.Install (lefts);
      stack.Install (rights);
      stack.Install (routers);
    }
  else if (m_stack == "dce-dccp")
    {
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux.so"));
      sock_factory = "ns3::LinuxDccpSocketFactory";
      stack.Install (routers);
      stack.Install (lefts);
      stack.Install (rights);
    }
  else if (m_stack == "dce-dccp3")
    {
      internetStack.Install (routers);
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("liblinux-stable.so"));
      sock_factory = "ns3::LinuxDccpSocketFactory";
      stack.Install (lefts);
      stack.Install (rights);
    }
  else if (m_stack == "ns3")
    {
      // Thx, Brian !
      Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
      Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));

      sock_factory = "ns3::TcpSocketFactory";
      internetStack.Install (nodes);
    }
  else if (m_stack == "nsc-linux")
    {
      internetStack.Install (routers);
      sock_factory = "ns3::TcpSocketFactory";
      internetStack.SetTcp ("ns3::NscTcpL4Protocol",
                            "Library", StringValue ("liblinux2.6.26.so"));
      internetStack.Install (lefts);
      internetStack.Install (rights);
      // Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control",
      //              StringValue ("reno"));
    }
  else if (m_stack == "nsc-freebsd")
    {
      // didn't work under 64bit (12/11/13)
      sock_factory = "ns3::TcpSocketFactory";
      internetStack.SetTcp ("ns3::NscTcpL4Protocol",
                            "Library", StringValue ("libfreebsd5.3.so"));
      internetStack.Install (nodes);
    }
  else
    {
      NS_ASSERT_MSG (0, "no stack " << m_stack << " is available");
    }

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ns"));

  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interfaces;

  NetDeviceContainer dev0, dev1, dev2;
  for (uint32_t i = 0; i < m_nNodes; i++)
    {
      std::ostringstream oss;
      oss << "10.0." << i << ".0";
      address.SetBase (oss.str ().c_str (), "255.255.255.0");
      dev0 = pointToPoint.Install (NodeContainer (lefts.Get (i), routers.Get (0)));
      address.Assign (dev0);
    }

  // bottle neck link
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("100ms"));
  dev1 = pointToPoint.Install (NodeContainer (routers.Get (0), routers.Get (1)));
  if (m_frag)
    {
      dev1.Get (0)->SetMtu (1000);
      dev1.Get (1)->SetMtu (1000);
    }
  // bottle neck link
  Ptr<RateErrorModel> em1 =
    CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
                                                "ErrorRate", DoubleValue (0.05),
                                                "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET)
                                                );
  dev1.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

  address.SetBase ("10.1.0.0", "255.255.255.0");
  address.Assign (dev1);

  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ns"));
  // for right links
  for (uint32_t i = 0; i < m_nNodes; i++)
    {
      std::ostringstream oss;
      oss << "10.2." << i << ".0";
      address.SetBase (oss.str ().c_str (), "255.255.255.0");
      dev2 = pointToPoint.Install (NodeContainer (routers.Get (1), rights.Get (i)));
      address.Assign (dev2);
    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  if (m_stack.find ("dce") != std::string::npos)
    {
      LinuxStackHelper::PopulateRoutingTables ();
      dceManager.Install (nodes);
      stack.SysctlSet (nodes, ".net.ipv4.conf.default.forwarding", "1");
    }

  // dceManager.RunIp (lefts.Get (0), Seconds (0.2), "route add default via 10.0.0.2");
  // dceManager.RunIp (lefts.Get (1), Seconds (0.2), "route add default via 10.0.1.2");
  // dceManager.RunIp (routers.Get (0), Seconds (0.2), "route add 10.2.0.0/16 via 10.1.0.2");
  // dceManager.RunIp (routers.Get (1), Seconds (0.2), "route add 10.0.0.0/16 via 10.1.0.1");
  // dceManager.RunIp (routers.Get (1), Seconds (0.2), "route show");
  // dceManager.RunIp (routers.Get (1), Seconds (0.2), "link set lo up");
  // dceManager.RunIp (rights.Get (0), Seconds (0.2), "route add 0.0.0.0/0 via 10.2.0.1");
  // dceManager.RunIp (rights.Get (1), Seconds (0.2), "route add default via 10.2.1.1");
  // dceManager.RunIp (rights.Get (0), Seconds (0.2), "route show");

  ApplicationContainer apps;
  DceApplicationHelper process;

  OnOffHelper onoff = OnOffHelper (sock_factory,
                                   InetSocketAddress (Ipv4Address ("10.2.0.2"), 2000));
  onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute ("PacketSize", StringValue (m_pktSize));
  onoff.SetAttribute ("DataRate", StringValue ("1Mbps"));

  BulkSendHelper bulk = BulkSendHelper (sock_factory,
                                        InetSocketAddress ("10.2.0.2", 2000));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  bulk.SetAttribute ("MaxBytes", UintegerValue (0));
  bulk.SetAttribute ("SendSize", UintegerValue (atoi (m_pktSize.c_str ())));

  // Flow 1 - n
  for (uint32_t i = 0; i < m_nNodes; i++)
    {
      std::ostringstream oss;
      oss << "10.2." << i << ".2";
      if (!m_bulk)
        {
          onoff.SetAttribute ("Remote", 
                              AddressValue (InetSocketAddress (Ipv4Address (oss.str ().c_str ()), 2000)));
          onoff.SetAttribute ("StartTime", TimeValue (Seconds (startTime)));
          apps = onoff.Install (lefts.Get (i));
        }
      else
        {
          bulk.SetAttribute ("Remote", 
                             AddressValue (InetSocketAddress (Ipv4Address (oss.str ().c_str ()), 2000)));
          apps = bulk.Install (lefts.Get (i));
          apps.Start (Seconds (startTime));
        }
    }

  PacketSinkHelper sink = PacketSinkHelper (sock_factory,
                                            InetSocketAddress (Ipv4Address::GetAny (), 2000));
  apps = sink.Install (rights);
  apps.Start (Seconds (3.9999));

  if (enablePcap)
    {
      pointToPoint.EnablePcapAll ("dce-tcp-comparison");
    }

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  Ptr<PacketSink> pktsink;
  uint64_t total = 0;
  std::cout << "Total ";
  for (uint32_t i = 0; i < m_nNodes; i++)
    {
      pktsink = apps.Get (i)->GetObject<PacketSink> ();
      std::cout << "Rx(" << i << ") = " << pktsink->GetTotalRx ()
                << " bytes ("
                << pktsink->GetTotalRx () * 8 / (stopTime - startTime)
                << " bps), ";
      total += pktsink->GetTotalRx () ;
    }
  std::cout << " total = " << total * 8 / (stopTime - startTime) << " bps";
  std::cout << std::endl;

  Simulator::Destroy ();
  return 0;
}
