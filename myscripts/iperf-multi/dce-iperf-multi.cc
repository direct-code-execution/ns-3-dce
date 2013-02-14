#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;


void RunIp (Ptr<Node> node, Time at, std::string str)
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

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |                |    |                |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
// 2 nodes : iperf client en iperf server ....
//
// Note : Tested with iperf 2.0.5, you need to modify iperf source in order to
//        allow DCE to have a chance to end an endless loop in iperf as follow:
//        in source named Thread.c at line 412 in method named thread_rest
//        you must add a sleep (1); to break the infinite loop....
// ===========================================================================
int main (int argc, char *argv[])
{
  bool useKernel = 0;
  bool useUdp = 0;
  bool useCsma = 0;
  std::string bandWidth = "1m";
  CommandLine cmd;
  int nb = 30;

  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.AddValue ("udp", "Use UDP. Default false (0)", useUdp);
  cmd.AddValue ("csma", "Use CSMA. Default false (point 2 point)", useCsma);
  cmd.AddValue ("bw", "BandWidth. Default 1m.", bandWidth);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (nb);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  DceManagerHelper dceManager;

  if (!useKernel)
    {
      InternetStackHelper stack;
      stack.Install (nodes);
    }
  else
    {
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      LinuxStackHelper stack;
      stack.Install (nodes);
    }
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  for (int n = 1 ; n < nb ; n++)
    {
      NodeContainer n2;

      n2.Add (nodes.Get (n - 1));
      n2.Add (nodes.Get (n));

      if (useCsma)
        {
          devices = csma.Install (n2);
        }
      else
        {
          devices = pointToPoint.Install (n2);
        }

      Ipv4InterfaceContainer interfaces = address.Assign (devices);
      address.NewNetwork ();
    }

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // Trace routing tables
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("all_the.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (0), routingStream);

  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.1.1.1");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("10");
  if (useUdp)
    {
      dce.AddArgument ("-u");
      dce.AddArgument ("-b");
      dce.AddArgument (bandWidth);
    }

  apps = dce.Install (nodes.Get (nb - 1));
  apps.Start (Seconds (0.7));
  apps.Stop (Seconds (20));

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");
  if (useUdp)
    {
      dce.AddArgument ("-u");
    }

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.6));

  if (useCsma)
    {
      csma.EnablePcap (useKernel ? "iperf-csma-kernel" : "iperf-csma-ns3", devices, false);
    }
  else
    {
      pointToPoint.EnablePcapAll (useKernel ? "iperf-p2p-kernel" : "iperf-p2p-ns3", false);
    }

  Simulator::Stop (Seconds (400.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
