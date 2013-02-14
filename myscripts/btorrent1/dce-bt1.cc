// ===========================================================================
//
//   Topology
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
//                100 Mbps, 1 ms
//
// This experience do bittorent !
//
//  Node 0 : server
//
// ===========================================================================

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/trace-helper.h"
#include <sys/stat.h>
#include <sys/types.h>

using namespace ns3;
void
CreateFiles ()
{
  std::ofstream osf ("files-0/index.html", std::fstream::trunc);
  osf << "<HTML><HEAD><TITLE>Hello</TITLE></HEAD><BODY><H3>HELLO</H3>"
      << "<P>is all right ?</BODY></HTML>" << std::endl;
  osf.close ();
}
int
main (int argc, char *argv[])
{
  bool useKernel = 0;

  CommandLine cmd;
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.Parse (argc, argv);

  mkdir ("files-0",0744);
  mkdir ("files-0/home/",0744);
  mkdir ("files-0/home/fred",0744);
  mkdir ("files-0/home/fred",0744);

//  CreateFiles ();

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  DceManagerHelper dceManager;
//  dceManager.SetTaskManagerAttribute( "FiberManagerType", StringValue ( "UcontextFiberManager" ) );

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
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // setup ip routes
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  dceManager.Install (nodes);


  DceApplicationHelper dce;
  ApplicationContainer server, client;

  dce.SetStackSize (1 << 20);

  // Launch the tracker
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("opentracker");
  dce.AddEnvironment ("HOME", "/home/fred");
  server = dce.Install (nodes.Get (0));
  server.Start (Seconds (1));

  // Launch the server
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("client_test");
  dce.AddArgument ("mycat.tor");
  server = dce.Install (nodes.Get (0));
  server.Start (Seconds (2));

  // Launch the client
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("client_test");
  dce.AddArgument ("mycat.tor");
  server = dce.Install (nodes.Get (1));
  server.Start (Seconds (3));

  pointToPoint.EnablePcapAll ("tor1", false);

  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}

