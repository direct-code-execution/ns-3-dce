/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Two nodes running Linux (LKL) kernels on a point-to-point link: the
 * kernels' sim0 devices are configured with ip, then a TCP and a UDP
 * client on node 0 send to servers on node 1. Every program must exit
 * with status 0.
 *
 * With --loopback, a single node runs the TCP client and server over its
 * loopback device instead (like dce-linux-simple).
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/dce-module.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace ns3;

static uint32_t g_started = 0;
static uint32_t g_succeeded = 0;

// A program succeeds if it exits with 0 and, for the servers, if it
// received everything the client sent.
static void
Finished (std::string name, uint32_t node, uint16_t pid, int status)
{
  bool ok = status == 0;
  if (ok && name.find ("server") != std::string::npos)
    {
      std::ostringstream path;
      path << "files-" << node << "/var/log/" << pid << "/stdout";
      std::ifstream file (path.str ());
      std::stringstream out;
      out << file.rdbuf ();
      ok = out.str ().find ("did read all buffers") != std::string::npos;
    }
  std::cout << Simulator::Now ().GetSeconds () << "s " << name << " exited with " << status
            << (ok ? "" : " FAILED") << std::endl;
  if (ok)
    {
      g_succeeded++;
    }
}

static void
Run (Ptr<Node> node, Time at, std::string binary, std::string args, bool check = true)
{
  DceApplicationHelper dce;
  dce.SetBinary (binary);
  dce.SetStackSize (1 << 16);
  dce.ResetArguments ();
  if (!args.empty ())
    {
      dce.ParseArguments (args.c_str ());
    }
  if (check)
    {
      dce.SetFinishedCallback (MakeBoundCallback (&Finished, binary + " " + args, node->GetId ()));
      g_started++;
    }
  ApplicationContainer apps = dce.Install (node);
  apps.Start (at);
}

int
main (int argc, char *argv[])
{
  bool loopback = false;
  bool pcap = false;
  CommandLine cmd;
  cmd.AddValue ("loopback", "Use one node and its loopback device", loopback);
  cmd.AddValue ("pcap", "Write pcap traces of the link", pcap);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (loopback ? 1 : 2);
  if (!loopback)
    {
      PointToPointHelper p2p;
      p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
      p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
      p2p.Install (nodes);
      if (pcap)
        {
          p2p.EnablePcapAll ("dce-lkl-p2p");
        }
    }
  DceManagerHelper dceManager;
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory");
  dceManager.Install (nodes);

  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      std::ostringstream addr;
      addr << "-f inet addr add 10.0.0." << i + 1 << "/24 dev sim0";
      Run (nodes.Get (i), Seconds (0.1), "ip", "link set lo up");
      if (!loopback)
        {
          Run (nodes.Get (i), Seconds (0.2), "ip", addr.str ());
          Run (nodes.Get (i), Seconds (0.3), "ip", "link set sim0 up");
        }
      Run (nodes.Get (i), Seconds (0.5), "ip", "addr show", false);
      Run (nodes.Get (i), Seconds (0.5), "ip", "route show", false);
    }

  Ptr<Node> server = nodes.Get (nodes.GetN () - 1);
  std::string serverAddr = loopback ? "127.0.0.1" : "10.0.0.2";
  Run (server, Seconds (1.0), "tcp-server", "");
  Run (nodes.Get (0), Seconds (1.5), "tcp-client", serverAddr);
  if (!loopback)
    {
      Run (server, Seconds (1.0), "udp-server", "");
      Run (nodes.Get (0), Seconds (1.5), "udp-client", serverAddr);
    }

  // udp-client sends one datagram per second, 1000 times.
  Simulator::Stop (Seconds (1100));
  Simulator::Run ();
  Simulator::Destroy ();

  bool ok = g_succeeded == g_started;
  std::cout << (ok ? "OK" : "FAILED") << " (" << g_succeeded << "/" << g_started
            << " programs succeeded)" << std::endl;
  return ok ? 0 : 1;
}
