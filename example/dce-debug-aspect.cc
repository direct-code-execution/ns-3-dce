#include "ns3/network-module.h"
#include "ns3/dce-module.h"
#include "ns3/core-module.h"
#include <string>

#include <hook-manager.h>

using namespace ns3;
using namespace aspcpp;

NS_LOG_COMPONENT_DEFINE ("DceDebugAspect");

static uint64_t g_ip_rcv = 0;
static uint64_t g_proc_backlog = 0;
static uint64_t g_arp_xmit = 0;

void IpRcv (void)
{
  NS_LOG_DEBUG ("ip_rcv ");
  g_ip_rcv++;
}
void ProcBacklog (void)
{
  NS_LOG_DEBUG ("process_backlog ");
  g_proc_backlog++;
}
void ArpXmit (void)
{
  NS_LOG_DEBUG ("arp_xmit ");
  g_arp_xmit++;
}



int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  HookManager hooks;
  hooks.AddHookBySourceAndFunction ("ip_input.c", "::ip_rcv", &IpRcv);
  hooks.AddHookByFunction ("::process_backlog", &ProcBacklog);
  hooks.AddHookByFunction ("::arp_xmit", &ArpXmit);
  // 
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
  dce.ResetArguments ();
  dce.ParseArguments ("-f inet addr add local 127.0.0.1/8 scope host dev lo");
  apps = dce.Install (nodes.Get (0));

  apps.Start (Seconds (2.0));
  dce.ResetArguments ();
  dce.ParseArguments ("link set lo up");
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

  std::cout << "called : ip_rcv " << g_ip_rcv <<
    " process_backlog " << g_proc_backlog <<
    " arp_xmit " << g_arp_xmit << std::endl;
    
  return 0;
}
