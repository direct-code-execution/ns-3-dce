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
//  +----------+
//  | external |
//  |  Linux   |
//  |   Host   |
//  |          |
//  | "thetap" |
//  +----------+
//  | 10.0.0.1 | ccnd
//  +----------+
//       |           node0         node1
//       |       +----------+    +----------+
//       +-------|  tap     |    |          |
//               | bridge   |    |          |
//               +----------+    +----------+
//               |  CSMA    |    |  CSMA    |
//               +----------+    +----------+
//               | 10.0.0.1 |    | 10.0.0.2 |  ccnd
//               +----------+    +----------+
//                     |               |
//                     |               |
//                     |               |
//                     =================
//                      CSMA LAN 10.0.0
//
//
//  Interraction between a real CCND and a CCND running within NS3.
//
//  First NS3 is launched, this create the tap-bridge with 10.0.0.1 address
//
//  Then ccnd is launched on local machine with a forwarding to NS3 ccnd listening on 10.0.0.2,
//
//  a ccnput is done,
//  and within NS3 the ccnget retrieve the file !
//
//  then the local machine do also a ccnget to retrieve a file from within NS3.
//
//  Use the script run-tap-ccnd.sh to run this sample.
//
//////////////////////////////////////////////////////////////////////////////

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapCCND");

void
CreateReadme ()
{
  std::ofstream osf ("/tmp/README", std::fstream::trunc);

  osf << "The wanted data is here :)" ;

  osf.close ();
}


int
main (int argc, char *argv[])
{
  std::string mode = "ConfigureLocal";
  std::string tapName = "thetap";
  int ccnxVersion = 4;

  CommandLine cmd;
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.AddValue ("cv", "Ccnx version 4 for 0.4.x variantes and 5 for 0.5.x variantes, default: 4",
                ccnxVersion);
  cmd.Parse (argc, argv);

  LogComponentEnable ("TapCCND", LOG_LEVEL_INFO);

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

  CreateReadme ();

  CcnClientHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch ccn daemon on node 0
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
  apps.Start (Seconds (1.0));

  // Stop ccnd before simu end.
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (9.0));

  // Publish file
  dce.SetBinary ((ccnxVersion == 4) ? "ccnput" : "ccnpoke");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment ("HOME", "/root");
  dce.AddEnvironment ("CCN_LOCAL_PORT", "2000");
  dce.AddArgument ("/NS3WORLD/README");
  dce.SetStdinFile ("/tmp/README");
  dce.AddFile ("/tmp/README","/tmp/README");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (2));

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
  dce.AddArgument ("tcp");
  dce.AddArgument ("10.0.0.1");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (3));


  // Retrieve a file
  dce.SetBinary ((ccnxVersion == 4) ? "ccnget" : "ccnpeek");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("CCN_LOCAL_PORT", "2000");
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("-c");
  dce.AddArgument ("/REALWORLD/README");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (5));

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

