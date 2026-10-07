/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Real ffmpeg streaming real video over simulated Wi-Fi, with the real
// Linux kernel stack, all inside ns-3 through DCE.
//
//   node 0 (server STA)         node 1 (AP)          node 2 (client STA)
//   +-------------------+   +--------------+   +------------------------+
//   | ffmpeg -re ... udp|   |              |   | ffmpeg -i udp://...    |
//   +-------------------+   |              |   | -c copy -f mpegts      |
//   | Linux 4.4 (DCE)   |   | Linux 4.4    |   |   /stream.ts           |
//   +-------------------+   +--------------+   +------------------------+
//   |  802.11n  10.1.1.1|   | 10.1.1.3     |   | 10.1.1.2  802.11n      |
//   +-------------------+   +--------------+   +------------------------+
//        )))))))))))))))))))))     (((((((((((((((((((((((((
//
// The server reads files-0/video.ts (a symlink to --video) at its native
// frame rate (-re) and sends it as MPEG-TS over UDP to the client. The
// client writes the stream it receives to files-2/stream.ts. With --fifo=1
// (default) that path is a named pipe, so a player on the host can watch
// the stream live while the simulation runs, either started by hand
//
//     vlc --play-and-exit - < files-2/stream.ts
//
// or by the example itself: --player="vlc --play-and-exit - < {}".
// With --realtime=1 (default) ns-3 runs in real-time mode, so the ffmpeg
// pacing inside the simulation matches the wall clock and the video plays
// at normal speed. With --realtime=0 --fifo=0 the example runs as fast as
// possible and finally checks that the received elementary streams are
// identical to the sent ones (exit status 1 otherwise): this is how test.py
// runs it.
//
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceWifiVideo");

static uint64_t g_rxBytes = 0;
static uint64_t g_rxPackets = 0;
static uint64_t g_lastRxBytes = 0;

static void
ClientMacRx (Ptr<const Packet> p)
{
  g_rxBytes += p->GetSize ();
  g_rxPackets++;
}

static void
Progress (Time interval)
{
  double kbps = (g_rxBytes - g_lastRxBytes) * 8.0 / interval.GetSeconds () / 1000.0;
  g_lastRxBytes = g_rxBytes;
  std::cout << "t=" << std::fixed << std::setprecision (1) << Simulator::Now ().GetSeconds ()
            << "s  client STA MAC rx: " << g_rxPackets << " frames, "
            << g_rxBytes / 1024 << " KiB total, " << std::setprecision (0) << kbps
            << " kbit/s over the last " << interval.GetSeconds () << "s" << std::endl;
  Simulator::Schedule (interval, &Progress, interval);
}

// Extract the elementary streams of an MPEG-TS file: the concatenated PES
// payloads of every PID that carries PES packets (PSI tables, PCR-only and
// null packets are skipped). A stream copy through ffmpeg re-multiplexes the
// transport layer (new PAT/PMT/PCR, continuity counters, PES headers) but the
// elementary streams must come out bit-exact, so that is what we compare.
static std::map<uint16_t, std::string>
ExtractElementaryStreams (const std::string &path, uint64_t &fileBytes)
{
  std::map<uint16_t, std::string> es;
  std::map<uint16_t, bool> isPes;
  std::ifstream f (path.c_str (), std::ios::binary);
  char pkt[188];
  fileBytes = 0;
  while (f.read (pkt, sizeof (pkt)))
    {
      fileBytes += sizeof (pkt);
      const uint8_t *p = (const uint8_t *)pkt;
      if (p[0] != 0x47)
        {
          continue; // lost sync; ignore this packet
        }
      bool pusi = p[1] & 0x40;
      uint16_t pid = ((p[1] & 0x1f) << 8) | p[2];
      uint8_t afc = (p[3] >> 4) & 3;
      size_t off = 4;
      if (afc & 2)
        {
          off += 1 + p[4]; // adaptation field
        }
      if (!(afc & 1) || off >= sizeof (pkt) || pid == 0x1fff)
        {
          continue; // no payload, or null packet
        }
      if (pusi)
        {
          if (isPes.find (pid) == isPes.end ())
            {
              // PES packets start with the 00 00 01 start code, PSI sections
              // start with a pointer field.
              isPes[pid] = (off + 3 <= sizeof (pkt) && p[off] == 0 && p[off + 1] == 0 && p[off + 2] == 1);
            }
          if (!isPes[pid])
            {
              continue;
            }
          if (off + 6 > sizeof (pkt))
            {
              continue;
            }
          uint8_t streamId = p[off + 3];
          off += 6; // start code, stream id, PES packet length
          bool hasHeader = !(streamId == 0xbc || streamId == 0xbe || streamId == 0xbf
                             || streamId == 0xf0 || streamId == 0xf1 || streamId == 0xff
                             || streamId == 0xf2 || streamId == 0xf8);
          if (hasHeader)
            {
              if (off + 3 > sizeof (pkt))
                {
                  continue;
                }
              off += 3 + p[off + 2]; // flags, flags, header data length, header data
            }
          if (off > sizeof (pkt))
            {
              continue;
            }
        }
      else if (!isPes[pid])
        {
          continue;
        }
      es[pid].append (pkt + off, sizeof (pkt) - off);
    }
  return es;
}

