// Test Local Socket (AF_UNIX)

#include "ns3/helper-module.h"
#include "ns3/simulator-module.h"
#include "ns3/core-module.h"

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

  dce.SetBinary ("unix-server");
  dce.ResetArguments ();
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  dce.SetBinary ("unix-client");
  dce.ResetArguments ();
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.1));

  Simulator::Stop (Seconds (1000.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
