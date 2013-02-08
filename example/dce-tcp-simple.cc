#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (1);

  InternetStackHelper stack;
  stack.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetBinary ("tcp-loopback");
  dce.SetStackSize (1 << 20);
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  Simulator::Stop (Seconds (1000100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