// Returns true when both files carry the same set of elementary streams.
static bool
CompareStreams (const std::string &input, const std::string &output)
{
  uint64_t inBytes, outBytes;
  std::map<uint16_t, std::string> in = ExtractElementaryStreams (input, inBytes);
  std::map<uint16_t, std::string> out = ExtractElementaryStreams (output, outBytes);
  std::cout << "Input  " << input << ": " << inBytes << " bytes, " << in.size () << " elementary streams" << std::endl;
  std::cout << "Output " << output << ": " << outBytes << " bytes, " << out.size () << " elementary streams" << std::endl;
  // PIDs may be renumbered by the muxer: compare the streams as sets.
  std::multiset<std::string> inSet, outSet;
  for (std::map<uint16_t, std::string>::iterator i = in.begin (); i != in.end (); ++i)
    {
      std::cout << "  input  PID 0x" << std::hex << i->first << std::dec << ": " << i->second.size () << " bytes" << std::endl;
      inSet.insert (i->second);
    }
  for (std::map<uint16_t, std::string>::iterator i = out.begin (); i != out.end (); ++i)
    {
      std::cout << "  output PID 0x" << std::hex << i->first << std::dec << ": " << i->second.size () << " bytes" << std::endl;
      outSet.insert (i->second);
    }
  bool same = (!inSet.empty () && inSet == outSet);
  std::cout << (same ? "PASS: the received elementary streams are identical to the sent ones"
                : "FAIL: the received elementary streams differ from the sent ones") << std::endl;
  return same;
}

static void
AddArguments (DceApplicationHelper &dce, const std::string &args)
{
  std::istringstream is (args);
  std::string arg;
  while (is >> arg)
    {
      dce.AddArgument (arg);
    }
}

static void
SetPosition (Ptr<Node> node, double x, double y)
{
  Ptr<ConstantPositionMobilityModel> m = CreateObject<ConstantPositionMobilityModel> ();
  m->SetPosition (Vector (x, y, 0));
  node->AggregateObject (m);
}

