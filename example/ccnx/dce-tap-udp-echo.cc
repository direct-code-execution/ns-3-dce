/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 *  Authors: Frederic Urbani <frederic.urbani@inria.fr>
 *   and tap parts from fd-tap-p2p.c from Alina Quereilhac <alina.quereilhac@sophia.inria.fr
*/

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/csma-helper.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <list>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////
//
//  +----------+
//  | external |
//  |  Linux   |
//  |   Host   |
//  |          |
//  | "thetap" |
//  +----------+
//  | 10.0.0.1 |
//  +----------+
//       |           node0         node1
//       |       +----------+    +----------+
//       +-------|  tap     |    |          |
//               | bridge   |    |          |
//               +----------+    +----------+
//               |  CSMA    |    |  CSMA    |
//               +----------+    +----------+
//               | 10.0.0.1 |    | 10.0.0.2 | udp-echo-server listening on port 2000
//               +----------+    +----------+
//                     |               |
//                     |               |
//                     |               |
//                     =================
//                      CSMA LAN 10.0.0
//
//////////////////////////////////////////////////////////////////////////////

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceTapUDPEcho");

int
main (int argc, char *argv[])
{
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";

  CommandLine cmd;
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.Parse (argc, argv);

  LogComponentEnable ("TapUDPEcho", LOG_LEVEL_INFO);

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NodeContainer nodes;
  nodes.Create (2);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));
  NetDeviceContainer devices = csma.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper addresses;
  addresses.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = addresses.Assign (devices);

  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue (mode));
  tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
  tapBridge.Install (nodes.Get (0), devices.Get (0));

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  dce.SetBinary ("udp-echo-server");
  dce.ResetArguments ();
  dce.AddArgument ("10.0.0.2");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (0.1));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (4.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

