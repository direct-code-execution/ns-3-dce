/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

//
// Handoff scenario with Multipath TCPed iperf
//
// Simulation Topology:
// Scenario: MN moves from under AR1 to AR2 (Wi-Fi AP), changing its IP address.
//           during movement, MN keeps iperf session to SV.
//
//                    +----------+
//                    |    SV    |
//                    +----+-----+  
//                         |  
//                     sim1|  
//               sim2 +----+--+--+ sim0
//              +-----|    R     |------+
//              |     +----------+      |
//              |                       |
//              |                       |
//              |sim0                   |sim0
//         +----+---+              +----+---+
//         |   AR1  |              | LTE BS |
//         +---+----+              +----+---+
//             |sim1                    |sim1
//             |                        +----------------+    
//           :::::                                  (always in range)        
//             |sim1                    |sim1            |
//        +---------+   (Movement)  +--------+           |
//        |    MN   |     <=====>   |   MN   +-----------+
//        +---------+               +--------+ sim0 (LteUe)

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/lte-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/quagga-helper.h"


using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceMptcpLteWifiV6");


static void AddAddress (Ptr<Node> node, Time at, const char *name, const char *address)
{
  std::ostringstream oss;
  oss << "-f inet6 addr add " << address << " dev " << name;
  LinuxStackHelper::RunIp (node, at, oss.str ());
}

void setPos (Ptr<Node> n, int x, int y, int z)
{
  Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
  n->AggregateObject (loc);
  Vector locVec2 (x, y, z);
  loc->SetPosition (locVec2);
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  double errRate = 0.00;
  cmd.AddValue ("errRate", "Rcv error rate.", errRate);
  cmd.Parse (argc, argv);

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  NodeContainer mn, sv, ar, ltebs, router;
  sv.Create (1);
  router.Create (1);
  ar.Create (1);
  mn.Create (1);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (50.0, -75.0, 0.0)); // SV
  positionAlloc->Add (Vector (50.0, -50.0, 0.0)); // R
  positionAlloc->Add (Vector (0.0, 10.0, 0.0)); // AR1
  //  positionAlloc->Add (Vector (100.0, 10.0, 0.0)); // LTE-BS
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (sv);
  mobility.Install (router);
  mobility.Install (ar);

  Ptr<ns3::RandomDiscPositionAllocator> r_position =
    CreateObject<RandomDiscPositionAllocator> ();
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (0.0));
  x->SetAttribute ("Max", DoubleValue (200.0));
  r_position->SetX (30);
  r_position->SetY (80);
  r_position->SetRho (x);
  mobility.SetPositionAllocator (r_position);
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-200, 60, 0, 70)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=50.0]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  //  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (mn);


  // LTE
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();

  // Down link: MN1 <-> LTE-BS
  NodeContainer enbNodes;
  enbNodes.Create(1);
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  setPos (enbNodes.Get (0), 90, -10, 0);

  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (mn.Get (0));

  // LTE-BS <-> R
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  Ptr<RateErrorModel> em1 =
    CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
                                                "ErrorRate", DoubleValue (errRate),
                                                "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET)
                                                );
  // Up link
  NetDeviceContainer dev1 = p2p.Install (NodeContainer (router.Get (0), pgw));
  dev1.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

  // backend
  NetDeviceContainer dev2 = p2p.Install (NodeContainer (sv.Get (0), router.Get (0)));
  dev2.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

  p2p.Install (NodeContainer (router.Get (0), ar.Get (0)));

  // Wi-Fi
  WifiHelper wifi;
  WifiMacHelper mac;
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper phyChannel = YansWifiChannelHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  // setup Wifi sta. 
  phy.SetChannel (phyChannel.Create ());
  Ssid ssid1 = Ssid ("wifi-ap1");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid1),
               "ActiveProbing", BooleanValue (false));
  wifi.Install (phy, mac, mn);
  // setup ap.
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid1));
  wifi.Install (phy, mac, ar.Get (0));


  DceManagerHelper dceMng;
  DceApplicationHelper dce;
  LinuxStackHelper stack;
  dceMng.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                          "Library", StringValue ("liblinux.so"));
  dceMng.SetLoader ("ns3::DlmLoaderFactory");
  dceMng.SetTaskManagerAttribute ("FiberManagerType",
                                  StringValue ("UcontextFiberManager"));
  dceMng.Install (mn);
  dceMng.Install (router);
  dceMng.Install (sv);
  dceMng.Install (ar);
  stack.Install (mn);
  stack.Install (sv);
  stack.Install (router);
  stack.Install (ar);

  // Prefix configuration
  std::string sv_sim0 ("2001:1:2:3::1/64");
  std::string sv_sim0_v4 ("10.0.0.1/24");

  // For SV
  AddAddress (sv.Get (0), Seconds (0.1), "sim0", sv_sim0.c_str ());
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (0.1), "addr add 10.0.0.1/24 dev sim0");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (3.15), "-4 route add default via 10.0.0.2 dev sim0");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (3.15), "-6 route add default via 2001:1:2:3::2 dev sim0");

  // For R (router)
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.1), "addr add 10.0.0.2/24 dev sim1");
  AddAddress (router.Get (0), Seconds (0.1), "sim1", "2001:1:2:3::2/64");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set sim1 up");
  AddAddress (router.Get (0), Seconds (0.1), "sim2", "2001:1:2:1::1/64");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set sim2 up");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.1), "route add 7.0.0.0/24 via 10.1.0.2 dev sim0");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (3.15), "-6 route add 2001:1:2:4::/64 via 2001:1:2:1::2 dev sim2");
  stack.SysctlSet (router, ".net.ipv6.conf.all.forwarding", "1");

  // For AR1 (the intermediate node)
  AddAddress (ar.Get (0), Seconds (0.1), "sim0", "2001:1:2:1::2/64");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (3.15), "-6 route add 2001:1:2:3::/64 via 2001:1:2:1::1 dev sim0");
  AddAddress (ar.Get (0), Seconds (0.12), "sim1", "2001:1:2:4::2/64");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (0.13), "link set sim1 up");
  stack.SysctlSet (ar, ".net.ipv6.conf.all.forwarding", "1");

  // For AR2 (the intermediate node)
