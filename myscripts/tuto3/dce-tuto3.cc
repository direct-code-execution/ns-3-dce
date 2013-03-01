#include "ns3/dce-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/data-rate.h"

using namespace ns3;

static std::string Ipv4AddressToString (Ipv4Address ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}
// ===========================================================================
//
//   Topology
//
//
//         node 0                node 1               node 2
//   +----------------+    +----------------+    +----------------+
//   |   linux TCP    |    |   linux TCP    |    |   linux TCP    |
//   +----------------+    +----------------+    +----------------+
//   | iperf client   |    |                |    | iperf server   |
//   +----------------+    +----------------+    +----------------+
//   | point-to-point |    | point-to-point |    | point-to-point |
//   +----------------+    +----------------+    +----------------+
//           |  5 Mbps, 1 ms  |          |  5 Mbps 1ms   |
//           +----------------+          +---------------+
//       10.1.1.1       10.1.1.2      10.1.2.1       10.1.2.2
//
// This experience use iperf
//
// ===========================================================================

int main (int argc, char *argv[])
{
  std::string delay = "1ms";
  std::string rate = "5Mbps";
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (3);

  DceManagerHelper dceManager;
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
  LinuxStackHelper stack;
  stack.Install (nodes);

//  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
//  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (rate));
  p2p.SetChannelAttribute ("Delay", StringValue (delay));

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");

  NetDeviceContainer devices;
  devices = p2p.Install (nodes.Get (0), nodes.Get (1));
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  devices = p2p.Install (nodes.Get (1), nodes.Get (2));
  address.SetBase ("10.1.2.0", "255.255.255.252");
  interfaces = address.Assign (devices);

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.1.2.2");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("10");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.7));
  apps.Stop (Seconds (20));

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");

  apps = dce.Install (nodes.Get (2));
  apps.Start (Seconds (0.6));

  Simulator::Stop (Seconds (40.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
