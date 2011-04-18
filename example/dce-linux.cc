#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <fstream>

using namespace ns3;

#define noCSMA 1
#define noP2P 1
#define WIFI 1
#define TCP 1
#define UDP 1

static void RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("./ip");
  process.SetStackSize (1<<16);
  process.ResetArguments();
  process.ParseArguments(str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

static void AddAddress (Ptr<Node> node, Time at, const char *name, const char *address)
{
  std::ostringstream oss;
  oss << "-f inet addr add " << address << " dev " << name;
  RunIp (node, at, oss.str ());
}


int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  NetDeviceContainer devices;
#if defined(CSMA)
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
  devices = csma.Install (nodes);
#elif defined(P2P)
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  devices = p2p.Install (nodes);
#elif defined(WIFI)
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  WifiHelper wifi = WifiHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper phyChannel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper mac;
  phy.SetChannel (phyChannel.Create ());
  mac.SetType ("ns3::AdhocWifiMac");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  devices = wifi.Install (phy, mac, nodes);
#endif

  DceManagerHelper processManager;
  processManager.SetLoader ("ns3::DlmLoaderFactory");
  processManager.SetNetworkStack("ns3::LinuxSocketFdFactory",
				 "Library", StringValue ("libnet-next-2.6.so"));
  processManager.Install (nodes);

  AddAddress (nodes.Get (0), Seconds (0.1), "sim0", "10.0.0.2/8");
  RunIp (nodes.Get (0), Seconds (0.11), "link set sim0 up");

  AddAddress (nodes.Get (1), Seconds (0.1), "sim0", "10.0.0.3/8");
  RunIp (nodes.Get (1), Seconds (0.11), "link set sim0 up");

  RunIp (nodes.Get (0), Seconds (0.2), "link show");
  RunIp (nodes.Get (0), Seconds (0.3), "route show table all");
  RunIp (nodes.Get (0), Seconds (0.4), "addr list");
  RunIp (nodes.Get (1), Seconds (0.2), "link show");
  RunIp (nodes.Get (1), Seconds (0.3), "route show table all");
  RunIp (nodes.Get (1), Seconds (0.4), "addr list");

  {
    DceApplicationHelper process;
    ApplicationContainer apps;

#if defined(TCP)
    process.SetBinary ("process-tcp-server");
    process.ResetArguments ();
    process.SetStackSize (1<<16);
    apps = process.Install (nodes.Get (0));
    apps.Start (Seconds (1.0));

    process.SetBinary ("process-tcp-client");
    process.ResetArguments ();
    process.ParseArguments ("10.0.0.2");
    apps = process.Install (nodes.Get (1));
    apps.Start (Seconds (1.5));
#elif defined(UDP)
    process.SetBinary ("process-udp-server");
    process.ResetArguments ();
    process.SetStackSize (1<<16);
    apps = process.Install (nodes.Get (0));
    apps.Start (Seconds (1.0));

    process.SetBinary ("process-udp-client");
    process.ResetArguments ();
    process.ParseArguments ("10.0.0.2");
    apps = process.Install (nodes.Get (1));
    apps.Start (Seconds (1.5));
#endif
  }

#if defined(CSMA)
  csma.EnablePcapAll ("process-linux");
#elif defined(P2)
  p2p.EnablePcapAll ("process-linux");
#elif defined(WIFI)
  phy.EnablePcapAll ("process-linux");
#endif
  //std::ofstream ascii;
  //ascii.open ("process.tr");
  //PointToPointHelper::EnableAsciiAll (ascii);


  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
