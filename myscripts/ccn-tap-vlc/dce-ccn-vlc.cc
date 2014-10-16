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
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////
//
//  Video playing using VLC and CCN for the transport.
//
//  For this test we use :
//
//    A. some real applications on real machine for 2 parts :
//
//     1. the vlc player with the ccn plugin : see CCNX sources in apps/vlc directory
//     2. a local real ccnd running on default port (9596), which role is to serve the VLC client,
//
//    B. Two virtual nodes linked by CSMA with:
//     1. the first node is bridged to the real machine using a tap-bridge,
//     2. the second one run a ccnd where VIDEO interrests and datas will transit,
//        with also a ccnr the repository serving the VIDEO.
//
//  For the need of tap bridge and VIDEO display NS3 is running in Real Time Mode.
//
//  CCN Route:
//          1. the real ccnd(9596) forward interrests starting by /VIDEO to NS3/CCND on node 2
//
//  Steps:  detail in run-ccn-vlc.sh
//
//     1. First launch this DCE exe which do this steps :
//         I   - Create TAP-BRIDGE
//         II  - Launch CCND on NODE 2 (10.0.0.2) at second 1.0
//         III - Launch CCNR on NODE 2 (10.0.0.2) at second 2.0
//         IV  - Publish the file into the repository using this command (at second 5.0) :
//               SyncTest -put  $VIDEOFILE ccnx:///VIDEO/bunny.ts
//         V   - Wait end for 10 minutes.
//
//     2. Launch real CCND:9596
//     3. Route CCND:9596 interrests to CCND:10.0.0.2
//     4. Launch VLC
//     5. Wait 10 minutes.
//     6. stop real ccnd
//
//
//  +----------+
//  | external |
//  |  Linux   |
//  |   Host   | 1 ccnd on standard port (9596)
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
//               +----------+    +----------+  ccnr
//               | 10.0.0.1 |    | 10.0.0.2 |  ccnd
//               +----------+    +----------+
//                     |               |
//                     |               |
//                     |               |
//                     =================
//                      CSMA LAN 10.0.0
//
//
//
//  Use the script run-ccn-vlc.sh to run this sample.
//
//////////////////////////////////////////////////////////////////////////////

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceTapVlc");

int
main (int argc, char *argv[])
{
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";
  std::string videofile = "bunny.ts";
  std::string dataRate = "1800Mbps";

  CommandLine cmd;
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("video", "Video File to use", videofile);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.AddValue ("dr", "DataRate default : 1800Mbps", dataRate);
  cmd.Parse (argc, argv);
  LogComponentEnable ("DceTapVlc", LOG_LEVEL_INFO);

  std::string copy = "cp " + videofile + " files-1/tmp/video";

  // Create need directories
  mkdir ("files-1",0744);
  mkdir ("files-1/REPO/",0744);
  mkdir ("files-1/tmp/",0744);
  // Copy the video file to /tmp of NODE 1.
  int ret = system (copy.c_str ());

  if ( !WIFEXITED (ret) || (0 != WEXITSTATUS (ret)))
    {
      std::cout << "Unable to copy the video file named:" << videofile << std::endl;
      return 0;
    }
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NodeContainer nodes;
  nodes.Create (2);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);


  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma.SetChannelAttribute ("Delay", StringValue ("1ms") );
  NetDeviceContainer devices = csma.Install (nodes);

  Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> (
      "RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.|Max=1.]"),
      "ErrorRate", DoubleValue (0.00001));
  // devices.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

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

  // Launch ccnd daemon on node 1
  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.AddEnvironment ("CCND_CAP", "50000");
//  dce.AddEnvironment("CCND_DEBUG", "-1");
  dce.AddEnvironment ("CCND_CAP", "");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "10.0.0.2");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (1.0));

  // Launch the repository on node 1 : ccnr , with a repository in dir /REPO
  dce.SetBinary ("ccnr");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("CCNR_DIRECTORY", "/REPO/");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (2.0));

  // Publish the video file to the repository : SyncTest -put  /tmp/video ccnx:///VIDEO/bunny.ts
  // The video file has been copied in node 1 at the start of the scenario
  dce.SetBinary ("SyncTest");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("-put");
  dce.AddArgument ("/tmp/video");
  dce.AddArgument ("ccnx:///VIDEO/bunny.ts");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (5.0));

  Simulator::Stop (Seconds (600.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

