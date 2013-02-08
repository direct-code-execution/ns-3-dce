#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"

using namespace ns3;

void
CreateReadme ()
{
  std::ofstream osf ("/tmp/README", std::fstream::trunc);

  osf << "The wanted data is here :)" ;

  osf.close ();
}

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

  CcnClientHelper dce;
  ApplicationContainer apps, putter, getter;

  dce.SetStackSize (1 << 20);

  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.AddEnvironment ("CCND_CAP", "50000");
  dce.AddEnvironment ("CCND_DEBUG", "-1");
  dce.AddEnvironment ("CCN_LOCAL_PORT", "");

  dce.AddEnvironment ("CCND_CAP", "");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.0));

  dce.ResetArguments ();
  dce.SetBinary ("ccnpoke");
  dce.SetStdinFile ("/tmp/README");
  dce.AddFile ("/tmp/README", "/tmp/README");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment ("HOME", "/root");

  putter = dce.Install (nodes.Get (0));
  putter.Start (Seconds (1.0));

  CreateReadme ();
  dce.ResetArguments ();
  dce.SetBinary ("ccnpeek");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("ccnx:/LeReadme");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (2.0));

  // Stop ccnd
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (59.0));

  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
