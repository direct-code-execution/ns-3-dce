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

  dce.SetStackSize (1 << 20);

  dce.SetBinary ("udp-server");
  dce.ResetArguments ();
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  dce.SetBinary ("udp-client");
  dce.ResetArguments ();
  dce.AddArgument ("127.0.0.1");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.5));

  Simulator::Stop (Seconds (1000100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
