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
#include "ns3/file-descriptor-helper.h"
#include "ns3/file-descriptor-net-device.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <list>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////
//
//     tap0
//  +----------+
//  | external |
//  |  Linux   |
//  |   Host   |
//  |----------|
//  | 10.0.0.2 | ccnd
//  +----------+
//       |
//     node0
//  +----------+
//  |   dev0   |
//  | 10.0.0.1 | ccnd
//  +----------+
//
//  Interraction between a real CCND and a CCND running within NS3.
//
//  First NS3 is launched, this create the tap0 interface with 10.0.0.2 external address, and 10.0.0.1 internal address.
//
//  Then ccnd is launched on local machine with a forwarding to NS3 ccnd,
//
//  a ccnput is done,
//  and within NS3 the ccnget retrieve the file !
//
//  Use the script run-tap-ccnd.sh to run this sample.
//
//////////////////////////////////////////////////////////////////////////////

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TapCCND");

bool
Exists (char *filename) 
{
  struct stat st;
  int retval = ::stat (filename, &st);
  return retval == 0;
}

void
CreateTap (Mac48Address mac, Ipv4Address ip, Ipv4Mask mask, Ipv4Address gateway, std::string endpoint)
{
  NS_LOG_FUNCTION (mac << ip << mask << gateway << endpoint);
 
  std::ostringstream ossGateway;
  ossGateway << "-g" << gateway;
  std::ostringstream ossIp;
  ossIp << "-i" << ip;
  std::ostringstream ossMac;
  ossMac << "-m" << mac;
  std::ostringstream ossNetmask;
  ossNetmask << "-n" << mask;
  std::ostringstream ossMode;
  ossMode << "-o" << "1"; //Always CONFIGURE_LOCAL
  std::ostringstream ossPath;
  ossPath << "-p" << endpoint;

  char *tapCreator = getenv("TAPCREATOR"); 
  if ( tapCreator == NULL or !Exists(tapCreator) )
    {
      NS_FATAL_ERROR ("Could not find manager");
    }

  // fork to exec the tap creation program
  pid_t pid = ::fork ();
  if (pid == 0)
    {
      // execute the external program to create the TAP device
      int status = ::execlp (tapCreator, 
                             tapCreator,                 // argv[0] (filename)
                             //ossDeviceName.str().c_str(),     // argv[1] (-d<device name>)
                             ossGateway.str().c_str(),   // argv[2] (-g<gateway>)
                             ossIp.str().c_str(),        // argv[3] (-i<IP address>)
                             ossMac.str().c_str(),       // argv[4] (-m<MAC address>)
                             ossNetmask.str().c_str(),   // argv[5] (-n<net mask>)
                             ossMode.str().c_str(),      // argv[6] (-o<operating mode>)
                             ossPath.str().c_str(),      // argv[7] (-p<path>)
                             (char *)NULL);

      if (status == -1)
        {
          NS_LOG_ERROR ("Cannot execl tap-manager, errno=" << ::strerror (errno));
        }
      ::_exit (-1);
    }
  else
    {
      // parent
      int st;
      pid_t waited = waitpid (pid, &st, 0);
      if (waited == -1)
        {
          NS_FATAL_ERROR ("Cannot wait for tap-manager, errno=" << strerror (errno));
        }

      NS_ASSERT (pid == waited);
      if (!WIFEXITED (st))
        {
          // tap manager did not complete successfully
          NS_FATAL_ERROR ("tap-manager did not exit correctly" << st);
        }
      else if (WEXITSTATUS (st) != 0)
        {
          NS_FATAL_ERROR ("tap-manager did not complete successfully, err=" << WEXITSTATUS (st));
        }

      // the tap fd should be available on our unix socket now.
    }
}

int 
main( int argc, char *argv[] )
{
  LogComponentEnable("TapCCND", LOG_LEVEL_INFO);

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NodeContainer nodes;
  nodes.Create (1);

  InternetStackHelper stack;
  stack.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  FileDescriptorHelper fdHelper;
  NetDeviceContainer devices0;
  devices0 = fdHelper.Install (nodes.Get(0));

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces0;
  interfaces0 = address.Assign (devices0);

//  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<FileDescriptorNetDevice> device0 = DynamicCast<FileDescriptorNetDevice> (devices0.Get(0));
  std::string endpoint0 = device0->GetEndpoint();
  Mac48Address macAddress0 = Mac48Address::Allocate();
  CreateTap (macAddress0, Ipv4Address ("10.0.0.2"), Ipv4Mask ("255.255.255.252"), Ipv4Address ("10.0.0.1"), endpoint0);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1<<20);

  // Launch ccn daemon on node 0
  dce.SetBinary ("ccnd");
  dce.ResetArguments();
  dce.ResetEnvironment();

  dce.AddEnvironment("CCND_CAP", "50000");
  dce.AddEnvironment("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment("CCN_LOCAL_PORT", "2000");

  dce.AddEnvironment("CCND_CAP", "");
  dce.AddEnvironment("CCND_AUTOREG", "");
  dce.AddEnvironment("CCND_LISTEN_ON", "");
  dce.AddEnvironment("CCND_MTU", "");
  dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");
  dce.AddEnvironment("CCND_LISTEN_ON", "10.0.0.1");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));

  // Forward interrest to real world !
  dce.SetBinary ("ccndc");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment("HOME", "/home/furbani");
  dce.AddEnvironment("CCN_LOCAL_PORT", "2000");
  dce.AddArgument ("-v");
  dce.AddArgument ("add");
  dce.AddArgument ("/");
  dce.AddArgument ("tcp");
  dce.AddArgument ("10.0.0.2");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (20));


  // Retrieve a file
  dce.SetBinary ("ccnget");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("CCN_LOCAL_PORT", "2000");
  dce.AddEnvironment("HOME", "/home/furbani");
  dce.AddArgument ("-c");
  dce.AddArgument ("/REALWORLD/README");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (21));

  Simulator::Stop (Seconds (40.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

