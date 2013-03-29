#include "ns3/core-module.h"
#include "ns3/dce-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  NodeContainer nodes;
  nodes.Create (1);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1<<20);

  dce.SetBinary ("tenseconds");
  dce.ResetArguments ();
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.0));

  Simulator::Stop (Seconds(30.0));
  Simulator::Run ();
  Simulator::Destroy ();
}
