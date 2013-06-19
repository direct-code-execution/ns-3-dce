//#include "memory-usage.h"
#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <fstream>
#include <string>


using namespace ns3;

#define PORT 8080

static std::string Ipv4AddressToString (Ipv4Address ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}

void IpRcv (void)
{
  std::cout << "ip_rcv " << std::endl;
}
void IpRcvFinish (void)
{
  std::cout << "ip_rcv_finish " << std::endl;
}

long g_memory;
#if 0
static void PrintMemory (long start)
{
  long now = GetCurrentMemoryUsage ();
  g_memory = now - start;
}
#endif
static Time RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1 << 16);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
  return at + TimeStep (1);
}

static Time AddAddress (Ptr<Node> node, Time at, std::string name, std::string address)
{
  at = RunIp (node, at, "-f inet addr add " + address + " dev " + name);
  return RunIp (node, at, "link set " + name + " up");
}

void
SetupIpStacks (std::string linuxStack, std::string ns3App, NodeContainer nodes)
{
  DceManagerHelper processManager;
  if (linuxStack == "")
    {
      Ipv4StaticRoutingHelper staticHelper;
      InternetStackHelper stack;
      stack.SetRoutingHelper (staticHelper);
      stack.Install (nodes);
    }
  else
    {
      processManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                      "Library", StringValue (linuxStack));
    }
  if (ns3App ==  "false")
    {
      processManager.Install (nodes);
    }
  if (linuxStack != "")
    {
      for (uint32_t i = 0; i < nodes.GetN (); i++)
        {
          Ptr<Node> n = nodes.Get (i);
          RunIp (n, Seconds (0.0), "link set lo up");
        }
    }
}

static std::string Device (Ptr<NetDevice> dev)
{
  std::ostringstream oss;
  oss << "sim" << dev->GetIfIndex ();
  return oss.str ();
}

static std::vector<std::pair<Ptr<Node>, Ipv4Address> >
SetupIpAddresses (std::string linuxStack, NodeContainer nodes, std::vector<NetDeviceContainer> devs)
{
  Time start = Seconds (0.1);
  Time increment = TimeStep (1);
  std::vector<std::pair<Ptr<Node>, Ipv4Address> > pairs;
  Ipv4AddressHelper addressAlloc;
  Ipv4Mask mask = Ipv4Mask ("255.255.255.0");
  addressAlloc.SetBase ("10.1.0.0", mask);
  for (uint32_t i = 0; i < devs.size (); i++)
    {
      addressAlloc.NewNetwork ();
      Ipv4Address ipA = addressAlloc.NewAddress ();
      Ipv4Address ipB = addressAlloc.NewAddress ();
      Ptr<NetDevice> devA = devs[i].Get (0);
      Ptr<NetDevice> devB = devs[i].Get (1);
      Ptr<Node> a = devA->GetNode ();
      Ptr<Node> b = devB->GetNode ();
      if (linuxStack == "")
        {
          Ptr<Ipv4> ipv4A = a->GetObject<Ipv4> ();
          Ptr<Ipv4> ipv4B = b->GetObject<Ipv4> ();
          uint32_t interfA = ipv4A->AddInterface (devA);
          uint32_t interfB = ipv4B->AddInterface (devB);
          ipv4A->AddAddress (interfA, Ipv4InterfaceAddress (ipA, mask));
          ipv4B->AddAddress (interfB, Ipv4InterfaceAddress (ipB, mask));
          ipv4A->SetUp (interfA);
          ipv4B->SetUp (interfB);

          Ptr<Ipv4StaticRouting> routing = DynamicCast<Ipv4StaticRouting> (ipv4A->GetRoutingProtocol ());
          routing->SetDefaultRoute (ipB, interfA, 0);
        }
      else
        {
          start = AddAddress (a, start, Device (devA), Ipv4AddressToString (ipA) + "/24");
          start = AddAddress (b, start, Device (devB), Ipv4AddressToString (ipB) + "/24");
          start = RunIp (a, start, "route add default via " + Ipv4AddressToString (ipB) + " dev " + Device (devA));
        }
      pairs.push_back (std::make_pair (a, ipA));
      if (i == devs.size () - 1)
        {
          pairs.push_back (std::make_pair (b, ipB));
        }
    }
  return pairs;
}


