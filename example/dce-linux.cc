#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include <fstream>

using namespace ns3;

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

static void AddAddress (Ptr<Node> node, Time at, const char *name, const std::string prefixAddr,
                        int number, std::string suffixAddr)
{
  std::ostringstream oss;
  oss << "-f inet addr add " << prefixAddr << number << suffixAddr << " dev " << name;
  RunIp (node, at, oss.str ());
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  char linkType = 'p'; // P2P
  bool reliable = true;

  cmd.AddValue ("linkType" , "Link type: ie : C for CSMA, P for Point to Point and w for Wifi, default to P2P", linkType);
  cmd.AddValue ("reliable" , "If true use TCP transport else UDP, default is TCP", reliable);
  cmd.Parse (argc, argv);
  linkType = tolower (linkType);
  switch ( linkType )
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

  switch ( linkType )
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

        WifiHelper wifi = WifiHelper::Default ();
        YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
        YansWifiChannelHelper phyChannel = YansWifiChannelHelper::Default ();
        NqosWifiMacHelper mac;
        phy.SetChannel (phyChannel.Create ());
        mac.SetType ("ns3::AdhocWifiMac");
        wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
        devices = wifi.Install (phy, mac, nodes);
        phy.EnablePcapAll ("process-linux");
      }
      break;

    default: break;
  }

  DceManagerHelper processManager;
 // processManager.SetLoader ("ns3::DlmLoaderFactory");
  processManager.SetNetworkStack("ns3::LinuxSocketFdFactory",
				 "Library", StringValue ("libnet-next-2.6.so"));
  processManager.Install (nodes);

  for (int n=0; n < 2; n++)
    {
      AddAddress (nodes.Get (n), Seconds (0.1), "sim0", "10.0.0.", 2 + n, "/8" );
      RunIp (nodes.Get (n), Seconds (0.11),
          ( 'p' == linkType )? "link set sim0 up arp off":"link set sim0 up arp on");
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
        process.SetStackSize (1<<16);
        apps = process.Install (nodes.Get (0));
        apps.Start (Seconds (1.0));

        process.SetBinary ("tcp-client");
        process.ResetArguments ();
        process.ParseArguments ("10.0.0.2");
        apps = process.Install (nodes.Get (1));
        apps.Start (Seconds (1.5));
      }
    else
      {
        process.SetBinary ("udp-server");
        process.ResetArguments ();
        process.SetStackSize (1<<16);
        apps = process.Install (nodes.Get (0));
        apps.Start (Seconds (1.0));

        process.SetBinary ("udp-client");
        process.ResetArguments ();
        process.ParseArguments ("10.0.0.2");
        apps = process.Install (nodes.Get (1));
        apps.Start (Seconds (1.5));
      }

  Simulator::Stop (Seconds (2000000.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
