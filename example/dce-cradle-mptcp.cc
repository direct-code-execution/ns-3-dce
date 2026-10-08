#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"

using namespace ns3;

// Bytes the server received from each address of the client.
static std::map<Ipv4Address, uint64_t> g_received;

static void
ServerRx (Ptr<const Packet> p)
{
  Ptr<Packet> packet = p->Copy ();
  PppHeader ppp;
  Ipv4Header ip;
  packet->RemoveHeader (ppp);
  if (ppp.GetProtocol () == 0x0021) // IPv4
    {
      packet->RemoveHeader (ip);
      g_received[ip.GetSource ()] += packet->GetSize ();
    }
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
  uint32_t nRtrs = 2;
  CommandLine cmd;
  cmd.AddValue ("nRtrs", "Number of routers. Default 2", nRtrs);
  cmd.Parse (argc, argv);

  NodeContainer nodes, routers;
  nodes.Create (2);
  routers.Create (nRtrs);

  // The applications' TCP sockets are MPTCP sockets (Linux MPTCP v1: only
  // with LKL; libos has no MPTCP).
  Config::SetDefaultFailSafe ("ns3::LklSocketFdFactory::Mptcp", BooleanValue (true));

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType",
                                      StringValue ("UcontextFiberManager"));

  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));
  LinuxStackHelper stack;
  stack.Install (nodes);
  stack.Install (routers);

  dceManager.Install (nodes);
  dceManager.Install (routers);

  PointToPointHelper pointToPoint;
  NetDeviceContainer devices1, devices2;
  Ipv4AddressHelper address1, address2;
  std::ostringstream cmd_oss;
  address1.SetBase ("10.1.0.0", "255.255.255.0");
  address2.SetBase ("10.2.0.0", "255.255.255.0");

  for (uint32_t i = 0; i < nRtrs; i++)
    {
      // Left link
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
      pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
      devices1 = pointToPoint.Install (nodes.Get (0), routers.Get (i));
      // Assign ip addresses
      Ipv4InterfaceContainer if1 = address1.Assign (devices1);
      address1.NewNetwork ();
      // setup ip routes
      cmd_oss.str ("");
      cmd_oss << "rule add from " << if1.GetAddress (0, 0) << " table " << (i+1);
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.1." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add default via " << if1.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.1.0.0/16 via " << if1.GetAddress (1, 0) << " dev sim0";
      LinuxStackHelper::RunIp (routers.Get (i), Seconds (0.2), cmd_oss.str ().c_str ());

      // Right link
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
      pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ns"));
      devices2 = pointToPoint.Install (nodes.Get (1), routers.Get (i));
      // Assign ip addresses
      Ipv4InterfaceContainer if2 = address2.Assign (devices2);
      address2.NewNetwork ();
      // setup ip routes
      cmd_oss.str ("");
      cmd_oss << "rule add from " << if2.GetAddress (0, 0) << " table " << (i+1);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.2." << i << ".0/24 dev sim" << i << " scope link table " << (i+1);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add default via " << if2.GetAddress (1, 0) << " dev sim" << i << " table " << (i+1);
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "route add 10.2.0.0/16 via " << if2.GetAddress (1, 0) << " dev sim1";
      LinuxStackHelper::RunIp (routers.Get (i), Seconds (0.2), cmd_oss.str ().c_str ());

      setPos (routers.Get (i), 50, i * 20, 0);
    }

  // default route
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "route add default via 10.1.0.2 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), "route add default via 10.2.0.2 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "rule show");

  stack.SysctlSet (routers, ".net.ipv4.conf.all.forwarding", "1");

  // MPTCP: up to 4 subflows per connection; the client opens subflows from
  // its other addresses, the server announces its other addresses.
  for (uint32_t n = 0; n < 2; n++)
    {
      LinuxStackHelper::RunIp (nodes.Get (n), Seconds (0.5), "mptcp limits set subflows 4 add_addr_accepted 4");
    }
  for (uint32_t i = 1; i < nRtrs; i++)
    {
      cmd_oss.str ("");
      cmd_oss << "mptcp endpoint add 10.1." << i << ".1 dev sim" << i << " subflow fullmesh";
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.5), cmd_oss.str ().c_str ());
      cmd_oss.str ("");
      cmd_oss << "mptcp endpoint add 10.2." << i << ".1 dev sim" << i << " signal";
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.5), cmd_oss.str ().c_str ());
    }
  Config::ConnectWithoutContext ("/NodeList/1/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxEnd",
                                 MakeCallback (&ServerRx));

  ApplicationContainer apps;
  OnOffHelper onoff = OnOffHelper ("ns3::LinuxTcpSocketFactory",
                                   InetSocketAddress ("10.2.0.1", 9));
  onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute ("PacketSize", StringValue ("1024"));
  onoff.SetAttribute ("DataRate", StringValue ("10Mbps"));
  apps = onoff.Install (nodes.Get (0));
  apps.Start (Seconds (5.0));

  // server on node 1
  PacketSinkHelper sink = PacketSinkHelper ("ns3::LinuxTcpSocketFactory",
                                            InetSocketAddress (Ipv4Address::GetAny (), 9));
  apps = sink.Install (nodes.Get (1));
  apps.Start (Seconds (3.9999));

  pointToPoint.EnablePcapAll ("mptcp-dce-cradle", false);

  apps.Start (Seconds (4));

  setPos (nodes.Get (0), 0, 20 * (nRtrs - 1) / 2, 0);
  setPos (nodes.Get (1), 100, 20 * (nRtrs - 1) / 2, 0);

  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();

  bool ok = true;
  for (uint32_t i = 0; i < nRtrs; i++)
    {
      std::ostringstream addr;
      addr << "10.1." << i << ".1";
      uint64_t bytes = g_received[Ipv4Address (addr.str ().c_str ())];
      std::cout << "received from " << addr.str () << ": " << bytes << " bytes" << std::endl;
      ok = ok && bytes > 0;
    }
#ifdef LKL_LINUX
  // With MPTCP, every path carries data.
  return ok ? 0 : 1;
#else
  return 0;
#endif
}
