#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/dce-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer devices = csma.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType",
                                      StringValue ("UcontextFiberManager"));
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                              "Library", StringValue ("liblinux.so"));

  LinuxStackHelper stack;
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  dceManager.Install (nodes);

  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetBinary ("dccp-server");
  dce.SetStackSize (1 << 16);
  dce.ResetArguments ();
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  dce.SetBinary ("dccp-client");
  dce.SetStackSize (1 << 16);
  dce.ResetArguments ();
  dce.AddArgument ("10.0.0.1");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (4.5));

  csma.EnablePcapAll ("dce-dccp");

  Simulator::Stop (Seconds (1000100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
