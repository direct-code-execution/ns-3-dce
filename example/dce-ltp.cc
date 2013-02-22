#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include <fstream>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceLtp");

const char *bins[] = {
  "in6_01",
  "asapi_05",
  //               "getaddrinfo_01",
  "in6_02",
  //               "asapi_06",
  "asapi_07",
  "asapi_03",
  //               "asapi_04",
  "asapi_02",
  "asapi_01"
};

void
PrintTcpFlags (std::string key, std::string value)
{
  NS_LOG_INFO (key << "=" << value);
}

int main (int argc, char *argv[])
{
  CommandLine cmd;

  cmd.Parse (argc, argv);
  NodeContainer nodes;
  nodes.Create (1);

  NetDeviceContainer devices;

  DceManagerHelper processManager;
  // processManager.SetLoader ("ns3::DlmLoaderFactory");
  //  processManager.SetLoader ("ns3::CopyLoaderFactory");
  processManager.SetTaskManagerAttribute ("FiberManagerType",
                                          StringValue ("UcontextFiberManager"));
  processManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
  LinuxStackHelper stack;
  stack.Install (nodes);

  // Ipv4AddressHelper address;
  // address.SetBase ("10.0.0.0", "255.255.255.0");
  // Ipv4InterfaceContainer interfaces = address.Assign (devices);

  processManager.Install (nodes);

  for (uint32_t n = 0; n < nodes.GetN (); n++)
    {
      //      RunIp (nodes.Get (n), Seconds (0.2), "link set lo up");
      // RunIp (nodes.Get (n), Seconds (0.2), "link show");
      // RunIp (nodes.Get (n), Seconds (0.3), "route show table all");
      // RunIp (nodes.Get (n), Seconds (0.4), "addr list");
    }

  DceApplicationHelper process;
  ApplicationContainer apps;

  for (int i = 0; i < 7; i++)
    {
      process.SetBinary (bins[i]);
      process.SetUid (1000);
      process.ResetArguments ();
      process.SetStackSize (1 << 16);
      //  process.ParseArguments ("-L");
      //process.ParseArguments ("10.0.0.1");
      apps = process.Install (nodes.Get (0));
      apps.Start (Seconds (1.5 + i));
    }

  // print tcp sysctl value
  LinuxStackHelper::SysctlGet (nodes.Get (0), Seconds (1.0),
                               ".net.ipv4.tcp_available_congestion_control", &PrintTcpFlags);

  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
