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
 *
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
//  Video playing using VLC and CCN for the transport.
//
//  For this test we use :
//
//    A. some real applications on real machine for 4 parts :
//
//     1. the vlc player with the ccn plugin : see CCNX sources in apps/vlc directory
//     2. a local real ccnd running on default port (9596), which role is to serve the VLC client,
//     3. a local real ccnd running on port 3000, which role is to serve the CCN REPOSITORY
//     4. the CCN repository talking to CCND:3000 (because it is written in Java Not Yet Supported by DCE).
//
//    B. Two virtual nodes linked by CSMA with:
//     1. the first node is bridged to the real machine using a tap-bridge,
//     2. the second one run a ccnd where VIDEO interrests and datas will transit.
//
//  For the need of tap bridge and VIDEO display NS3 is running in Real Time Mode.
//
//  CCN Routes:
//          1. ccnd(9596) forward every interrests to NS3/CCND(2000)
//          2. NS3/CCND(2000) forward every interrests to CCND(3000)
//
//  Steps:  detail in run-tap-vlc.sh
//
//     1. First launch this DCE exe which do this steps :
//         I   - Create TAP-BRIDGE
//         II  - Launch CCND on NODE 2 (10.0.0.2) and port 2000
//         III - Configure this CCND to forward interrests to CCND on 10.0.0.1:3000
//         IV  - Wait end for 10 minutes.
//
//     2. Launch real CCND:9596
//     3. Launch real CCND:3000
//     4. Launch CCN REPOSITORY
//     5. Route CCND:9596 interrests to CCND:10.0.0.2:2000
//     6. Put the video file to the repository
//     7. Launch VLC
//     8. Wait 10 minutes.
//     9. stop all daemons
//
//
//  +----------+
//  | external |
//  |  Linux   |
//  |   Host   | 1 ccnd on standard port (9596), 1 ccnd on port 3000, 1 repository using ccnd:3000
//  |          |
//  | "thetap" | 1 vlc client querying ccnx:///VIDEO/bunny.ts
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
//               | 10.0.0.1 |    | 10.0.0.2 |  ccnd:2000
//               +----------+    +----------+
//                     |               |
//                     |               |
//                     |               |
//                     =================
//                      CSMA LAN 10.0.0
//
//
//
//  Use the script run-tap-vlc.sh to run this sample.
//
//////////////////////////////////////////////////////////////////////////////

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceTapVlc");

int
main (int argc, char *argv[])
{
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";
  std::string transport = "tcp";

  CommandLine cmd;
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.AddValue ("transport", "Transport used by ccnd for interrest forwarding (tcp|udp)", transport),
  cmd.Parse (argc, argv);
  LogComponentEnable ("DceTapVlc", LOG_LEVEL_INFO);

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

  CcnClientHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch ccn daemon on node 1
  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.AddEnvironment ("CCND_CAP", "50000");
  dce.AddEnvironment ("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment ("CCN_LOCAL_PORT", "2000");

  dce.AddEnvironment ("CCND_CAP", "");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "10.0.0.2");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (0.0));

  // Forward interrest to real world !
  dce.SetBinary ("ccndc");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment ("HOME", "/root");
  dce.AddEnvironment ("CCN_LOCAL_PORT", "2000");
  dce.AddArgument ("-v");
  dce.AddArgument ("add");
  dce.AddArgument ("/");
  dce.AddArgument (transport);
  dce.AddArgument ("10.0.0.1");
  dce.AddArgument ("3000");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (10));

  Simulator::Stop (Seconds (600.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

