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
//                    +----------+
//                         |sim0
//               sim1 +----+-----+ sim2
//              +-----|    R     |------+
//              |     +----------+      |
//              |                       |
//              |                       |
//              |sim0                   |sim0
//         +----+---+              +----+---+
//         |   AR1  |              |   AR2  |
//         +---+----+              +----+---+
//             |sim1                    |sim1
//             |                        |
//           :::::
//             |sim0                    |sim0
//        +---------+   (Movement)  +--------+
//        |    MN   |     <=====>   |   MN   |
//        +---------+               +--------+

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/quagga-helper.h"


using namespace ns3;


static void AddAddress (Ptr<Node> node, Time at, const char *name, const char *address)
{
  std::ostringstream oss;
  oss << "-f inet6 addr add " << address << " dev " << name;
  LinuxStackHelper::RunIp (node, at, oss.str ());
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  double errRate = 0.00;
  cmd.AddValue ("errRate", "Rcv error rate.", errRate);
  cmd.Parse (argc, argv);

  NodeContainer mn, sv, ar, router;
  sv.Create (1);
  router.Create (1);
  ar.Create (2);
  mn.Create (1);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (50.0, -75.0, 0.0)); // SV
  positionAlloc->Add (Vector (50.0, -50.0, 0.0)); // R
  positionAlloc->Add (Vector (0.0, 10.0, 0.0)); // AR1
  positionAlloc->Add (Vector (100.0, 10.0, 0.0)); // AR2
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (sv);
  mobility.Install (router);
  mobility.Install (ar);

  Ptr<ns3::RandomDiscPositionAllocator> r_position =
    CreateObject<RandomDiscPositionAllocator> ();
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (0.0));
  x->SetAttribute ("Max", DoubleValue (100.0));
  r_position->SetX (30);
  r_position->SetY (100);
  r_position->SetRho (x);
  mobility.SetPositionAllocator (r_position);
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 150, 30, 60)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (mn);


  // backend
  PointToPointHelper p2p;
  Ptr<RateErrorModel> em1 =
    CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
                                                "ErrorRate", DoubleValue (errRate),
                                                "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET)
                                                );
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer dev = p2p.Install (NodeContainer (sv.Get (0), router.Get (0)));
  dev.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

  p2p.Install (NodeContainer (router.Get (0), ar.Get (0)));
  p2p.Install (NodeContainer (router.Get (0), ar.Get (1)));

  WifiHelper wifi;
  WifiMacHelper mac;
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper phyChannel = YansWifiChannelHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");

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

  // Second SSid
  phy.SetChannel (phyChannel.Create ());
  Ssid ssid2 = Ssid ("wifi-ap2");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid2),
               "ActiveProbing", BooleanValue (false));
  wifi.Install (phy, mac, mn);
  // ap
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid2));
  wifi.Install (phy, mac, ar.Get (1));


  DceManagerHelper dceMng;
  DceApplicationHelper dce;
  LinuxStackHelper stack;
  dceMng.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                          "Library", StringValue ("liblinux.so"));
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

  // For SV
  AddAddress (sv.Get (0), Seconds (0.1), "sim0", sv_sim0.c_str ());
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (3.15), "-6 route add default via 2001:1:2:3::2 dev sim0");

  // For R (router)
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set lo up");
  AddAddress (router.Get (0), Seconds (0.1), "sim0", "2001:1:2:3::2/64");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set sim0 up");
  AddAddress (router.Get (0), Seconds (0.1), "sim1", "2001:1:2:1::1/64");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set sim1 up");
  AddAddress (router.Get (0), Seconds (0.1), "sim2", "2001:1:2:2::1/64");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (0.11), "link set sim2 up");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (3.15), "-6 route add 2001:1:2:4::/64 via 2001:1:2:1::2 dev sim1");
  LinuxStackHelper::RunIp (router.Get (0), Seconds (3.15), "-6 route add 2001:1:2:7::/64 via 2001:1:2:2::2 dev sim2");
  stack.SysctlSet (router, ".net.ipv6.conf.all.forwarding", "1");

  // For AR1 (the intermediate node)
  AddAddress (ar.Get (0), Seconds (0.1), "sim0", "2001:1:2:1::2/64");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (3.15), "-6 route add 2001:1:2:3::/64 via 2001:1:2:1::1 dev sim0");
  AddAddress (ar.Get (0), Seconds (0.12), "sim1", "2001:1:2:4::2/64");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (0.13), "link set sim1 up");

  // For AR2 (the intermediate node)
  AddAddress (ar.Get (1), Seconds (0.1), "sim0", "2001:1:2:2::2/64");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (3.15), "-6 route add 2001:1:2:3::/64 via 2001:1:2:2::1 dev sim0");
  AddAddress (ar.Get (1), Seconds (0.12), "sim1", "2001:1:2:7::2/64");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (0.13), "link set sim1 up");
  stack.SysctlSet (ar, ".net.ipv6.conf.all.forwarding", "1");

  // For MN
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "-6 rule add from 2001:1:2:4:200:ff:fe00:7 table 2");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "route add default via fe80::200:ff:fe00:8 dev sim0 table 2");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "route add default via fe80::200:ff:fe00:8 dev sim0");
  //  LinuxStackHelper::RunIp (mn.Get (0), Seconds (27.0), "link set sim1 up");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (2.0), "link set sim1 up");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "-6 rule add from 2001:1:2:7:200:ff:fe00:9 table 3");
  //  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "route add default via fe80::200:ff:fe00:a dev sim1");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (0.20), "route add default via fe80::200:ff:fe00:a dev sim1 table 3");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (15.20), "-6 addr change 2001:1:2:7::3939/64 dev sim1");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (18.20), "-6 addr flush dev sim1");
  // disable default injection from ra
  stack.SysctlSet (mn, ".net.ipv6.conf.sim0.accept_ra_defrtr", "0");
  stack.SysctlSet (mn, ".net.ipv6.conf.sim1.accept_ra_defrtr", "0");

  // debug
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (4.0), "addr list");
  LinuxStackHelper::RunIp (ar.Get (0), Seconds (4.1), "addr list");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (40.2), "addr list");
  LinuxStackHelper::RunIp (sv.Get (0), Seconds (20.0), "route show table all");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (49.0), "route show table all");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (49.0), "-6 rule show");
  LinuxStackHelper::RunIp (ar.Get (1), Seconds (10.0), "route show table all");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (10.0), "route show table all");
  LinuxStackHelper::RunIp (mn.Get (0), Seconds (10.1), "route get 2001:1:2:3::1 from 2001:1:2:4:200:ff:fe00:7");

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
    apps = dce.Install (sv);
    apps.Start (Seconds (4));

    // MN
    dce.SetBinary ("iperf");
    dce.ResetArguments ();
    dce.ResetEnvironment ();
    dce.AddArgument ("-V");
    dce.AddArgument ("-c");
    std::string sv_addr = sv_sim0;
    sv_addr.replace (sv_addr.find ("/"), 3, "\0  ");
    dce.AddArgument (sv_addr);
    dce.AddArgument ("-i");
    dce.AddArgument ("1");
    dce.AddArgument ("--time");
    dce.AddArgument ("100");
    apps = dce.Install (mn);
    apps.Start (Seconds (10.0));

    // AR
    quagga.EnableRadvd (ar.Get (0), "sim1", "2001:1:2:4::/64");
    quagga.EnableRadvd (ar.Get (1), "sim1", "2001:1:2:7::/64");
    quagga.EnableZebraDebug (ar);
    quagga.Install (ar);
  }

  phy.EnablePcapAll ("dce-mptcp-handoff");
  p2p.EnablePcapAll ("dce-mptcp-handoff");

  Simulator::Stop (Seconds (50.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
