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
NS_LOG_COMPONENT_DEFINE ("DceLinux");

static void RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1 << 16);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

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
  nodes.Create (2);

  NetDeviceContainer devices;

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  devices = p2p.Install (nodes);

  DceManagerHelper processManager;
  // processManager.SetLoader ("ns3::DlmLoaderFactory");
  //  processManager.SetLoader ("ns3::CopyLoaderFactory");
  processManager.SetTaskManagerAttribute ("FiberManagerType",
                                          StringValue ("UcontextFiberManager"));
  processManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
  LinuxStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  processManager.Install (nodes);

  for (int n = 0; n < 2; n++)
    {
      RunIp (nodes.Get (n), Seconds (0.2), "link show");
      RunIp (nodes.Get (n), Seconds (0.3), "route show table all");
      RunIp (nodes.Get (n), Seconds (0.4), "addr list");
    }

  DceApplicationHelper process;
  ApplicationContainer apps;

  RunIp (nodes.Get (0), Seconds (0.4), "address add 2001:db8::a/64 dev sim0");
  RunIp (nodes.Get (0), Seconds (0.4), "xfrm policy add dir out src 2001:db8::a dst 2001:db8::b tmpl proto esp");
  RunIp (nodes.Get (0), Seconds (0.4), "xfrm state add src 2001:db8::a dst 2001:db8::b proto esp spi 1 enc 'cbc(aes)' 0x3ed0af408cf5dcbf5d5d9a5fa806b224");
  RunIp (nodes.Get (0), Seconds (0.4), "xfrm state add src 192.168.2.100 dst 192.168.1.10 proto esp spi 0x00000301 mode tunnel  auth md5 0x96358c90783bbfa3d7b196ceabe0536b enc des3_ede 0xf6ddb555acfd9d77b03ea3843f2653255afe8eb5573965df");
  RunIp (nodes.Get (0), Seconds (0.4), "xfrm state get mark 7 src 192.168.2.100 dst 192.168.1.10 proto esp spi 0x00000301");
  RunIp (nodes.Get (0), Seconds (0.4), "xfrm policy");
  RunIp (nodes.Get (0), Seconds (0.4), "xfrm state");



  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
