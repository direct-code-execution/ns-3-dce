#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ccnx/misc-tools.h"

/**
 * Comment to use iperf2 instead
 */
#define IPERF3

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
//
// Note : Tested with iperf 3.1, you need to modify source as is done in 
//        utils/iperf3_1.patch
// ===========================================================================



// duration of the iperf in seconds (s)
const std::string iperfDuration =  "5";

int main (int argc, char *argv[])
{
  Ptr<Node> clientNode;
  Ptr<Node> serverNode;
  std::string stack = "ns3";
  bool useUdp = 0;
  std::string bandWidth = "1m";
  std::string windowSize = "120KB";
  const Time simMaxDuration = Seconds(40);
  CommandLine cmd;
  cmd.AddValue ("stack", "Name of IP stack: ns3/linux/freebsd.", stack);
  cmd.AddValue ("udp", "Use UDP. Default false (0)", useUdp);
  cmd.AddValue ("bw", "BandWidth. Default 1m.", bandWidth);
  cmd.AddValue ("window", "iperf --window parameter", windowSize);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);
  clientNode = nodes.Get(0);
  serverNode = nodes.Get(1);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices, devices2;
  devices = pointToPoint.Install (nodes);
  devices2 = pointToPoint.Install (nodes);

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
  address.SetBase ("10.1.2.0", "255.255.255.252");
  interfaces = address.Assign (devices2);

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
  std::ostringstream oss;

  dce.SetStackSize (1 << 20);


        //! 'i+1' because 0 is localhost
        Ipv4Address serverAddr = serverNode->GetObject<Ipv4>()->GetAddress(i+1, 0).GetLocal();
        Ipv4Address sourceAddr = clientNode->GetObject<Ipv4>()->GetAddress(i+1, 0).GetLocal();

        //! TODO, we should be able to not specify a port but it seems buggy so for now, let's set a port
      //  InetSocketAddress local( sourceAddr);
        InetSocketAddress local(sourceAddr, 0);
        InetSocketAddress remote(serverAddr, 5001);
  #ifdef IPERF3
  // Setup client on node 0
  dce.SetBinary ("iperf3");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.2.0.1");
  dce.AddArgument ("--interval=1"); // interval between reports
  dce.AddArgument ("--time=10");  // duration of the test
  dce.AddArgument ("--verbose");
  dce.AddArgument ("--json");   // export to json
  dce.AddArgument ("--logfile=client.res");  // into this file
//  dce.AddArgument ("-P");   // number of streams to run in parallel
//  dce.AddArgument ("1");

  apps = dce.Install ( clientNode );
  apps.Start (Seconds (5.0));
  apps.Stop (simMaxDuration);

  // Launch iperf server on node 1
  dce.SetBinary ("iperf3");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("--verbose");
  dce.AddArgument ("--json");   // export to json
  dce.AddArgument ("--logfile=server.res");  // into this file
  dce.AddArgument ("--server");
  if (useUdp)
  {
    dce.AddArgument ("--udp");
  }

  apps = dce.Install (serverNode);
  #else
  /* By default iperf2 listens on port 5001 */
  oss.str("");
  oss << "--window=" << windowSize;

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("--client=10.2.0.1");
//  dce.AddArgument ("");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument (iperfDuration);
  dce.AddArgument ("--bind=10.1.0.1");  // TODO get address from clientNode
  dce.AddArgument ("--reportstyle=C");  // export as CSV
  dce.AddArgument (oss.str());   // size of Rcv or send buffer

  apps = dce.Install ( clientNode );
  apps.Start (Seconds (5.0));
  apps.Stop (simMaxDuration);

  // Launch iperf server on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("--bind=10.2.0.1");  //TODO get address programmatacilly from clientNode
  dce.AddArgument ("--parallel=1");
  dce.AddArgument (oss.str());   // size of Rcv or send buffer
  if (useUdp)
    {
      dce.AddArgument ("--udp");
    }

  apps = dce.Install (serverNode);
  #endif


  pointToPoint.EnablePcapAll ("iperf-" + stack, false);

  apps.Start (Seconds (0.6));

  setPos (clientNode, 1, 10, 0);
  setPos (serverNode, 50,10, 0);

  Simulator::Stop (simMaxDuration);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
