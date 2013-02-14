/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/dce-module.h"
#include "ns3/mpi-interface.h"

using namespace ns3;

// Run Hint :  $ mpirun -np 2 dce-mpi-udp

int
main (int argc, char *argv[])
{
  // Distributed simulation setup
  MpiInterface::Enable (&argc, &argv);
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::DistributedSimulatorImpl"));

  uint32_t systemId = MpiInterface::GetSystemId ();
  uint32_t systemCount = MpiInterface::GetSize ();


  // Check for valid distributed parameters.
  // Must have 2 and only 2 Logical Processors (LPs)

  if (systemCount != 2)
    {
      std::cout << "This simulation requires 2 and only 2 logical processors." << std::endl;
      return 1;
    }

  //-------------------------------------------------


  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  Ptr<Node> node1 = CreateObject<Node> (0); // <------ for MPI, it goes to run in core_1 (process 1)
  Ptr<Node> node2 = CreateObject<Node> (1); // <------ for MPI,                   "   _2          2
  nodes.Add (node1);
  nodes.Add (node2);

  InternetStackHelper stack;
  stack.Install (nodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  if (0 == systemId)
    {
      dce.SetBinary ("udp-server");
      dce.ResetArguments ();
      apps = dce.Install (node1);
      apps.Start (Seconds (4.0));
    }

  if (1 == systemId)
    {
      dce.SetBinary ("udp-client");
      dce.ResetArguments ();
      dce.AddArgument ("10.1.1.1");
      apps = dce.Install (node2);
      apps.Start (Seconds (4.5));
    }


  Simulator::Stop (Seconds (1050.0));
  Simulator::Run ();
  Simulator::Destroy ();

  //-------------------------------------------------

  // Exit the MPI execution environment
  MpiInterface::Disable ();
  return 0;

}
