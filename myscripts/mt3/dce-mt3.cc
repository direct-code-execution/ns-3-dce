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
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/wifi-module.h"
#include "math.h"

using namespace ns3;

void
CreateReadme ()
{
  std::ofstream osf ("/tmp/README", std::fstream::trunc);

  osf << "The wanted data is here :)" ;

  osf.close ();
}
static void
SetPosition (Ptr<Node> node, Vector position)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  mobility->SetPosition (position);
}

float r = 50.0;
static void
AdvancePosition (Ptr<Node> node)
{
  // A circle with a turn each 1 s

  Time t = Simulator::Now ();

  int frac = (t.GetMilliSeconds () % 1000);

  float angle =  2.0 * M_PI * frac / 1000.0;

  SetPosition (node, Vector (r * cos (angle), r * sin (angle), 0));

  Simulator::Schedule (MilliSeconds (10), &AdvancePosition, node);

}
int
main (int argc, char *argv[])
{
  CommandLine cmd;
  bool useKernel = 0;
  cmd.AddValue ("r", "Radius of the circle default (50) ", r);
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.Parse (argc, argv);
  NodeContainer nodes;
  nodes.Create (4);

  MobilityHelper mob;

  mob.Install (nodes);

  SetPosition (nodes.Get (0), Vector (0,0,0));
  SetPosition (nodes.Get (1), Vector (r / 2,0,0));
  SetPosition (nodes.Get (2), Vector (-r / 2,0,0));
  SetPosition (nodes.Get (3), Vector (r,0,0));

  Simulator::Schedule (Seconds (0.0), &AdvancePosition, nodes.Get (3));
  DceManagerHelper dceManager;

  if (!useKernel)
    {
      InternetStackHelper stack;
      stack.Install (nodes);
    }
  else
    {
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      LinuxStackHelper stack;
      stack.Install (nodes);
    }

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes.Get (0), nodes.Get (1));
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  devices = pointToPoint.Install (nodes.Get (0), nodes.Get (2));
  address.SetBase ("10.1.2.0", "255.255.255.0");
  interfaces = address.Assign (devices);

  NodeContainer dt;

  dt.Add (nodes.Get (2));
  dt.Add (nodes.Get (3));

  WifiHelper wifi = WifiHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper phyChannel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper mac;
  phy.SetChannel (phyChannel.Create ());
  mac.SetType ("ns3::AdhocWifiMac");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  devices = wifi.Install (phy, mac, dt);
  address.SetBase ("10.1.3.0", "255.255.255.0");
  interfaces = address.Assign (devices);

  NodeContainer ut;

  ut.Add (nodes.Get (1));
  ut.Add (nodes.Get (3));

  devices = wifi.Install (phy, mac, ut);
  address.SetBase ("10.1.4.0", "255.255.255.0");
  interfaces = address.Assign (devices);
  dceManager.Install (nodes);

  if (!useKernel)
    {
      Ipv4StaticRoutingHelper multicast;
      // 2) Set up a default multicast route on the sender n0
      Ptr<Node> sender = nodes.Get (3);
      Ptr<NetDevice> senderIf = sender->GetDevice (1);
      multicast.SetDefaultMulticastRoute (sender, senderIf);
    }
  else
    {
      DceApplicationHelper process;
      ApplicationContainer a;
      process.SetBinary ("ip");
      process.SetStackSize (1 << 16);
      process.ResetArguments ();
      process.ParseArguments ("route add 224.0.0.0/4 dev sim1");
      for (int n = 0; n < 4; n++)
        {
          a = process.Install (nodes.Get (n));
          a.Start (Seconds (0.001));
        }
    }

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Sender
  dce.SetBinary ("mcsend");
  dce.ResetArguments ();
  dce.AddArgument ("225.1.2.4");
  dce.AddArgument ("1234");
  apps = dce.Install (nodes.Get (3));
  apps.Start (Seconds (0.2));

  // Receiver
  dce.SetBinary ("mcreceive");
  dce.ResetArguments ();
  dce.AddArgument ("225.1.2.4");
  dce.AddArgument ("1234");
  apps = dce.Install (nodes.Get (2));
  apps.Start (Seconds (0.1));

  // Receiver
  dce.SetBinary ("mcreceive");
  dce.ResetArguments ();
  dce.AddArgument ("225.1.2.4");
  dce.AddArgument ("1234");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (0.1));

  // Create the animation object and configure for specified output
  AnimationInterface anim ("dce-mt3.xml");

  Simulator::Stop (Seconds (105.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
