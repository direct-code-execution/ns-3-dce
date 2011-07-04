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
 *  Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
*/

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-helper.h"

using namespace ns3;
//
// Network topology
//     1   2  3   4  5   6
//  n0 ---  n1 --- n2 --- nx  1Gbps, 2ms
//
// Every node run a ccnd daemon,
//
// node 0 put a file and node X get the file
//
//
NS_LOG_COMPONENT_DEFINE ("CcndInLine");

static std::string Ipv4AddressToString (Ipv4Address ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}

int main (int argc, char *argv[])
{
  //
  // Default number of nodes.  Overridable by command line argument.
  //
  uint32_t nNodes = 8;
  bool useTcp = 0;

  CommandLine cmd;
  cmd.AddValue("nNodes", "Number of nodes to place in the line", nNodes);
  cmd.AddValue ("tcp", "Use TCP to link ccnd daemons.", useTcp);
  cmd.Parse (argc, argv);

  NS_LOG_INFO( "useTcp: " << useTcp );

  NodeContainer nodes;
  nodes.Create (nNodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.0.0");

  InternetStackHelper stack;
  stack.Install (nodes);

  NetDeviceContainer devices;
  std::vector<Ipv4InterfaceContainer> vInterfaces;

  for (int n=0; n < (nNodes-1) ; n++)
    {
      devices = pointToPoint.Install (nodes.Get (n), nodes.Get ( 1 + n ) );
      Ipv4InterfaceContainer interfaces = address.Assign (devices);
      address.NewNetwork ();
      vInterfaces.push_back ( interfaces );
    }

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  // Install ccnd on each node
  for (int n=0; n < nNodes ; n++)
    {
      dce.SetStackSize (1 << 20);

      dce.SetBinary ("ccnd");
      dce.ResetArguments ();
      dce.ResetEnvironment ();

      dce.AddEnvironment ("CCND_CAP", "50000");
      dce.AddEnvironment ("CCND_DEBUG", "-1");
      dce.AddEnvironment ("CCN_LOCAL_PORT", "");

      dce.AddEnvironment ("CCND_CAP", "");
      dce.AddEnvironment ("CCND_AUTOREG", "");
      dce.AddEnvironment ("CCND_LISTEN_ON", "");
      dce.AddEnvironment ("CCND_MTU", "");
      dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
      dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
      dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");

      apps = dce.Install (nodes.Get (n));
      apps.Start (Seconds (1.0));
    }

  for (int n=0; n < nNodes ; n++)
    {
      if ( n > 0 )
        { // Forward /NODE0 interrest to prec node
          dce.SetBinary ("ccndc");
          dce.ResetArguments();
          dce.ResetEnvironment();
          dce.AddEnvironment("HOME", "/home/furbani");
          dce.AddArgument ("-v");
          dce.AddArgument ("add");
          dce.AddArgument ("/NODE0");
          dce.AddArgument (useTcp?"tcp":"udp");
          dce.AddArgument ( Ipv4AddressToString(vInterfaces[n-1].GetAddress ( 0 )) );

          apps = dce.Install (nodes.Get (n));
          apps.Start (Seconds ( ( 200.0 + n ) / 100  )); // Every 0.01s from time 2s
        }
    }

  // Publish a file using NODE number 0 : ccnput /NODE0/LeReadme </tmp/README
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("HOME", "/home/furbani");
  dce.SetBinary ("ccnput");
  dce.SetStdinFile ("/tmp/README");
  dce.AddArgument ("/NODE0/LeReadme");
  dce.AddEnvironment("HOME", "/home/furbani");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (60.0));


  // Retrieve the file using last NODE using a big CCN_LINGER value in order to have a chance of having a
  // response expecially when we use tcp to link daemons.
  // ccnget -c -a /NODE0/LeReadme
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("HOME", "/home/furbani");
  dce.AddEnvironment("CCN_LINGER", "3540"); // 1 little hour or less
  dce.SetBinary ("ccnget"); // First get can take 105s when ccnd daemons are linked with tcp, with 500 nodes.
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("-a");
  dce.AddArgument ("/NODE0/LeReadme");

  apps = dce.Install (nodes.Get (nNodes - 1));
  apps.Start (Seconds (60.0));

  // The second get is very fast but not furious :) because of cache usage the data is already in the local node !
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("HOME", "/home/furbani");
  dce.AddEnvironment("CCN_LINGER", "1");
  dce.SetBinary ("ccnget");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("-a");
  dce.AddArgument ("/NODE0/LeReadme");

  apps = dce.Install (nodes.Get (nNodes - 1));
  apps.Start (Seconds (3500.0));

  Simulator::Stop (Seconds(3600.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
