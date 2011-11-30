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
  std::string animFile = "NetAnim.tr";
  bool useKernel = 0;
  CommandLine cmd;
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  if (!useKernel)
    {
      InternetStackHelper stack;
      stack.Install (nodes);

      Ipv4AddressHelper address;
      address.SetBase ("10.1.1.0", "255.255.255.252");
      Ipv4InterfaceContainer interfaces = address.Assign (devices);

      // setup ip routes
      Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    }

  DceManagerHelper dceManager;

  if (useKernel)
    {
      dceManager.SetNetworkStack("ns3::LinuxSocketFdFactory", "Library", StringValue ("libnet-next-2.6.so"));

      AddAddress (nodes.Get (0), Seconds (0.1), "sim0", "10.1.1.1/8");
      RunIp (nodes.Get (0), Seconds (0.2), "link set sim0 up arp off");

      AddAddress (nodes.Get (1), Seconds (0.3), "sim0","10.1.1.2/8");
      RunIp (nodes.Get (1), Seconds (0.4), "link set sim0 up arp off");

    }
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1<<20);

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.1.1.2");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("10");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.7));
  apps.Stop (Seconds (20));

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (0.6));

  setPos (nodes.Get (0), 1, 10, 0);
  setPos (nodes.Get (1), 50,10, 0);

  // Create the animation object and configure for specified output

  AnimationInterface anim (animFile, false);

  anim.StartAnimation ();

  Simulator::Stop (Seconds(40.0));
  Simulator::Run ();
  Simulator::Destroy ();

  anim.StopAnimation ();

  return 0;
}
