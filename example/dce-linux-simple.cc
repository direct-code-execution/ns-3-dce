#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/dce-module.h"

using namespace ns3;

void finished (int *pstatus, uint16_t pid, int status)
{
  *pstatus = status;
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (1);

  DceManagerHelper dceManager;
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;
  dce.SetBinary ("ip");
  dce.SetStackSize (1 << 16);
  int st;
  dce.SetFinishedCallback (MakeBoundCallback (&finished, &st));
  dce.ResetArguments ();
  dce.ParseArguments ("-f inet addr add local 127.0.0.1/8 scope host dev lo");
  apps = dce.Install (nodes.Get (0));

  apps.Start (Seconds (2.0));
  dce.ResetArguments ();
  dce.ParseArguments ("link set lo up");
  dce.SetFinishedCallback (MakeNullCallback<void,uint16_t,int> ());
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (3.0));
  dce.ResetArguments ();
  dce.ParseArguments ("route list table all");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (3.1));

  dce.ResetArguments ();
  dce.ParseArguments ("addr show dev lo");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (3.0));

  dce.SetBinary ("udp-server");
  dce.SetBinary ("tcp-server");
  dce.ResetArguments ();
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  dce.SetBinary ("udp-client");
  dce.SetBinary ("tcp-client");
  dce.ResetArguments ();
  dce.AddArgument ("127.0.0.1");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.5));

  Simulator::Stop (Seconds (1000100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
