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

// This script is derived from the example of usage of EMU of NS-3 named emu-ping.cc
// you can find the original script in directory under ns-3 sources : src/emu/examples
//
// Allow ns-3 to ping a real host somewhere, using emulation mode
//
//   ------------
//   |  node n0 |
//   |          |
//   |  ---     |
//   | |   |    |
//   | |emu|    |
//   | |   |    |
//   |  ---     |
//   |   |      |
//   ----|-------
//       |
//     (device on host system, set to promiscuous mode)
//       |
//      --------- (Internet) -------
//
// To use this example:
//  1) You need to decide on a physical device on your real system, and either
//     overwrite the hard-configured device name below (eth0) or pass this
//     device name in as a command-line argument
//  2) The host device must be set to promiscuous mode
//     (e.g. "sudo ifconfig eth0 promisc")
//  3) Be aware that ns-3 will generate a fake mac address, and that in
//     some enterprise networks, this may be considered bad form to be
//     sending packets out of your device with "unauthorized" mac addresses
//  4) You will need to assign an IP address to the ns-3 simulation node that
//     is consistent with the subnet that is active on the host device's link.
//     That is, you will have to assign an IP address to the ns-3 node as if
//     it were on your real subnet.  Search for "Ipv4Address localIp" and
//     replace the string "1.2.3.4" with a valid IP address.
//  5) You will need to configure a default route in the ns-3 node to tell it
//     how to get off of your subnet. One thing you could do is a
//     'netstat -rn' command and find the IP address of the default gateway
//     on your host.  Search for "Ipv4Address gateway" and replace the string
//     "1.2.3.4" string with the gateway IP address.
#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/dce-module.h"
#include "misc-tools.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DcePingEmulationExample");

int
main (int argc, char *argv[])
{
  NS_LOG_INFO ("DCE Ping Emulation Example");

  std::string deviceName ("eth0");
  std::string remote ("208.77.188.166"); // example.com
  std::string local ("138.96.196.50");
  std::string sLocalMask ("255.255.240.0");
  //
  // Allow the user to override any of the defaults at run-time, via
  // command-line arguments
  //
  CommandLine cmd;
  cmd.AddValue ("deviceName", "Device name", deviceName);
  cmd.AddValue ("remote", "Remote IP address (dotted decimal only please)", remote);
  cmd.AddValue ("local", "Local IP address (dotted decimal only please)", local);
  cmd.AddValue ("localmask", "Local mask address (dotted decimal only please)", sLocalMask);
  cmd.Parse (argc, argv);

  Ipv4Address remoteIp (remote.c_str ());
  Ipv4Address localIp (local.c_str ());
  NS_ABORT_MSG_IF (localIp == "1.2.3.4", "You must change the local IP address before running this example");

  Ipv4Mask localMask (sLocalMask.c_str ());

  //
  // Since we are using a real piece of hardware we need to use the realtime
  // simulator.
  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

  //
  // Since we are going to be talking to real-world machines, we need to enable
  // calculation of checksums in our protocols.
  //
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // In such a simple topology, the use of the helper API can be a hindrance
  // so we drop down into the low level API and do it manually.
  //
  // First we need a single node.
  //
  NS_LOG_INFO ("Create Node");
  Ptr<Node> node = CreateObject<Node> ();

  //
  // Create an emu device, allocate a MAC address and point the device to the
  // Linux device name.  The device needs a transmit queueing discipline so
  // create a droptail queue and give it to the device.  Finally, "install"
  // the device into the node.
  //
  // Do understand that the ns-3 allocated MAC address will be sent out over
  // your network since the emu net device will spoof it.  By default, this
  // address will have an Organizationally Unique Identifier (OUI) of zero.
  // The Internet Assigned Number Authority IANA
  //
  //  http://www.iana.org/assignments/ethernet-numbers
  //
  // reports that this OUI is unassigned, and so should not conflict with
  // real hardware on your net.  It may raise all kinds of red flags in a
  // real environment to have packets from a device with an obviously bogus
  // OUI flying around.  Be aware.
  //
  NS_LOG_INFO ("Create Device");
  EmuFdNetDeviceHelper emu;
  emu.SetDeviceName (deviceName);
  NetDeviceContainer devices = emu.Install (node);
  Ptr<NetDevice> device = devices.Get (0);
  device->SetAttribute ("Address", Mac48AddressValue ("00:00:00:00:00:02"));

  // Ptr<Queue> queue = CreateObject<DropTailQueue> ();
  // device->SetQueue (queue);
  // node->AddDevice (device);

  //
  // Add a default internet stack to the node.  This gets us the ns-3 versions
  // of ARP, IPv4, ICMP, UDP and TCP.
  //
  NS_LOG_INFO ("Add Internet Stack");
  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (node);

  NS_LOG_INFO ("Create IPv4 Interface");
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  uint32_t interface = ipv4->AddInterface (device);
  Ipv4InterfaceAddress address = Ipv4InterfaceAddress (localIp, localMask);
  ipv4->AddAddress (interface, address);
  ipv4->SetMetric (interface, 1);
  ipv4->SetUp (interface);

  //
  // When the ping application sends its ICMP packet, it will happily send it
  // down the ns-3 protocol stack.  We set the IP address of the destination
  // to the address corresponding to example.com above.  This address is off
  // our local network so we have got to provide some kind of default route
  // to ns-3 to be able to get that ICMP packet forwarded off of our network.
  //
  // You have got to provide an IP address of a real host that you can send
  // real packets to and have them forwarded off of your local network.  One
  // thing you could do is a 'netstat -rn' command and find the IP address of
  // the default gateway on your host and add it below, replacing the
  // "1.2.3.4" string.
  //
  Ipv4Address gateway ("138.96.192.250");
  NS_ABORT_MSG_IF (gateway == "1.2.3.4", "You must change the gateway IP address before running this example");

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
  staticRouting->SetDefaultRoute (gateway, interface);

  DceManagerHelper dceManager;

  dceManager.Install (node);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch ping on node 0
  dce.SetBinary ("ping");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c 10");
  dce.AddArgument ("-s 1000");
  dce.AddArgument (remote);

  apps = dce.Install (node);
  apps.Start (Seconds (1.0));

  //
  // Enable a promiscuous pcap trace to see what is coming and going on our device.
  //
  emu.EnablePcap ("emu-ping", device, true);

  //
  // Now, do the actual emulation.
  //
  NS_LOG_INFO ("Run Emulation.");
  Simulator::Stop (Seconds (12.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
