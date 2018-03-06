#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include <fstream>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceLinux");

static void RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1 << 16);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

void
PrintTcpFlags (std::string key, std::string value)
{
  NS_LOG_INFO (key << "=" << value);
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  char linkType = 'p'; // P2P
  bool reliable = true;

  cmd.AddValue ("linkType", "Link type: ie : C for CSMA, P for Point to Point and w for Wifi, default to P2P", linkType);
  cmd.AddValue ("reliable", "If true use TCP transport else UDP, default is TCP", reliable);
  cmd.Parse (argc, argv);
  linkType = tolower (linkType);
  switch (linkType)
    {
    case 'c':
    case 'p':
    case 'w':
      break;
    default:
      std::cout << "Unknown link type : " << linkType << " ?" << std::endl;
      return 1;
    }

  NodeContainer nodes;
  nodes.Create (2);

  NetDeviceContainer devices;

  switch (linkType)
    {
    case 'c':
      {
        CsmaHelper csma;
        csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
        csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
        devices = csma.Install (nodes);
        csma.EnablePcapAll ("process-linux");
      }
      break;

    case 'p':
      {
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute ("DataRate", StringValue ("5Gbps"));
        p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
        devices = p2p.Install (nodes);
        p2p.EnablePcapAll ("process-linux");
      }
      break;

    case 'w':
      {
        MobilityHelper mobility;
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
        positionAlloc->Add (Vector (0.0, 0.0, 0.0));
        positionAlloc->Add (Vector (5.0, 0.0, 0.0));
        mobility.SetPositionAllocator (positionAlloc);
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (nodes);

        WifiHelper wifi;
        YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
        YansWifiChannelHelper phyChannel = YansWifiChannelHelper::Default ();
        WifiMacHelper mac;
        phy.SetChannel (phyChannel.Create ());
        mac.SetType ("ns3::AdhocWifiMac");
        wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
        devices = wifi.Install (phy, mac, nodes);
        phy.EnablePcapAll ("process-linux");
      }
      break;

    default:
      break;
    }

  DceManagerHelper processManager;
  // processManager.SetLoader ("ns3::DlmLoaderFactory");
  processManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
  processManager.Install (nodes);
  LinuxStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  LinuxStackHelper::PopulateRoutingTables ();

  for (int n = 0; n < 2; n++)
    {
      RunIp (nodes.Get (n), Seconds (0.2), "link show");
      RunIp (nodes.Get (n), Seconds (0.3), "route show table all");
      RunIp (nodes.Get (n), Seconds (0.4), "addr list");
    }

  DceApplicationHelper process;
  ApplicationContainer apps;

  if (reliable)
    {
      process.SetBinary ("tcp-server");
      process.ResetArguments ();
      process.SetStackSize (1 << 16);
      apps = process.Install (nodes.Get (0));
      apps.Start (Seconds (1.0));

      process.SetBinary ("tcp-client");
      process.ResetArguments ();
      process.ParseArguments ("10.0.0.1");
      apps = process.Install (nodes.Get (1));
      apps.Start (Seconds (1.5));
    }
  else
    {
      process.SetBinary ("udp-server");
      process.ResetArguments ();
      process.SetStackSize (1 << 16);
      apps = process.Install (nodes.Get (0));
      apps.Start (Seconds (1.0));

      process.SetBinary ("udp-client");
      process.ResetArguments ();
      process.ParseArguments ("10.0.0.1");
      apps = process.Install (nodes.Get (1));
      apps.Start (Seconds (1.5));
    }

  // print tcp sysctl value
  LinuxStackHelper::SysctlGet (nodes.Get (0), Seconds (1.0),
                               ".net.ipv4.tcp_available_congestion_control", &PrintTcpFlags);

  Simulator::Stop (Seconds (2000000.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
