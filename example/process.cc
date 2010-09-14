#include "ns3/helper-module.h"
#include "ns3/simulator-module.h"
#include "ns3/core-module.h"
#include <fstream>

using namespace ns3;

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer devices = p2p.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer addresses = ipv4.Assign (devices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  DceManagerHelper processManager;
  processManager.Install (nodes);

  DceApplicationHelper process;
  process.SetBinary ("process-udp-server");
  process.SetStackSize (1<<16);
  ApplicationContainer apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));

  process.SetBinary ("process-udp-client");
  process.AddArgument ("10.1.1.1");
  apps = process.Install (nodes.Get (1));
  apps.Start (Seconds (1.5));

  p2p.EnablePcapAll ("process");
  //std::ofstream ascii;
  //ascii.open ("process.tr");
  //PointToPointHelper::EnableAsciiAll (ascii);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
