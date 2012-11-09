// ===========================================================================
//
//  Launch the same executable on two different nodes.
//
//  Note: There is a topology but the network is not used
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
int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  DceManagerHelper dceManager;

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer app[2];

  dce.SetStackSize (1 << 20);
  // Launch the first
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("mytest");
  app[0] = dce.Install (nodes.Get (0));
  app[0].Start (Seconds (10));

  // Launch the second
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("mytest");
  app[1] = dce.Install (nodes.Get (1));
  app[1].Start (Seconds (20));

  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}

