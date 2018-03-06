/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

//
// Handoff scenario with Multipath TCPed iperf
//
// Simulation Topology:
// Scenario: H1 has 3G
//           during movement, MN keeps iperf session to SV.
//
//   <--------------------            ----------------------->
//                  LTE               Ethernet
//                   sim0 +----------+ sim1
//                  +------|  LTE  R  |------+
//                  |     +----------+      |
//              +---+                       +-----+
//          sim0|                                 |sim0
//     +----+---+                                 +----+---+
//     |   H1   |                                 |   H2   |
//     +---+----+                                 +----+---+
//          sim1|                                 |sim1
//              +--+                        +-----+
//                 | sim0 +----------+ sim1 |
//                  +-----|  WiFi R  |------+
//                        +----------+      
//                  WiFi              Ethernet
//   <--------------------            ----------------------->

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/config-store-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceMptcpLteWifi");

void setPos (Ptr<Node> n, int x, int y, int z)
{
  Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
  n->AggregateObject (loc);
  Vector locVec2 (x, y, z);
  loc->SetPosition (locVec2);
}

void
PrintTcpFlags (std::string key, std::string value)
{
  NS_LOG_INFO (key << "=" << value);
}

int main (int argc, char *argv[])
{
  LogComponentEnable ("DceMptcpLteWifi", LOG_LEVEL_ALL);
  std::string bufSize = "";
  bool disWifi = false;
  bool disLte = false;
  double stopTime = 45.0;
  std::string p2pdelay = "10ms";

  CommandLine cmd;
  cmd.AddValue ("bufsize", "Snd/Rcv buffer size.", bufSize);
  cmd.AddValue ("disWifi", "Disable WiFi.", disWifi);
  cmd.AddValue ("disLte", "Disable LTE.", disLte);
  cmd.AddValue ("stopTime", "StopTime of simulatino.", stopTime);
  cmd.AddValue ("p2pDelay", "Delay of p2p links. default is 10ms.", p2pdelay);
  cmd.Parse (argc, argv);

  if (disWifi && disLte)
    {
      NS_LOG_INFO ("no active interface");
      return 0;
    }

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  NodeContainer nodes, routers;
  nodes.Create (2);
  routers.Create (1);

  DceManagerHelper dceManager;
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));
  LinuxStackHelper stack;
  stack.Install (nodes);
  stack.Install (routers);

  dceManager.Install (nodes);
  dceManager.Install (routers);

  PointToPointHelper pointToPoint;
  NetDeviceContainer devices1, devices2;
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  YansWifiPhyHelper phy;

  Ipv4AddressHelper address1, address2;
  std::ostringstream cmd_oss;
  address1.SetBase ("10.1.0.0", "255.255.255.0");
  address2.SetBase ("10.2.0.0", "255.255.255.0");
  Ipv4InterfaceContainer if1, if2;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (p2pdelay));
  Ptr<RateErrorModel> em1 =
    CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
                                                "ErrorRate", DoubleValue (0.01),
                                                "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET)
                                                );

  setPos (nodes.Get (0), -20, 30 / 2, 0);
  setPos (nodes.Get (1), 100, 30 / 2, 0);
  // LTE
  if (!disLte)
    {
      // Left link: H1 <-> LTE-R
      NodeContainer enbNodes;
      enbNodes.Create(1);

      lteHelper->SetEpcHelper (epcHelper);
      Ptr<Node> pgw = epcHelper->GetPgwNode ();
      setPos (enbNodes.Get (0), 60, -4000, 0);

      NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
      NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (nodes.Get (0));

      // Assign ip addresses
      if1 = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
      lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));

      // setup ip routes
      cmd_oss.str ("");
      cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << 1;
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add default via " << "7.0.0.1 "  << " dev sim" << 0 << " table " << 1;
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());

      // LTE-R <-> H2
      // Right link
      devices2 = pointToPoint.Install (nodes.Get (1), pgw);
      devices2.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));
      // Assign ip addresses
      if2 = address2.Assign (devices2);
      address2.NewNetwork ();
      // setup ip routes
      cmd_oss.str ("");
      cmd_oss << "rule add from " << if2.GetAddress (0, 0) << " table " << (1);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.2." << 0 << ".0/24 dev sim" << 0 << " scope link table " << (1);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      setPos (pgw, 70, 0, 0);
    }

  if (!disWifi)
    {
      // Left link: H1 <-> WiFi-R
      WifiHelper wifi;
      phy = YansWifiPhyHelper::Default ();
      YansWifiChannelHelper phyChannel = YansWifiChannelHelper::Default ();
      WifiMacHelper mac;
      phy.SetChannel (phyChannel.Create ());
      mac.SetType ("ns3::AdhocWifiMac");
      wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
      devices1 = wifi.Install (phy, mac, NodeContainer (nodes.Get (0), routers.Get (0)));
      // Assign ip addresses
      if1 = address1.Assign (devices1);
      address1.NewNetwork ();
      // setup ip routes
      cmd_oss.str ("");
      cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << 2;
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.1." << 0 << ".0/24 dev sim"
              << devices1.Get (0)->GetIfIndex () << " scope link table " << 2;
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add default via " << if1.GetAddress (1, 0) << " dev sim" 
              << devices1.Get (0)->GetIfIndex () << " table " << 2;
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.1.0.0/16 via " << if1.GetAddress (0, 0) << " dev sim0";
      LinuxStackHelper::RunIp (routers.Get (0), Seconds (0.2), cmd_oss.str ().c_str ());

      // Global default route
      if (disLte)
        {
          cmd_oss.str ("");
          cmd_oss << "route add default via " << if1.GetAddress (1, 0) << " dev sim" 
                  << devices1.Get (0)->GetIfIndex ();
          LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
        }

      // Down/Up
#if 0
      cmd_oss.str ("");
      cmd_oss << "link set down dev sim1";
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (1.0), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "link set up dev sim1";
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (10.0), cmd_oss.str ().c_str ());
#endif

      // WiFi-R <-> H2
      // Right link
      devices2 = pointToPoint.Install (nodes.Get (1), routers.Get (0));
      devices2.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));
      // Assign ip addresses
      if2 = address2.Assign (devices2);
      address2.NewNetwork ();
      // setup ip routes
      cmd_oss.str ("");
      cmd_oss << "rule add from " << if2.GetAddress (0, 0) << " table " << (2);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.2." << 1 << ".0/24 dev sim" << 1 << " scope link table " << (2);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.1.0.0/16 via " << if2.GetAddress (1, 0) << " dev sim" << 1 << " table " << (2);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.2.0.0/16 via " << if2.GetAddress (1, 0) << " dev sim1";
      LinuxStackHelper::RunIp (routers.Get (0), Seconds (0.2), cmd_oss.str ().c_str ());
      setPos (routers.Get (0), 70, 30, 0);
    }

  // default route
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "route add default via 7.0.0.1 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), "route add default via 10.2.0.2 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "rule show");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.1), "route show table all");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), "rule show");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.1), "route show table all");

  // debug
  stack.SysctlSet (nodes, ".net.mptcp.mptcp_debug", "1");

