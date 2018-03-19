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
NS_LOG_COMPONENT_DEFINE ("DceNetperf");
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
// 2 nodes : netperf and netserver ....
//
// ===========================================================================

int main (int argc, char *argv[])
{
  std::string stack = "ns3";
  bool useUdp = 0;
  std::string bandWidth = "1m";
  CommandLine cmd;
  cmd.AddValue ("stack", "Name of IP stack: ns3/linux/freebsd.", stack);
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

  if (stack == "ns3")
    {
      InternetStackHelper stack;
      stack.Install (nodes);
      dceManager.Install (nodes);
    }
  else if (stack == "linux")
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      dceManager.Install (nodes);
      LinuxStackHelper stack;
      stack.Install (nodes);
#else
      NS_LOG_ERROR ("Linux kernel stack for DCE is not available. build with dce-linux module.");
      // silently exit
      return 0;
#endif
    }
  else if (stack == "freebsd")
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::FreeBSDSocketFdFactory", "Library", StringValue ("libfreebsd.so"));
      dceManager.Install (nodes);
      FreeBSDStackHelper stack;
      stack.Install (nodes);
#else
      NS_LOG_ERROR ("FreeBSD kernel stack for DCE is not available. build with dce-freebsd module.");
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
  if (stack == "linux")
    {
      LinuxStackHelper::PopulateRoutingTables ();
    }
#endif


  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  dce.SetBinary ("netperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-P");
  dce.AddArgument ("0");
  dce.AddArgument ("-v");
  dce.AddArgument ("0");
  dce.AddArgument ("-D");
  dce.AddArgument ("-0.20");
  dce.AddArgument ("-4");
  dce.AddArgument ("-H");
  dce.AddArgument ("10.1.1.2");
  dce.AddArgument ("-p");
  dce.AddArgument ("12685");
  dce.AddArgument ("-t");
  dce.AddArgument ("TCP_STREAM");
  dce.AddArgument ("-l");
  dce.AddArgument ("10");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (3.0));
  apps.Stop (Seconds(20));

  dce.SetBinary ("netserver");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-D");
  dce.AddArgument ("-f");
  dce.AddArgument ("-p");
  dce.AddArgument ("12685");
  
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (0.6));

	pointToPoint.EnablePcapAll ("netperf-" + stack, false);
	
  setPos (nodes.Get (0), 1, 10, 0);
  setPos (nodes.Get (1), 50,10, 0);

  Simulator::Stop (Seconds (40.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
