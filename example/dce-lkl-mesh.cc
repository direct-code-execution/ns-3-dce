/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

/*
 * Linux 802.11s mesh points (mac80211 in LKL, on LklWifiDevice radios) and
 * ns-3 mesh points (MeshHelper, dot11s) on one channel, in a line:
 *
 *   linux-0 ... linux-(L-1)   ns3-0 ... ns3-(N-1)
 *
 * with neighbors <distance> apart, so that a path between the two ends
 * needs several hops. Linux joins mesh "mesh" (the mesh ID of ns-3's
 * dot11s) on 5500 MHz (channel 100, where MeshHelper puts its interfaces),
 * open (no SAE: ns-3 has none). The first Linux node then pings every other node.
 *
 * The pcap files (lkl-mesh-<node>-<device>.pcap, radiotap) have every
 * frame of both stacks: compare them field by field, e.g. with tshark,
 * against what each stack expects of the other. Frames of Linux that ns-3
 * serializes differently are reported as "header mismatch".
 *
 * With --iw=0, the radios are only brought up and listed (no iw needed).
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/dce-module.h"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace ns3;

static void
Finished (std::string name, uint16_t pid, int status)
{
  std::cout << Simulator::Now ().GetSeconds () << "s " << name << " exited with " << status
            << std::endl;
}

static void
Run (Ptr<Node> node, Time at, std::string binary, std::string args)
{
  DceApplicationHelper dce;
  dce.SetBinary (binary);
  dce.SetStackSize (1 << 16);
  dce.ResetArguments ();
  dce.ParseArguments (args.c_str ());
  std::ostringstream name;
  name << "node " << node->GetId () << ": " << binary << " " << args;
  dce.SetFinishedCallback (MakeBoundCallback (&Finished, name.str ()));
  ApplicationContainer apps = dce.Install (node);
  apps.Start (at);
}

static void
HeaderMismatch (std::string context, const std::vector<uint8_t> &frame, Ptr<const Packet> sent)
{
  std::ostringstream linux;
  for (uint8_t b : frame)
    {
      linux << std::hex << std::setw (2) << std::setfill ('0') << +b;
    }
  std::cout << Simulator::Now ().GetSeconds () << "s header mismatch " << context
            << "\n  linux " << linux.str () << "\n  ns-3  " << *sent << std::endl;
}

int
main (int argc, char *argv[])
{
  uint32_t nLinux = 2;
  uint32_t nNs3 = 2;
  double distance = 50;
  double stop = 30;
  bool iw = true;
  CommandLine cmd;
  cmd.AddValue ("linux", "Linux mesh points", nLinux);
  cmd.AddValue ("ns3", "ns-3 mesh points", nNs3);
  cmd.AddValue ("distance", "Distance between neighbors (m)", distance);
  cmd.AddValue ("stop", "Simulation time (s)", stop);
  cmd.AddValue ("iw", "Join the mesh with iw (0: only bring the radios up)", iw);
  cmd.Parse (argc, argv);
  // Linux drops packets whose checksums ns-3 left out
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  NodeContainer linuxNodes;
  linuxNodes.Create (nLinux);
  NodeContainer ns3Nodes;
  ns3Nodes.Create (nNs3);
  NodeContainer all (linuxNodes, ns3Nodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0), "MinY", DoubleValue (0),
                                 "DeltaX", DoubleValue (distance),
                                 "GridWidth", UintegerValue (all.GetN ()),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (all);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
  // MeshHelper's channel (ZERO_CHANNEL policy), for the Linux radios too.
  phy.Set ("ChannelSettings", StringValue ("{100, 20, BAND_5GHZ, 0}"));
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  DceManagerHelper dceManager;
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblkl.so"));
  dceManager.Install (linuxNodes);
  LklWifiHelper lklWifi;
  NetDeviceContainer linuxDevices = lklWifi.Install (phy, linuxNodes);
  LklWifiHelper::EnablePcap (phy, "lkl-mesh", linuxDevices);
  LklWifiHelper::AssignStreams (linuxDevices, 100);

  MeshHelper mesh = MeshHelper::Default ();
  mesh.SetStackInstaller ("ns3::Dot11sStack");
  NetDeviceContainer ns3Devices = mesh.Install (phy, ns3Nodes);
  mesh.AssignStreams (ns3Devices, 200);
  phy.EnablePcap ("lkl-mesh", ns3Nodes, true);

  InternetStackHelper internet;
  internet.Install (ns3Nodes);
  Ipv4AddressHelper address;
  std::ostringstream base; // after the addresses of the Linux nodes
  base << "0.0.0." << nLinux + 1;
  address.SetBase ("10.1.1.0", "255.255.255.0", base.str ().c_str ());
  address.Assign (ns3Devices);

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::LklWifiDevice/HeaderMismatch",
                   MakeCallback (&HeaderMismatch));

  for (uint32_t i = 0; i < nLinux; i++)
    {
      Ptr<Node> node = linuxNodes.Get (i);
      std::ostringstream addr;
      addr << "addr add 10.1.1." << i + 1 << "/24 dev wlan0";
      if (iw)
        {
          Run (node, Seconds (0.5), "iw", "dev wlan0 set type mp");
          Run (node, Seconds (0.6), "ip", "link set wlan0 up");
          Run (node, Seconds (0.7), "iw", "dev wlan0 mesh join mesh freq 5500");
          Run (node, Seconds (0.8), "ip", addr.str ());
          Run (node, Seconds (stop - 1), "iw", "dev wlan0 station dump");
          Run (node, Seconds (stop - 1), "iw", "dev wlan0 mpath dump");
        }
      else
        {
          Run (node, Seconds (0.6), "ip", "link set wlan0 up");
          Run (node, Seconds (0.8), "ip", addr.str ());
          Run (node, Seconds (1.0), "ip", "link show");
        }
    }
  if (iw)
    {
      for (uint32_t i = 1; i < all.GetN (); i++)
        {
          std::ostringstream args;
          args << "-c 5 10.1.1." << i + 1;
          Run (linuxNodes.Get (0), Seconds (5 + 6 * i), "ping", args.str ());
        }
    }

  Simulator::Stop (Seconds (stop));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