#if 0
  AddAddress (ar.Get (1), Seconds (0.1), "sim0", "2001:1:2:2::2/64");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (3.15), "-6 route add 2001:1:2:3::/64 via 2001:1:2:2::1 dev sim0");
  AddAddress (ar.Get (1), Seconds (0.12), "sim1", "2001:1:2:7::2/64");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (0.13), "link set sim1 up");
#endif

  // For LTE-BS
  // Assign ip addresses
  Ipv4InterfaceContainer if1;
  if1 = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));

  // setup ip routes
  std::ostringstream cmd_oss;
  cmd_oss.str ("");
  cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << 1;
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
  cmd_oss.str ("");
  cmd_oss << "route add default via " << "7.0.0.1 "  << " dev sim" << 0 << " table " << 1;
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.1), "route add default via 7.0.0.1 dev sim0");

  // LTE-BS <-> R
  // Assign ip addresses
  Ipv4AddressHelper address1;
  address1.SetBase ("10.1.0.0", "255.255.255.0");
  if1 = address1.Assign (dev1);
  address1.NewNetwork ();

  //  LinuxStackHelper::RunIp (pgw, Seconds (1.0), "route add 10.0.0.0/24 via 10.1.0.1 dev sim0");
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (pgw->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.0.0.0"), Ipv4Mask ("255.255.255.0"),
                                              Ipv4Address ("10.1.0.1"), 3);

  // setup ip routes
#if 0
  cmd_oss.str ("");
  cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << (1);
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
  cmd_oss.str ("");
  cmd_oss << "route add 10.2." << 0 << ".0/24 dev sim" << 0 << " scope link table " << (1);
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
#endif
  setPos (pgw, 100, -10, 0);

  // For MN
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.11), "link set sim1 up");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "-6 rule add from 2001:1:2:4:200:ff:fe00:a table 2");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "route add default via fe80::200:ff:fe00:b dev sim1 table 2");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "route add default via fe80::200:ff:fe00:b dev sim1");
  // disable default injection from ra
  stack.SysctlSet (mn, ".net.ipv6.conf.sim1.accept_ra_defrtr", "0");

  //  LinuxStackHelper::PopulateRoutingTables ();

  // debug
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (4.0), "addr list");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (4.1), "addr list");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (40.2), "addr list");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (20.0), "route show table all");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (20.0), "route show table all");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (49.0), "route show table all");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (49.0), "-6 rule show");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (10.0), "route show table all");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (10.1), "route get 2001:1:2:3::1 from 2001:1:2:4:200:ff:fe00:a");

  stack.SysctlSet (mn, ".net.mptcp.mptcp_debug", "1");
  stack.SysctlSet (sv, ".net.mptcp.mptcp_debug", "1");

  {
    ApplicationContainer apps;
    QuaggaHelper quagga;
    dce.SetStackSize (1 << 20);

    // SV
    dce.SetBinary ("iperf");
    dce.ResetArguments ();
    dce.ResetEnvironment ();
    dce.AddArgument ("-V");
    dce.AddArgument ("-s");
    dce.AddArgument ("-P");
    dce.AddArgument ("1");
    dce.AddArgument ("-i");
    dce.AddArgument ("1");
    apps = dce.Install (sv);
    apps.Start (Seconds (4));

    // MN
    dce.SetBinary ("iperf");
    dce.ResetArguments ();
    dce.ResetEnvironment ();
    dce.AddArgument ("-V");
    dce.AddArgument ("-c");
    std::string sv_addr = sv_sim0_v4;
    sv_addr.replace (sv_addr.find ("/"), 3, "\0  ");
    dce.AddArgument (sv_addr);
    dce.AddArgument ("-i");
    dce.AddArgument ("1");
    dce.AddArgument ("--time");
    dce.AddArgument ("10000");
    apps = dce.Install (mn);
    apps.Start (Seconds (6.0));

    // AR
    quagga.EnableRadvd (ar.Get (0), "sim1", "2001:1:2:4::/64");
    quagga.EnableZebraDebug (ar);
    quagga.Install (ar);
  }

  phy.EnablePcapAll ("dce-mptcp-lte-wifi-v6");
  p2p.EnablePcapAll ("dce-mptcp-lte-wifi-v6");

  // Simulator::Stop (Seconds (50.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