int
main (int argc, char *argv[])
{
  std::string video = "video.ts";
  std::string stack = "linux";
  std::string player = "";
  bool realtime = true;
  bool fifo = true;
  double distance = 10.0;
  double stopTime = 60.0;
  uint16_t port = 5004;
  bool pcap = true;
  bool ping = true;
  std::string fiber = "ucontext";
  std::string txArgs = "";
  std::string rxArgs = "";
  std::string rxInArgs = "";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("video", "MPEG-TS file streamed by the server (any container ffmpeg can read as-is)", video);
  cmd.AddValue ("stack", "IP stack on the DCE nodes: linux (real kernel) or ns3", stack);
  cmd.AddValue ("realtime", "Run ns-3 in real-time mode so that the stream plays at wall-clock speed", realtime);
  cmd.AddValue ("fifo", "Write the received stream to a named pipe (watch it live) instead of a regular file", fifo);
  cmd.AddValue ("player", "Host command run while the simulation runs to watch files-2/stream.ts; "
                "{} is replaced by that path, otherwise it is appended (e.g. --player=vlc)", player);
  cmd.AddValue ("distance", "Distance in meters between the AP and the client STA", distance);
  cmd.AddValue ("stopTime", "Simulation duration in seconds", stopTime);
  cmd.AddValue ("txArgs", "Extra ffmpeg options for the sender, inserted before the output URL", txArgs);
  cmd.AddValue ("rxInArgs", "Extra ffmpeg input options for the receiver, inserted before -i", rxInArgs);
  cmd.AddValue ("rxArgs", "Extra ffmpeg options for the receiver, inserted before the output file", rxArgs);
  cmd.AddValue ("fiber", "DCE fiber manager: ucontext or pthread", fiber);
  cmd.AddValue ("ping", "Run a real ping from the client to the server before streaming", ping);
  cmd.AddValue ("pcap", "Capture 802.11 frames seen by the AP to dce-wifi-video-*.pcap", pcap);
  cmd.Parse (argc, argv);

  if (realtime)
    {
      GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
      // Catch up silently when the host is slow (e.g. while a player opens the pipe).
      Config::SetDefault ("ns3::RealtimeSimulatorImpl::SynchronizationMode",
                          EnumValue (RealtimeSimulatorImpl::SYNC_BEST_EFFORT));
    }

  // Expose the video to node 0 as /video.ts, and prepare node 2's /stream.ts.
  // A relative --video is looked up in the current directory, then in the
  // directories of DCE_PATH (the test clip is installed next to the DCE
  // binaries, as bin_dce/video.ts).
  char resolved[PATH_MAX];
  resolved[0] = 0;
  if (realpath (video.c_str (), resolved) == 0 && video[0] != '/' && getenv ("DCE_PATH") != 0)
    {
      std::istringstream dcePath (getenv ("DCE_PATH"));
      std::string dir;
      while (std::getline (dcePath, dir, ':'))
        {
          if (dir != "" && realpath ((dir + "/" + video).c_str (), resolved) != 0)
            {
              break;
            }
        }
    }
  if (access (resolved, R_OK) != 0 || std::string (resolved) == "")
    {
      std::cerr << "Cannot find video file '" << video << "' (looked in the current directory and DCE_PATH)" << std::endl;
      return 1;
    }
  mkdir ("files-0", 0755);
  mkdir ("files-2", 0755);
  unlink ("files-0/video.ts");
  if (symlink (resolved, "files-0/video.ts") != 0)
    {
      perror ("symlink files-0/video.ts");
      return 1;
    }
  unlink ("files-2/stream.ts");
  if (fifo && mkfifo ("files-2/stream.ts", 0644) != 0)
    {
      perror ("mkfifo files-2/stream.ts");
      return 1;
    }

  NodeContainer nodes;
  nodes.Create (3);
  Ptr<Node> server = nodes.Get (0);
  Ptr<Node> ap = nodes.Get (1);
  Ptr<Node> client = nodes.Get (2);

  // --- 802.11n infrastructure BSS on channel 36 ---------------------------
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
  phy.Set ("ChannelSettings", StringValue ("{36, 20, BAND_5GHZ, 0}"));

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211n);
  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");

  Ssid ssid = Ssid ("dce-video");
  WifiMacHelper mac;
  mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevices = wifi.Install (phy, mac, NodeContainer (server, client));
  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (phy, mac, ap);

  SetPosition (server, -5.0, 0.0);
  SetPosition (ap, 0.0, 0.0);
  SetPosition (client, distance, 0.0);

  NetDeviceContainer devices;
  devices.Add (staDevices.Get (0)); // server -> 10.1.1.1
  devices.Add (staDevices.Get (1)); // client -> 10.1.1.2
  devices.Add (apDevice.Get (0));   // ap     -> 10.1.1.3

  // --- DCE and the IP stack ------------------------------------------------
  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType",
                                      StringValue (fiber == "pthread" ? "PthreadFiberManager" : "UcontextFiberManager"));
  if (stack == "linux")
    {
#ifdef LINUX_STACK
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      dceManager.Install (nodes);
      LinuxStackHelper linux;
      linux.Install (nodes);
#else
      std::cerr << "DCE was built without --with-lkl; use --stack=ns3" << std::endl;
      return 1;
#endif
    }
  else
    {
      InternetStackHelper internet;
      internet.Install (nodes);
      dceManager.Install (nodes);
    }

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  std::ostringstream serverIp, clientIp;
  interfaces.GetAddress (0).Print (serverIp);
  interfaces.GetAddress (1).Print (clientIp);

#ifdef LINUX_STACK
  if (stack == "linux")
    {
      LinuxStackHelper::RunIp (server, Seconds (1.0), "addr list");
      LinuxStackHelper::RunIp (client, Seconds (1.0), "route show");
    }
