#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
// Each node run a CCN daemon and each one forward interrests to other one,
//
// then a ccnput is launch on node 0 and a ccnget is launch on node 1
//
// ===========================================================================
int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps, putter, getter;

  dce.SetStackSize (1<<20);

  // Launch ccn daemon on node 0
  dce.SetBinary ("ccnd");
  dce.ResetArguments();
  dce.ResetEnvironment();

  dce.AddEnvironment("CCND_CAP", "50000");
  dce.AddEnvironment("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment("CCN_LOCAL_PORT", "9695");

  dce.AddEnvironment("CCND_CAP", "");
  dce.AddEnvironment("CCND_AUTOREG", "");
  dce.AddEnvironment("CCND_LISTEN_ON", "");
  dce.AddEnvironment("CCND_MTU", "");
  dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));

  // Configure ccn daemon on node 0 to forward interrests to node 1
  dce.SetBinary ("ccndc");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("HOME", "/home/furbani");  // USE TO FIND keystore under $HOME/.ccnx/.ccnx_keystore
  dce.AddArgument ("-f");
  dce.AddArgument ("/tmp/ccnd0.conf");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (10.0));

  // Launch ccn daemon on node 1
  dce.SetBinary ("ccnd");
  dce.ResetArguments();
  dce.ResetEnvironment();

  dce.AddEnvironment("CCND_CAP", "50000");
  dce.AddEnvironment("CCND_DEBUG", "-1");
  dce.AddEnvironment("CCN_LOCAL_PORT", "9695");

  dce.AddEnvironment("CCND_CAP", "");
  dce.AddEnvironment("CCND_AUTOREG", "");
  dce.AddEnvironment("CCND_LISTEN_ON", "");
  dce.AddEnvironment("CCND_MTU", "");
  dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (2.0));

  // Configure ccn daemon on node 1 to forward interrests to node 0
  dce.SetBinary ("ccndc");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("HOME", "/home/furbani");
  dce.AddArgument ("-f");
  dce.AddArgument ("/tmp/ccnd1.conf");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (11.0));

  // put a file somewhere on the Internet !
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccnput");
  dce.SetStdinFile ("/tmp/README");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment("HOME", "/home/furbani");

  putter = dce.Install (nodes.Get (0));
  putter.Start (Seconds (40.0));

  // Try to retrieve the file !
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccnget");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment("HOME", "/home/furbani");

  getter = dce.Install (nodes.Get (1));
  getter.Start (Seconds (42.0));

  Simulator::Stop (Seconds(100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
