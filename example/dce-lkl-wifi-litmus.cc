/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

/*
 * Litmus tests of ns-3 wifi behaviors against Linux's mac80211, running in
 * LKL on LklWifiDevice radios that share the channel with ns-3 devices.
 * Each scenario puts Linux where it shows what a real stack does, or where
 * its frames exercise an ns-3 code path:
 *
 *  promisc   ns-3 AP; Linux STA pinging it; a Linux STA and an ns-3 STA in
 *            promiscuous mode overhearing; another BSS on the channel.
 *            What each observer delivers is printed (lkl-sniff for Linux).
 *  spectrum  2.4 GHz on a spectrum channel (--channel=single|multi):
 *            --ap=1: ns-3 802.11n AP with Linux 802.11g STAs pinging it;
 *            --ap=0: Linux 802.11g mesh points only.
 *  rate      ns-3 AP (--manager) and one Linux STA, --distance apart:
 *            Linux (minstrel) sends UDP up with iperf, then the AP sends
 *            UDP down; goodputs and the rates used are printed.
 *  dense     Linux and ns-3 mesh points close together, flooding the
 *            channel, with frame capture (--captureWindow).
 *  pcap      Linux and ns-3 mesh points pinging, and a Linux radio in
 *            monitor mode writing what it receives (mon.pcap), next to
 *            the ns-3 pcap of the same radio.
 *  macchange ns-3 AP, an ns-3 STA whose address changes at runtime (as
 *            TapBridge does), a Linux STA pinging it, and a Linux STA
 *            changing its own address, up and down.
 *
 * The scenarios with an ns-3 AP need it to answer the Authentication frames
 * of Linux STAs, which ns-3 APs do not do (they skip authentication):
 * without that, Linux STAs do not associate.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/dce-module.h"
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceLklWifiLitmus");

namespace {

void
Finished (std::string name, uint16_t pid, int status)
{
  std::cout << Simulator::Now ().GetSeconds () << "s exit " << status << " pid " << pid << ": "
            << name << std::endl;
}

void
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

std::string
Ip (std::string net, uint32_t host)
{
  std::ostringstream s;
  s << net << "." << host;
  return s.str ();
}

/// Bring a Linux radio up as a STA of an SSID, with an address.
void
LinuxSta (Ptr<Node> node, std::string ssid, uint16_t freq, std::string addr)
{
  std::ostringstream connect;
  connect << "dev wlan0 connect " << ssid << " " << freq;
  Run (node, Seconds (0.5), "ip", "link set wlan0 up");
  Run (node, Seconds (0.7), "iw", connect.str ());
  Run (node, Seconds (0.8), "ip", "addr add " + addr + "/24 dev wlan0");
  Run (node, Seconds (2.9), "iw", "dev wlan0 link");
}

/// Bring a Linux radio up as a mesh point, with an address.
void
LinuxMesh (Ptr<Node> node, uint16_t freq, std::string addr)
{
  std::ostringstream join;
  join << "dev wlan0 mesh join mesh freq " << freq;
  Run (node, Seconds (0.5), "iw", "dev wlan0 set type mp");
  Run (node, Seconds (0.6), "ip", "link set wlan0 up");
  Run (node, Seconds (0.7), "iw", join.str ());
  Run (node, Seconds (0.8), "ip", "addr add " + addr + "/24 dev wlan0");
}

// What the ns-3 promiscuous observers deliver.
typedef std::tuple<std::string, NetDevice::PacketType, Mac48Address, Mac48Address, uint16_t> PromiscKey;
std::map<PromiscKey, uint64_t> g_promisc;

bool
PromiscRx (std::string name, Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
           const Address &from, const Address &to, NetDevice::PacketType type)
{
  g_promisc[std::make_tuple (name, type, Mac48Address::ConvertFrom (from),
                             Mac48Address::ConvertFrom (to), protocol)]++;
  return true;
}

const char *
TypeName (NetDevice::PacketType type)
{
  switch (type)
    {
    case NetDevice::PACKET_HOST: return "host";
    case NetDevice::PACKET_BROADCAST: return "broadcast";
    case NetDevice::PACKET_MULTICAST: return "multicast";
    case NetDevice::PACKET_OTHERHOST: return "otherhost";
    default: return "other";
    }
}

void
PrintPromisc (void)
{
  for (auto &i : g_promisc)
    {
      std::cout << "ns3-promisc " << std::get<0> (i.first) << " type="
                << TypeName (std::get<1> (i.first)) << " src=" << std::get<2> (i.first)
                << " dst=" << std::get<3> (i.first) << " proto=0x" << std::hex
                << std::get<4> (i.first) << std::dec << " frames=" << i.second << std::endl;
    }
}

// Data frames put on the air, by transmitter and rate.
std::map<std::pair<std::string, std::string>, uint64_t> g_txRates;

void
TxPsdu (std::string name, WifiConstPsduMap psdus, WifiTxVector txVector, double power)
{
  for (auto &i : psdus)
    {
      if (i.second->GetHeader (0).IsData () && !i.second->GetAddr1 ().IsGroup ())
        {
          g_txRates[std::make_pair (name, txVector.GetMode ().GetUniqueName ())]++;
        }
    }
}

std::map<std::string, uint64_t> g_rxDrops;

void
RxDrop (Ptr<const Packet> packet, WifiPhyRxfailureReason reason)
{
  std::ostringstream s;
  s << reason;
  g_rxDrops[s.str ()]++;
}

void
ChangeAddress (Ptr<NetDevice> device, Mac48Address address)
{
  std::cout << Simulator::Now ().GetSeconds () << "s ns-3 device " << device->GetAddress ()
            << " -> " << address << std::endl;
  device->SetAddress (address);
}

std::map<Mac48Address, uint64_t> g_txByTa;
std::map<Mac48Address, uint64_t> g_failByTa;

void
MacTxTa (WifiConstPsduMap psdus, WifiTxVector txVector, double power)
{
  for (auto &i : psdus)
    {
      if (i.second->GetHeader (0).IsData ())
        {
          g_txByTa[i.second->GetHeader (0).GetAddr2 ()]++;
        }
    }
}

void
TxFailed (Mac48Address to)
{
  g_failByTa[to]++;
}

void
WatchPhy (Ptr<WifiPhy> phy, std::string name)
{
  phy->TraceConnectWithoutContext ("PhyTxPsduBegin", MakeBoundCallback (&TxPsdu, name));
  phy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
}

void
PrintStats (void)
{
  for (auto &i : g_txRates)
    {
      std::cout << "tx-rate " << i.first.first << " " << i.first.second << " " << i.second
                << std::endl;
    }
  for (auto &i : g_rxDrops)
    {
      std::cout << "rx-drop " << i.first << " " << i.second << std::endl;
    }
}

} // namespace

int
main (int argc, char *argv[])
{
  std::string scenario = "promisc";
  std::string channelType = "single";
  std::string manager = "ns3::ThompsonSamplingWifiManager";
  bool ap = true;
  double distance = 10;
  double stop = 20;
  uint32_t nLinux = 4;
  uint32_t nNs3 = 4;
  double captureWindow = 16;
  CommandLine cmd;
  cmd.AddValue ("scenario", "promisc, spectrum, rate, dense, pcap or macchange", scenario);
  cmd.AddValue ("channel", "spectrum: single or multi (model spectrum channel)", channelType);
  cmd.AddValue ("ap", "spectrum: an ns-3 802.11n AP (1) or Linux mesh points only (0)", ap);
  cmd.AddValue ("manager", "rate: the rate manager of the ns-3 AP", manager);
  cmd.AddValue ("distance", "rate: distance of the Linux STA to the AP (m)", distance);
  cmd.AddValue ("linux", "dense: Linux mesh points", nLinux);
  cmd.AddValue ("ns3", "dense: ns-3 mesh points", nNs3);
  cmd.AddValue ("captureWindow", "dense: frame capture window (us)", captureWindow);
  cmd.AddValue ("stop", "Simulation time (s)", stop);
  cmd.Parse (argc, argv);
  // Linux drops packets whose checksums ns-3 left out
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  DceManagerHelper dceManager;
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblkl.so"));
  LklWifiHelper lklWifi;
  InternetStackHelper internet;
  Ipv4AddressHelper address;
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator> ();
  mobility.SetPositionAllocator (positions);
  YansWifiChannelHelper yansChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper yans;
  yans.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  if (scenario == "promisc" || scenario == "macchange")
    {
      // BSS "litmus": AP, Linux STAs 0 and 1, ns-3 STA. promisc: BSS
      // "other" (AP, STA) on the same channel.
      bool promisc = scenario == "promisc";
      yans.SetChannel (yansChannel.Create ());
      yans.Set ("ChannelSettings", StringValue ("{36, 20, BAND_5GHZ, 0}"));
      NodeContainer apNode (1), linuxNodes (2), staNode (1), otherNodes (promisc ? 2 : 0);
      NodeContainer all (apNode, linuxNodes, staNode, otherNodes);
      for (uint32_t i = 0; i < all.GetN (); i++)
        {
          positions->Add (Vector (5.0 * i, 0, 0));
        }
      mobility.Install (all);

      WifiHelper wifi;
      wifi.SetStandard (WIFI_STANDARD_80211a);
      WifiMacHelper mac;
      mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (Ssid ("litmus")));
      NetDeviceContainer apDevice = wifi.Install (yans, mac, apNode);
      mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (Ssid ("litmus")));
      NetDeviceContainer staDevice = wifi.Install (yans, mac, staNode);
      NetDeviceContainer otherDevices;
      if (promisc)
        {
          mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (Ssid ("other")));
          otherDevices.Add (wifi.Install (yans, mac, otherNodes.Get (0)));
          mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (Ssid ("other")));
          otherDevices.Add (wifi.Install (yans, mac, otherNodes.Get (1)));
        }
      dceManager.Install (linuxNodes);
      NetDeviceContainer linuxDevices = lklWifi.Install (yans, linuxNodes);
      LklWifiHelper::EnablePcap (yans, "litmus-" + scenario, linuxDevices);
      LklWifiHelper::AssignStreams (linuxDevices, 100);
      yans.EnablePcap ("litmus-" + scenario, NodeContainer (apNode, staNode, otherNodes), true);

      internet.Install (NodeContainer (apNode, staNode, otherNodes));
      address.SetBase ("10.1.1.0", "255.255.255.0", "0.0.0.100");
      address.Assign (apDevice); // .100
      address.Assign (staDevice); // .101
      LinuxSta (linuxNodes.Get (0), "litmus", 5180, "10.1.1.1");
      LinuxSta (linuxNodes.Get (1), "litmus", 5180, "10.1.1.2");

      if (promisc)
        {
          address.SetBase ("10.2.2.0", "255.255.255.0", "0.0.0.100");
          address.Assign (otherDevices);
          // Linux STA 1 and the ns-3 STA overhear Linux STA 0 and the AP.
          Run (linuxNodes.Get (1), Seconds (1.0), "ip", "link set wlan0 promisc on");
          Run (linuxNodes.Get (1), Seconds (3.0), "lkl-sniff",
               "wlan0 " + std::to_string (stop - 4));
          staDevice.Get (0)->SetPromiscReceiveCallback (MakeBoundCallback (&PromiscRx, "ns3-sta"));
          Run (linuxNodes.Get (0), Seconds (4.0), "ping", "-c 20 -i 0.2 10.1.1.100");
          // the other BSS
          PingHelper ping (Ipv4Address ("10.2.2.100"));
          ping.SetAttribute ("Count", UintegerValue (20));
          ping.SetAttribute ("Interval", TimeValue (Seconds (0.2)));
          ApplicationContainer pings = ping.Install (otherNodes.Get (1));
          pings.Start (Seconds (4.0));
          Simulator::Schedule (Seconds (stop - 0.5), &PrintPromisc);
        }
      else
        {
          // Linux STA 0 pings the ns-3 STA throughout; its address changes
          // at 6 s, as TapBridge (UseLocal) sets it to that of its tap.
          Run (linuxNodes.Get (0), Seconds (3.0), "ping",
               "-c " + std::to_string ((int)((stop - 4) / 0.2)) + " -i 0.2 10.1.1.101");
          Simulator::Schedule (Seconds (6.0), &ChangeAddress, staDevice.Get (0),
                               Mac48Address ("02:00:00:00:01:01"));
          Ptr<WifiMac> staMac = DynamicCast<WifiNetDevice> (staDevice.Get (0))->GetMac ();
          DynamicCast<WifiNetDevice> (staDevice.Get (0))->GetPhy ()->TraceConnectWithoutContext (
            "PhyTxPsduBegin", MakeCallback (&MacTxTa));
          staMac->GetWifiRemoteStationManager ()->TraceConnectWithoutContext (
            "MacTxDataFailed", MakeCallback (&TxFailed));
          // What Linux does: change the address while up, then down and up.
          Ptr<Node> l1 = linuxNodes.Get (1);
          Run (l1, Seconds (6.0), "ip", "link set wlan0 address 02:00:00:00:02:02");
          Run (l1, Seconds (6.5), "ip", "link show wlan0");
          Run (l1, Seconds (7.0), "ip", "link set wlan0 down");
          Run (l1, Seconds (7.1), "ip", "link set wlan0 address 02:00:00:00:02:02");
          Run (l1, Seconds (7.2), "ip", "link set wlan0 up");
          Run (l1, Seconds (7.3), "iw", "dev wlan0 connect litmus 5180");
          Run (l1, Seconds (7.4), "ip", "addr add 10.1.1.2/24 dev wlan0");
          Run (l1, Seconds (9.0), "iw", "dev wlan0 link");
          Run (l1, Seconds (9.5), "ping", "-c 10 -i 0.2 10.1.1.100");
          Simulator::Schedule (Seconds (stop - 0.5), [] () {
            for (auto &i : g_txByTa)
              {
                std::cout << "ns3-sta mac-tx ta=" << i.first << " frames=" << i.second
                          << std::endl;
              }
            for (auto &i : g_failByTa)
              {
                std::cout << "ns3-sta tx-failed to=" << i.first << " frames=" << i.second
                          << std::endl;
              }
          });
        }
    }
  else if (scenario == "spectrum")
    {
      Ptr<SpectrumChannel> channel;
      if (channelType == "single")
        {
          channel = CreateObject<SingleModelSpectrumChannel> ();
        }
      else
        {
          channel = CreateObject<MultiModelSpectrumChannel> ();
        }
      channel->AddPropagationLossModel (CreateObject<LogDistancePropagationLossModel> ());
      channel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());
      SpectrumWifiPhyHelper spectrum;
      spectrum.SetChannel (channel);
      spectrum.Set ("ChannelSettings", StringValue ("{1, 20, BAND_2_4GHZ, 0}"));
      spectrum.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      lklWifi.SetStandard (WIFI_STANDARD_80211g);

      NodeContainer apNode (ap ? 1 : 0), linuxNodes (2);
      NodeContainer all (apNode, linuxNodes);
      for (uint32_t i = 0; i < all.GetN (); i++)
        {
          positions->Add (Vector (5.0 * i, 0, 0));
        }
      mobility.Install (all);
      dceManager.Install (linuxNodes);
      NetDeviceContainer linuxDevices = lklWifi.Install (spectrum, linuxNodes);
      LklWifiHelper::EnablePcap (spectrum, "litmus-spectrum", linuxDevices);
      LklWifiHelper::AssignStreams (linuxDevices, 100);
      for (uint32_t i = 0; i < linuxDevices.GetN (); i++)
        {
          WatchPhy (DynamicCast<LklWifiDevice> (linuxDevices.Get (i))->GetPhy (),
                    "linux-" + std::to_string (i));
        }
      if (ap)
        {
          WifiHelper wifi;
          wifi.SetStandard (WIFI_STANDARD_80211n);
          WifiMacHelper mac;
          mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (Ssid ("litmus")));
          NetDeviceContainer apDevice = wifi.Install (spectrum, mac, apNode);
          spectrum.EnablePcap ("litmus-spectrum", apDevice, true);
          WatchPhy (DynamicCast<WifiNetDevice> (apDevice.Get (0))->GetPhy (), "ns3-ap");
          internet.Install (apNode);
          address.SetBase ("10.1.1.0", "255.255.255.0", "0.0.0.100");
          address.Assign (apDevice);
          LinuxSta (linuxNodes.Get (0), "litmus", 2412, "10.1.1.1");
          LinuxSta (linuxNodes.Get (1), "litmus", 2412, "10.1.1.2");
          Run (linuxNodes.Get (0), Seconds (4.0), "ping", "-c 10 -i 0.2 10.1.1.100");
          Run (linuxNodes.Get (1), Seconds (4.0), "ping", "-c 10 -i 0.2 10.1.1.100");
        }
      else
        {
          LinuxMesh (linuxNodes.Get (0), 2412, "10.1.1.1");
          LinuxMesh (linuxNodes.Get (1), 2412, "10.1.1.2");
          Run (linuxNodes.Get (0), Seconds (5.0), "ping", "-c 10 -i 0.2 10.1.1.2");
        }
      Simulator::Schedule (Seconds (stop - 0.5), &PrintStats);
    }
  else if (scenario == "rate")
    {
      yans.SetChannel (yansChannel.Create ());
      yans.Set ("ChannelSettings", StringValue ("{36, 20, BAND_5GHZ, 0}"));
      NodeContainer apNode (1), linuxNodes (1);
      positions->Add (Vector (0, 0, 0));
      positions->Add (Vector (distance, 0, 0));
      mobility.Install (NodeContainer (apNode, linuxNodes));

      WifiHelper wifi;
      wifi.SetStandard (WIFI_STANDARD_80211a);
      wifi.SetRemoteStationManager (manager);
      WifiMacHelper mac;
      mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (Ssid ("litmus")));
      NetDeviceContainer apDevice = wifi.Install (yans, mac, apNode);
      wifi.AssignStreams (apDevice, 300);
      yans.EnablePcap ("litmus-rate", apDevice, true);
      WatchPhy (DynamicCast<WifiNetDevice> (apDevice.Get (0))->GetPhy (), "ns3-ap");
      dceManager.Install (linuxNodes);
      NetDeviceContainer linuxDevices = lklWifi.Install (yans, linuxNodes);
      LklWifiHelper::AssignStreams (linuxDevices, 100);
      WatchPhy (DynamicCast<LklWifiDevice> (linuxDevices.Get (0))->GetPhy (), "linux-sta");
      internet.Install (apNode);
      address.SetBase ("10.1.1.0", "255.255.255.0", "0.0.0.100");
      address.Assign (apDevice);
      LinuxSta (linuxNodes.Get (0), "litmus", 5180, "10.1.1.1");

      // Up: Linux (minstrel) from 4 to 12 s, to a sink on the AP.
      PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5001));
      Ptr<PacketSink> upSink = DynamicCast<PacketSink> (sink.Install (apNode).Get (0));
      Run (linuxNodes.Get (0), Seconds (4.0), "iperf", "-c 10.1.1.100 -u -b 40M -l 1400 -t 8");
      // Down: the AP (the manager) from 13 to 21 s; lkl-sniff counts.
      OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address ("10.1.1.1"), 9));
      onoff.SetConstantRate (DataRate ("40Mbps"), 1400);
      ApplicationContainer down = onoff.Install (apNode);
      down.Start (Seconds (13.0));
      down.Stop (Seconds (21.0));
      Run (linuxNodes.Get (0), Seconds (13.0), "lkl-sniff", "wlan0 8");
      Run (linuxNodes.Get (0), Seconds (6.0), "iw", "dev wlan0 station dump");
      Run (linuxNodes.Get (0), Seconds (11.0), "iw", "dev wlan0 station dump");
      uint64_t *upBytes = new uint64_t (0);
      Simulator::Schedule (Seconds (4.0), [upSink, upBytes] () { *upBytes = upSink->GetTotalRx (); });
      Simulator::Schedule (Seconds (12.0), [upSink, upBytes] () {
        std::cout << "goodput up linux-minstrel " << (upSink->GetTotalRx () - *upBytes) * 8 / 8e6
                  << " Mbps" << std::endl;
      });
      Simulator::Schedule (Seconds (stop - 0.5), &PrintStats);
    }
  else if (scenario == "dense" || scenario == "pcap")
    {
      bool dense = scenario == "dense";
      yans.SetChannel (yansChannel.Create ());
      yans.Set ("ChannelSettings", StringValue ("{100, 20, BAND_5GHZ, 0}"));
      if (dense)
        {
          yans.SetFrameCaptureModel ("ns3::SimpleFrameCaptureModel", "Margin", DoubleValue (5),
                                     "CaptureWindow", TimeValue (MicroSeconds (captureWindow)));
        }
      if (!dense)
        {
          nLinux = 2;
          nNs3 = 2;
        }
      NodeContainer linuxNodes (nLinux), ns3Nodes (nNs3), monitorNode (dense ? 0 : 1);
      NodeContainer all (linuxNodes, ns3Nodes, monitorNode);
      Ptr<UniformRandomVariable> u = CreateObject<UniformRandomVariable> ();
      for (uint32_t i = 0; i < all.GetN (); i++)
        {
          // dense: spread so that some pairs are hidden or much stronger
          positions->Add (dense ? Vector (u->GetValue (0, 80), u->GetValue (0, 20), 0)
                                : Vector (20.0 * i, 0, 0));
        }
      mobility.Install (all);
      dceManager.Install (NodeContainer (linuxNodes, monitorNode));
      NetDeviceContainer linuxDevices = lklWifi.Install (yans, NodeContainer (linuxNodes, monitorNode));
      LklWifiHelper::EnablePcap (yans, "litmus-" + scenario, linuxDevices);
      LklWifiHelper::AssignStreams (linuxDevices, 100);
      MeshHelper mesh = MeshHelper::Default ();
      mesh.SetStackInstaller ("ns3::Dot11sStack");
      NetDeviceContainer ns3Devices = mesh.Install (yans, ns3Nodes);
      mesh.AssignStreams (ns3Devices, 200);
      yans.EnablePcap ("litmus-" + scenario, ns3Nodes, true);
      for (uint32_t i = 0; i < nLinux; i++)
        {
          WatchPhy (DynamicCast<LklWifiDevice> (linuxDevices.Get (i))->GetPhy (), "linux");
        }
      internet.Install (ns3Nodes);
      std::ostringstream base;
      base << "0.0.0." << nLinux + 1;
      address.SetBase ("10.1.1.0", "255.255.255.0", base.str ().c_str ());
      address.Assign (ns3Devices);
      for (uint32_t i = 0; i < nLinux; i++)
        {
          LinuxMesh (linuxNodes.Get (i), 5500, Ip ("10.1.1", i + 1));
        }
      if (dense)
        {
          for (uint32_t i = 0; i < nLinux; i++)
            {
              // unicast to the next node, and broadcasts
              Run (linuxNodes.Get (i), Seconds (3.0), "ping",
                   "-c 200 -i 0.02 -s 1000 " + Ip ("10.1.1", (i + 1) % (nLinux + nNs3) + 1));
              Run (linuxNodes.Get (i), Seconds (3.0), "ping", "-b -c 200 -i 0.03 -s 600 10.1.1.255");
            }
          for (uint32_t i = 0; i < nNs3; i++)
            {
              OnOffHelper onoff ("ns3::UdpSocketFactory",
                                 InetSocketAddress (Ipv4Address ("10.1.1.255"), 9));
              onoff.SetConstantRate (DataRate ("2Mbps"), 800);
              ApplicationContainer app = onoff.Install (ns3Nodes.Get (i));
              app.Start (Seconds (3.0 + 0.01 * i));
              app.Stop (Seconds (stop - 1));
            }
        }
      else
        {
          for (uint32_t i = 1; i < nLinux + nNs3; i++)
            {
              Run (linuxNodes.Get (0), Seconds (5.0 + 4 * i), "ping",
                   "-c 5 " + Ip ("10.1.1", i + 1));
            }
          Ptr<Node> monitor = monitorNode.Get (0);
          Run (monitor, Seconds (0.5), "iw", "dev wlan0 set type monitor");
          Run (monitor, Seconds (0.6), "ip", "link set wlan0 up");
          Run (monitor, Seconds (0.7), "iw", "dev wlan0 set freq 5500");
          Run (monitor, Seconds (1.0), "lkl-sniff", "wlan0 " + std::to_string (stop - 2) + " mon.pcap");
        }
      Simulator::Schedule (Seconds (stop - 0.5), &PrintStats);
    }
  else
    {
      NS_FATAL_ERROR ("Unknown scenario " << scenario);
    }

  Simulator::Stop (Seconds (stop));
  Simulator::Run ();
  std::cout << "litmus " << scenario << " completed at " << Simulator::Now ().GetSeconds () << "s"
            << std::endl;
  Simulator::Destroy ();
  return 0;
}
