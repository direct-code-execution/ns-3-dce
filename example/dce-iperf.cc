#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ccnx/misc-tools.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceIperf");
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
  std::string bandWidth = "1m";
  CommandLine cmd;
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.AddValue ("udp", "Use UDP. Default false (0)", useUdp);
  cmd.AddValue ("bw", "BandWidth. Default 1m.", bandWidth);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType", StringValue ("UcontextFiberManager"));

  if (!useKernel)
    {
      InternetStackHelper stack;
      stack.Install (nodes);
    }
  else
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      LinuxStackHelper stack;
      stack.Install (nodes);
#else
      NS_LOG_ERROR ("Linux kernel stack for DCE is not available. build with dce-linux module.");
      // silently exit
      return 0;
#endif
    }

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
#ifdef KERNEL_STACK
  if (useKernel)
    {
      LinuxStackHelper::PopulateRoutingTables ();
    }
#endif

  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.1.1.2");
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
  if (useUdp)
    {
      dce.AddArgument ("-u");
    }

  apps = dce.Install (nodes.Get (1));

  pointToPoint.EnablePcapAll (useKernel ? "iperf-kernel" : "iperf-ns3", false);

  apps.Start (Seconds (0.6));

  setPos (nodes.Get (0), 1, 10, 0);
  setPos (nodes.Get (1), 50,10, 0);

  Simulator::Stop (Seconds (40.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
