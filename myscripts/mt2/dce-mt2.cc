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
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  /*
  Ipv4Address multicastSource ("10.1.1.1");
  Ipv4Address multicastGroup ("225.1.2.4");
*/
  Ipv4StaticRoutingHelper multicast;
  // 2) Set up a default multicast route on the sender n0
  Ptr<Node> sender = nodes.Get (0);
  Ptr<NetDevice> senderIf = devices.Get (0);
  multicast.SetDefaultMulticastRoute (sender, senderIf);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Sender
  dce.SetBinary ("mcsend");
  dce.ResetArguments ();
  dce.AddArgument ("225.1.2.4");
  dce.AddArgument ("1234");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (2));

  // Receiver
  dce.SetBinary ("mcreceive");
  dce.ResetArguments ();
  dce.AddArgument ("225.1.2.4");
  dce.AddArgument ("1234");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (1));

  Simulator::Stop (Seconds (111.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
