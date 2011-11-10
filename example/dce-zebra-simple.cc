/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UCLA
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/dce-module.h"
#include "ns3/quagga-helper.h"
#include "ns3/point-to-point-grid.h"
#include "../helper/ipv4-dce-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("quagga");

void
procExit (std::string path, uint16_t pid, int retcode)
{
  std::cout << "Process exit path=" << path << " pid=" << pid << " retcode=" << retcode << std::endl;
}

// Parameters
uint32_t nNodes = 2;
uint32_t stopTime = 60;
std::string netStack = "ns3";

void TestDisable (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node->GetId ());
  
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ipv4 != 0, "ipv4 should not be null");

  // The thing I didn't know is that interface status (isUp/isDown) and device status (isLinkUp) are two totally different things.  It is possible to set interface up and down, but there is not an easy way to alter status of device. This similar to reality: it is possible to programmatically disable interface, but to actually disable a link one needs to physically cut the cable
  ipv4->SetDown (2);
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("nNodes", "Number of Router nodes", nNodes);
  cmd.AddValue ("stopTime", "Time to stop(seconds)", stopTime);
  cmd.AddValue ("netStack", "What network stack", netStack);
  cmd.Parse (argc, argv);

  PointToPointHelper p2p;
  InternetStackHelper stack;
  Ipv4DceRoutingHelper ipv4RoutingHelper;
  
  stack.SetRoutingHelper (ipv4RoutingHelper);

  PointToPointGridHelper grid (nNodes, nNodes, p2p);
  grid.InstallStack (stack);

  grid.AssignIpv4Addresses (
                            Ipv4AddressHelper("10.1.0.0", "255.255.255.0"),
                            Ipv4AddressHelper("10.2.0.0", "255.255.255.0")
                            );

  Ptr<DceManagerHelper> dceManager = CreateObject<DceManagerHelper> ();
  dceManager->SetNetworkStack("ns3::Ns3SocketFdFactory");

  QuaggaHelper quagga;
  
  ApplicationContainer apps;

  for (int i=0; i<nNodes; i++)
    for (int j=0; j<nNodes; j++)
      {
        Ptr<Node> node = grid.GetNode (i,j);
        dceManager->Install (node);
        quagga.EnableOspf (node);
        quagga.EnableOspfDebug (node);
        quagga.EnableZebraDebug (node);
        apps.Add (quagga.Install (node));
      }

  apps.Stop (Seconds(150.0));

  Simulator::ScheduleWithContext (grid.GetNode (0,0)->GetId (),
                                  Seconds (80.0), TestDisable, grid.GetNode (0,0));

  Config::Connect ("/NodeList/*/$ns3::DceManager/Exit", MakeCallback (&procExit));

  // Trace routing tables 
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("routes.log", std::ios::out);
  ipv4RoutingHelper.PrintRoutingTableAllEvery (Seconds (10), routingStream);

  Simulator::Stop (Seconds(160.0));
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("End of experiment");

  return 0;
}