#if 1
  LinuxStackHelper::SysctlGet (nodes.Get (0), NanoSeconds (0),
                               ".net.ipv4.tcp_available_congestion_control", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), NanoSeconds (0),
                               ".net.ipv4.tcp_rmem", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), NanoSeconds (0),
                               ".net.ipv4.tcp_wmem", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), NanoSeconds (0),
                               ".net.core.rmem_max", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), NanoSeconds (0),
                               ".net.core.wmem_max", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), Seconds (1),
                               ".net.ipv4.tcp_available_congestion_control", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), Seconds (1),
                               ".net.ipv4.tcp_rmem", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), Seconds (1),
                               ".net.ipv4.tcp_wmem", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), Seconds (1),
                               ".net.core.rmem_max", &PrintTcpFlags);
  LinuxStackHelper::SysctlGet (nodes.Get (0), Seconds (1),
                               ".net.core.wmem_max", &PrintTcpFlags);
#endif
#if 1
  if (bufSize.length () != 0)
    {
      stack.SysctlSet (nodes, ".net.ipv4.tcp_rmem",
                       bufSize + " " + bufSize + " " + bufSize);
      //                       "4096 87380 " +bufSize);
      stack.SysctlSet (nodes, ".net.ipv4.tcp_wmem",
                       bufSize + " " + bufSize + " " + bufSize);
      stack.SysctlSet (nodes, ".net.core.rmem_max",
                       bufSize);
      stack.SysctlSet (nodes, ".net.core.wmem_max",
                       bufSize);
    }
#endif


  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.2.0.1");
  dce.ParseArguments ("-y C");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("40");
#if 0
  if (bufSize.length () != 0)
    {
      dce.AddArgument ("-w");
      dce.AddArgument (bufSize);
    }
#endif

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (5.0));
  //  apps.Stop (Seconds (15));

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");
#if 0
  if (bufSize.length () != 0)
    {
      dce.AddArgument ("-w");
      dce.AddArgument (bufSize);
    }
#endif
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (4));

  pointToPoint.EnablePcapAll ("mptcp-lte-wifi", false);
  phy.EnablePcapAll ("mptcp-lte-wifi", false);
  //  lteHelper->EnableTraces ();

  // Output config store to txt format
  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig2;
  outputConfig2.ConfigureDefaults ();
  outputConfig2.ConfigureAttributes ();

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
