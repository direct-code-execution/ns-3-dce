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
  nodes.Create (1);

  DceManagerHelper processManager;
  processManager.SetNetworkStack("ns3::LinuxSocketFdFactory",
				 "Library", StringValue ("libnet-next-2.6.so"));
  //processManager.SetLoader("ns3::CopyLoaderFactory");
  processManager.Install (nodes);

  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("build/debug/ip");
  process.SetStackSize (1<<16);
#if 0
  process.ParseArguments("link show");
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));
#endif
#if 1
  process.ResetArguments();
  process.ParseArguments("-f inet addr add local 127.0.0.1/8 scope host dev lo");
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (2.0));
  process.ResetArguments();
  process.ParseArguments("link set lo up");
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (3.0));
  process.ResetArguments();
  process.ParseArguments("route list table all");
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (3.1));
#endif

  process.ResetArguments();
  process.ParseArguments("addr show dev lo");
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (3.0));

#if 0
  process.SetBinary ("build/debug/examples/process/process-udp-server");
  process.ResetArguments();
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  process.SetBinary ("build/debug/examples/process/process-udp-client");
  process.ResetArguments();
  process.AddArgument ("127.0.0.1");
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (4.5));
#else
  process.SetBinary ("build/debug/examples/process/process-tcp-loopback");
  process.ResetArguments();
  apps = process.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));
#endif

  Simulator::Stop (Seconds(1000100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
