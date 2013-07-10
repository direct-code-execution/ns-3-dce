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

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

void
CreateReadme ()
{
  std::ofstream osf ("/tmp/README", std::fstream::trunc);

  for (int l = 0; l < 1000; l++)
    {
      osf << "The wanted data is here :)" << std::endl;
    }

  osf.close ();
}
int
main (int argc, char *argv[])
{
  CommandLine cmd;
  bool useKernel = 0;
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);
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

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  if (!useKernel)
    {
      Ipv4StaticRoutingHelper multicast;
      // 2) Set up a default multicast route on the sender n0
      Ptr<Node> sender = nodes.Get (0);
      Ptr<NetDevice> senderIf = devices.Get (0);
      multicast.SetDefaultMulticastRoute (sender, senderIf);

      sender = nodes.Get (1);
      senderIf = devices.Get (1);
      multicast.SetDefaultMulticastRoute (sender, senderIf);
    }
  else
    {
      DceApplicationHelper process;
      ApplicationContainer a;
      process.SetBinary ("ip");
      process.SetStackSize (1 << 16);
      process.ResetArguments ();
      process.ParseArguments ("route add 224.0.0.0/4 dev sim0");
      for (int n = 0; n < 2; n++)
        {
          a = process.Install (nodes.Get (n));
          a.Start (Seconds (0.001));
        }
    }

  dceManager.Install (nodes);

  CcnClientHelper dce;
  ApplicationContainer apps, putter, getter;

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

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.1));

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (0.1));

  // Configure ./ccndc add / udp 224.0.0.204 2869
  dce.SetBinary ("ccndc");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root"); // USE TO FIND keystore under $HOME/.ccnx/.ccnx_keystore
  dce.AddArgument ("add");
  dce.AddArgument ("/");
  dce.AddArgument ("udp");
  dce.AddArgument ("225.1.2.4");
  dce.AddArgument ("1234");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.2));

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (0.2));

  dce.ResetArguments ();
//  dce.ResetEnvironment();
  dce.SetBinary ("ccnpoke");
  dce.SetStdinFile ("/tmp/README");
  CreateReadme ();
  dce.AddFile ("/tmp/README", "/tmp/README");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment ("HOME", "/root");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (1.0));

  dce.ResetArguments ();
  dce.SetBinary ("ccnpeek");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("ccnx:/LeReadme");
  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (2.0));

  pointToPoint.EnablePcapAll ("ccn-mt1", false);

  Simulator::Stop (Seconds (111.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