uint16_t g_serverPid = -1;

static void ServerStarted (uint16_t pid)
{
  g_serverPid = pid;
}

double g_sinkRxBytes = 0;

static void SinkReceivedBytes (Ptr<const Packet> p, const Address & from)
{
  g_sinkRxBytes += p->GetSize ();
}


std::string AsString (uint32_t i)
{
  std::ostringstream oss;
  oss << i;
  return oss.str ();
}

long AsNumber (std::string s)
{
  std::istringstream iss;
  iss.str (s);
  long v;
  iss >> v;
  return v;
}

static long ReadTotalBytesReceived (uint32_t node)
{
  std::ifstream is;
  std::string filename = "files-" + AsString (node) + "/var/log/" + AsString (g_serverPid) + "/stdout";
  is.open (filename.c_str (), std::ios_base::in);
  std::string line, prev;
  while (getline (is, line))
    {
      prev = line;
    }
  // now, parse the last line.
  std::string::size_type i = prev.rfind (" ");
  NS_ASSERT (i != std::string::npos);
  std::string bytes = prev.substr (i + 1, prev.size () - (i + 1));
  is.close ();
  return AsNumber (bytes);
}

int main (int argc, char *argv[])
{
  //HookManager hooks;
  //hooks.AddHookBySourceAndFunction ("ip-input.c", "ip_rcv", &IpRcv);
  //hooks.AddHookBySourceAndFunction ("ip-input.c", "ip_rcv_finish", &IpRcvFinish);
  //hooks.AddHookBySourceAndFunction ("ip-output.c", "ip_local_out", &IpLocalOut);
  //hooks.AddHookBySourceAndFunction ("ip-output.c", "nf_hook_slow", &IpLocalOut);
  //hooks.AddHookBySourceAndFunction ("af_inet.c", "inet_sendmsg", &IpLocalOut);
  // udp_sendmsg udp.c
  // ip_route_output_flow
  // ip_output
  // ip_finish_output
  // ip_finish_output2
  // neigh_resolve_output
  // send_arp
  // arp_xmit
  // arp_mangle
  // dev_hard_start_xmit
  // ip_append_data
  // udp_push_pending_frames
  // ip_push_pending_frames
  // udp_flush_pending_frames
  // ip_local_out
  // ipv4_conntrack_defrag
  // ipv4_conntrack_local
  // ipv4_confirm
  // dst_output
  // netif_rx
  // net_rx_action
  // process_backlog
  // __netif_receive_skb
  // ipv6_rcv
  // arp_rcv
  // ip_rcv
  // arp_process

//  long memoryAtStart = GetCurrentMemoryUsage ();
  SystemWallClockMs clock;

  clock.Start ();

  Config::SetDefault ("ns3::Ipv4L3Protocol::DefaultTtl", UintegerValue (255));
  std::string delay = "1ns";
  std::string rate = "5Mbps";
  std::string packetSize = "1000";
  std::string packetRate = "1000000";
  std::string linuxStack = "";
  std::string ns3App = "false";
  Time duration = Seconds (10);
  std::string pcap = "no";
  uint32_t size = 1;
  CommandLine cmd;
  cmd.AddValue ("Duration", "simulation duration", duration);
  cmd.AddValue ("Pcap", "enable pcap: yes or no", pcap);
  cmd.AddValue ("Delay", "link delay", delay);
  cmd.AddValue ("LinkBandwidth", "link bandwidth", rate);
  cmd.AddValue ("Size", "number of nodes in ring", size);
  cmd.AddValue ("TxPacketSize", "size of packets sent (bytes)", packetSize);
  cmd.AddValue ("TxBandwidth", "rate at which packets are sent (in bytes per second)", packetRate);
  cmd.AddValue ("Ns3App", "Use an ns-3 application or posix one", ns3App);
  cmd.AddValue ("LinuxPath", "path to linux network stack", linuxStack);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (size);

  SetupIpStacks (linuxStack, ns3App, nodes);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (rate));
  csma.SetChannelAttribute ("Delay", StringValue (delay));
  std::vector<NetDeviceContainer> devs;
  for (uint32_t i = 0; i < size - 1; i++)
    {
      NodeContainer linkNodes;
      linkNodes.Add (nodes.Get (i));
      linkNodes.Add (nodes.Get (i + 1));
      NetDeviceContainer dev = csma.Install (linkNodes);
      devs.push_back (dev);
    }

  Ptr<Node> clientNode, serverNode;
  Ipv4Address serverIp;
  if (size > 1)
    {
      std::vector<std::pair<Ptr<Node>, Ipv4Address> > pairs;
      pairs = SetupIpAddresses (linuxStack, nodes, devs);
      clientNode = pairs[0].first;
      serverNode = pairs[pairs.size () - 1].first;
      serverIp = pairs[pairs.size () - 1].second;
    }
  else
    {
      clientNode = nodes.Get (0);
      serverNode = nodes.Get (0);
      serverIp = Ipv4Address ("127.0.0.1");
    }

  if (ns3App == "false")
    {
      DceApplicationHelper process;
      ApplicationContainer apps;
      process.SetStackSize (1 << 16);
      process.SetBinary ("udp-perf");

      // setup client
      process.AddArgument ("--client");
      process.AddArgument ("--host=" + Ipv4AddressToString (serverIp));
      process.AddArgument ("--pktsize=" + packetSize);
      process.AddArgument ("--bandwidth=" + packetRate);
      apps = process.Install (clientNode);
      apps.Start (Seconds (4.0));

      // setup server
      process.ResetArguments ();
      apps = process.Install (serverNode);
      apps.Start (Seconds (4.0));
      apps.Get (0)->TraceConnectWithoutContext ("ProcessStarted", MakeCallback (&ServerStarted));
    }
  else
    {
      ApplicationContainer apps;
      OnOffHelper onoff = OnOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (serverIp, PORT));
      onoff.SetAttribute ("OnTime", StringValue ("Constant:1.0"));
      onoff.SetAttribute ("OffTime", StringValue ("Constant:0.0"));
      onoff.SetAttribute ("PacketSize", StringValue (packetSize));
      onoff.SetAttribute ("DataRate", StringValue (packetRate + "Bps"));
      apps = onoff.Install (clientNode);
      apps.Start (Seconds (4.0));

      PacketSinkHelper sink = PacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), PORT));
      apps = sink.Install (serverNode);
      apps.Start (Seconds (3.9999));
      apps.Get (0)->TraceConnectWithoutContext ("Rx", MakeCallback (&SinkReceivedBytes));
    }

  if (pcap == "yes")
    {
      csma.EnablePcapAll ("ring-udp-perf");
    }

//  Simulator::Schedule (Seconds (4.00000), &PrintMemory, memoryAtStart);

  Simulator::Stop (duration);
  Simulator::Run ();

  Simulator::Destroy ();

  clock.End ();
  double elapsedMs = clock.GetElapsedReal ();
  double bytes;
  if (ns3App == "false")
    {
      bytes = ReadTotalBytesReceived (size - 1);
    }
  else
    {
      bytes = g_sinkRxBytes;
    }
  double pktSize = AsNumber (packetSize);
  double pps = bytes / pktSize / elapsedMs * 1000.0;

  std::cout << "packet size(bytes),n nodes,pps,memory(kbytes)" << std::endl;
  std::cout << packetSize << "," << size << "," << pps << "," << g_memory << std::endl;

  return 0;
}