#endif

  // --- Applications: real ping and real ffmpeg -----------------------------
  DceApplicationHelper dce;
  dce.SetStackSize (1 << 24); // 16 MiB: ffmpeg probes streams with real decoders
  ApplicationContainer apps;

  // ping server from client once associated, to show the stack is alive.
  if (ping)
    {
      dce.SetBinary ("ping");
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.AddArgument ("-c");
      dce.AddArgument ("3");
      dce.AddArgument (serverIp.str ());
      apps = dce.Install (client);
      apps.Start (Seconds (1.5));
    }

  // Receiver first: ffmpeg listens on udp://10.1.1.2:5004 and writes the
  // stream unchanged to /stream.ts (files-2/stream.ts on the host).
  std::ostringstream listenUrl;
  listenUrl << "udp://" << clientIp.str () << ":" << port << "?timeout=5000000"; // exit after 5 s without packets
  dce.SetBinary ("ffmpeg");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-nostdin");
  dce.AddArgument ("-hide_banner");
  dce.AddArgument ("-loglevel");
  dce.AddArgument ("info");
  dce.AddArgument ("-y");
  // By default ffmpeg probes a live MPEG-TS input until it has read 5 MB,
  // which for a small stream means buffering it entirely before writing
  // anything. Probe 1 s / 200 kB at most so that the output follows the
  // input closely enough to be watched live.
  dce.AddArgument ("-analyzeduration");
  dce.AddArgument ("1000000");
  dce.AddArgument ("-probesize");
  dce.AddArgument ("200000");
  AddArguments (dce, rxInArgs);
  dce.AddArgument ("-i");
  dce.AddArgument (listenUrl.str ());
  dce.AddArgument ("-c");
  dce.AddArgument ("copy");
  dce.AddArgument ("-f");
  dce.AddArgument ("mpegts");
  AddArguments (dce, rxArgs);
  dce.AddArgument ("/stream.ts");
  apps = dce.Install (client);
  apps.Start (Seconds (2.0));

  // Sender: ffmpeg reads /video.ts at its native rate and streams it.
  std::ostringstream sendUrl;
  sendUrl << "udp://" << clientIp.str () << ":" << port << "?pkt_size=1316";
  dce.SetBinary ("ffmpeg");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-nostdin");
  dce.AddArgument ("-hide_banner");
  dce.AddArgument ("-loglevel");
  dce.AddArgument ("info");
  dce.AddArgument ("-re");
  dce.AddArgument ("-i");
  dce.AddArgument ("/video.ts");
  dce.AddArgument ("-c");
  dce.AddArgument ("copy");
  dce.AddArgument ("-f");
  dce.AddArgument ("mpegts");
  AddArguments (dce, txArgs);
  dce.AddArgument (sendUrl.str ());
  apps = dce.Install (server);
  apps.Start (Seconds (3.0));

  // --- Tracing ---------------------------------------------------------------
  Ptr<WifiNetDevice> clientDevice = DynamicCast<WifiNetDevice> (staDevices.Get (1));
  clientDevice->GetMac ()->TraceConnectWithoutContext ("MacRx", MakeCallback (&ClientMacRx));
  if (pcap)
    {
      phy.EnablePcap ("dce-wifi-video", apDevice.Get (0));
    }
  Simulator::Schedule (Seconds (1.0), &Progress, Seconds (1.0));

  std::cout << "Server STA " << serverIp.str () << " streams " << resolved << std::endl
            << "Client STA " << clientIp.str () << " writes files-2/stream.ts"
            << (fifo ? " (named pipe: open it with a player to watch live)" : "") << std::endl
            << "Process output: files-{0,2}/var/log/<pid>/{stdout,stderr}" << std::endl;

  // Optionally start a host player on the pipe (e.g. --player=vlc).
  pid_t playerPid = 0;
  if (player != "")
    {
      std::string command = player;
      if (command.find ("{}") != std::string::npos)
        {
          command.replace (command.find ("{}"), 2, "files-2/stream.ts");
        }
      else
        {
          command += " files-2/stream.ts";
        }
      char *args[] = { (char *)"/bin/sh", (char *)"-c", (char *)command.c_str (), 0 };
      if (posix_spawn (&playerPid, "/bin/sh", 0, 0, args, environ) != 0)
        {
          perror ("posix_spawn");
          playerPid = 0;
        }
    }

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();

  if (playerPid > 0)
    {
      int status;
      waitpid (playerPid, &status, 0);
    }

  if (fifo)
    {
      return 0; // the stream went to a player; nothing left to compare
    }
  return CompareStreams (resolved, "files-2/stream.ts") ? 0 : 1;
}
