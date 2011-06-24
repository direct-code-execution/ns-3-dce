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
  ApplicationContainer apps, putter, getter;

  dce.SetStackSize (1<<20);

  dce.SetBinary ("ccnd");
  dce.ResetArguments();
  dce.ResetEnvironment();

  dce.AddEnvironment("CCND_CAP", "50000");
  dce.AddEnvironment("CCND_DEBUG", "-1");
  dce.AddEnvironment("CCN_LOCAL_PORT", "");

  dce.AddEnvironment("CCND_CAP", "");
  dce.AddEnvironment("CCND_AUTOREG", "");
  dce.AddEnvironment("CCND_LISTEN_ON", "");
  dce.AddEnvironment("CCND_MTU", "");
  dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  dce.ResetArguments();
//  dce.ResetEnvironment();
  dce.SetBinary ("ccnput");
  dce.SetStdinFile ("/tmp/README");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment("HOME", "/home/furbani");

  putter = dce.Install (nodes.Get (0));
  putter.Start (Seconds (5.0));

  dce.ResetArguments();
//  dce.ResetEnvironment();
  dce.SetBinary ("ccnget");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("ccnx:/LeReadme");

  getter = dce.Install (nodes.Get (0));
  getter.Start (Seconds (6.0));


  Simulator::Stop (Seconds(60.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
