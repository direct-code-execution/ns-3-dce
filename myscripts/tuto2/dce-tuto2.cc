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
//   |    NS-3 TCP    |    |   NS-3  TCP    |    |   NS-3  TCP    |
//   +----------------+    +----------------+    +----------------+
//   | thttpd         |    |                |    | wget           |
//   +----------------+    +----------------+    +----------------+
//   | point-to-point |    | point-to-point |    | point-to-point |
//   +----------------+    +----------------+    +----------------+
//           |  5 Mbps, 1 ms  |          |  5 Mbps 1ms   |
//           +----------------+          +---------------+
//       10.1.1.1       10.1.1.2      10.1.2.1       10.1.2.2
//
// This experience use thttpd and wget
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

  InternetStackHelper stack;
  stack.Install (nodes);

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

  // Launch the server HTTP
  dce.SetBinary ("thttpd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetUid (1);
  dce.SetEuid (1);

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (1));

  // Launch the client WGET
  dce.SetBinary ("wget");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-r");
  dce.AddArgument ("http://10.1.1.1/index.html");

  apps = dce.Install (nodes.Get (2));
  apps.Start (Seconds (2));

  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
