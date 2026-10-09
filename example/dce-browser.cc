/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// A real graphical web browser inside ns-3: Dillo runs on a Wi-Fi station,
// fetches pages from a real thttpd running on another station through the
// access point, and draws its window on the host X display. --browser
// selects another DCE build of a browser (same invocation: [options] URL).
//
//   node 0 (server STA)         node 1 (AP)          node 2 (client STA)
//   +-------------------+   +--------------+   +------------------------+
//   | thttpd  (files-0) |   |              |   |dillo 10.1.1.1/browser/ |
//   +-------------------+   |              |   +------------------------+
//   | Linux 4.4 (DCE)   |   | Linux 4.4    |   | Linux 4.4 (DCE)        |
//   +-------------------+   +--------------+   +------------------------+
//   |  802.11n  10.1.1.1|   | 10.1.1.3     |   | 10.1.1.2  802.11n      |
//   +-------------------+   +--------------+   +------------------------+
//        )))))))))))))))))))))     (((((((((((((((((((((((((
//
// Everything runs inside the simulation: Dillo, FLTK, Xft, fontconfig and
// freetype are loaded by DCE against its libc; only the X11 protocol leaves
// the simulation, through the host socket passthrough (DceX11Helper). The
// browser's file system is files-2/: /usr and /etc are symlinked to the
// host's so that fontconfig finds its configuration and the fonts.
//
// Requires a DCE build of dillo in DCE_PATH (utils/build_kernel_deps.sh
// builds it when the FLTK headers are installed), DISPLAY, and the
// real-time simulator, which this example enables.
//
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/tap-bridge-module.h"

#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <filesystem>
#include <map>
#include <limits.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceBrowser");

static void
BrowserFinished (uint16_t pid, int status)
{
  std::cout << "the browser exited with status " << status << " at t="
            << Simulator::Now ().GetSeconds () << "s, stopping" << std::endl;
  Simulator::Stop ();
}

static void
SetPosition (Ptr<Node> node, double x, double y)
{
  Ptr<ConstantPositionMobilityModel> m = CreateObject<ConstantPositionMobilityModel> ();
  m->SetPosition (Vector (x, y, 0));
  node->AggregateObject (m);
}

// The web site served by the simulated server.
static bool
FindInDcePath (const std::string &name, std::string &found)
{
  char resolved[PATH_MAX];
  if (realpath (name.c_str (), resolved) != 0)
    {
      found = resolved;
      return true;
    }
  if (getenv ("DCE_PATH") == 0)
    {
      return false;
    }
  std::istringstream dcePath (getenv ("DCE_PATH"));
  std::string dir;
  while (std::getline (dcePath, dir, ':'))
    {
      if (dir != "" && realpath ((dir + "/" + name).c_str (), resolved) != 0)
        {
          found = resolved;
          return true;
        }
    }
  return false;
}

static void
CreateSite (void)
{
  mkdir ("files-0", 0755);
  // A directory of its own: test.py runs the examples in parallel in one
  // directory, and other examples write files-0/index.html too.
  mkdir ("files-0/browser", 0755);
  // The clips are copied, not linked: thttpd refuses symbolic links that
  // point outside its document root.
  std::string video;
  std::error_code ec;
  if (FindInDcePath ("video.ts", video))
    {
      std::filesystem::copy_file (video, "files-0/browser/video.ts", std::filesystem::copy_options::overwrite_existing, ec);
    }
  // the same clip as MPEG-1 program stream, the format Northstar's player decodes
  if (FindInDcePath ("video.mpg", video))
    {
      std::filesystem::copy_file (video, "files-0/browser/video.mpg", std::filesystem::copy_options::overwrite_existing, ec);
    }
  std::ofstream player ("files-0/browser/video.html");
  player << "<html><head><title>Video over simulated Wi-Fi</title>\n"
            "<style>body{font-family:sans-serif;margin:2em;background:#f4f6fa}h1{color:#1f4e79}"
            "video{border:2px solid #1f4e79;background:#000}</style></head>\n"
            "<body><h1>Video over simulated Wi-Fi</h1>\n"
            "<p>The clip below is fetched from thttpd on the server station through the simulated\n"
            "802.11n network and decoded by the browser's own MPEG-1 player, inside the simulation.</p>\n"
            "<video id=\"v\" controls autoplay width=\"720\" height=\"405\" src=\"video.mpg\" type=\"video/mpeg\">\n"
            "This browser cannot play MPEG-1 video.</video>\n"
            // Northstar's <video> element is picture only; its <audio> element
            // decodes the MP2 track of the same program stream through the SDL2
            // mixer, played on the host's PulseAudio server through DCE.
            // Muted autoplay is allowed without a click; the script unmutes the
            // soundtrack and lines it up with the picture once the video plays.
            "<audio id=\"a\" autoplay muted src=\"video.mpg\" type=\"video/mpeg\"></audio>\n"
            "<p><button onclick=\"var v=document.getElementById('v'),a=document.getElementById('a');"
            "a.pause();v.pause();v.currentTime=0;a.currentTime=0;a.muted=false;v.play();a.play();\">"
            "&#9654; Play again from the start</button> <span id=\"s\"></span>\n"
            "<script>\n"
            "var v=document.getElementById('v'),a=document.getElementById('a'),started=false;\n"
            "function sound(){if(started)return;started=true;a.currentTime=v.currentTime;"
            "a.muted=false;a.volume=1.0;a.play();}\n"
            "v.addEventListener('playing',sound);\n"
            // the sound is the master clock: the mixer starts a little late
            // and keeps about 0.1 s queued in the output device
            "setInterval(function(){if(!started||a.paused||v.paused)return;"
            "var t=a.currentTime-0.1;if(Math.abs(v.currentTime-t)>0.12)v.currentTime=t;},500);\n"
            "window.addEventListener('load',function(){if(!v.paused)sound();});\n"
            "setInterval(function(){document.getElementById('s').textContent="
            "'video '+v.currentTime.toFixed(1)+' s, sound '+a.currentTime.toFixed(1)+' s'"
            "+(a.muted?' (muted)':'');},1000);\n"
            "</script>\n"
            "<p><a href=\"video.mpg\">video.mpg</a> (MPEG-1 video + MP2 audio, 2.3 MB) &middot;\n"
            "<a href=\"video.ts\">video.ts</a> (H.264/AAC MPEG-TS, 1.2 MB, streamed by dce-wifi-video) &middot;\n"
            "<a href=\"index.html\">Back</a></p></body></html>\n";
  player.close ();
  std::ofstream index ("files-0/browser/index.html");
  index << "<html><head><title>Served from inside ns-3</title>\n"
           "<style>body{font-family:sans-serif;margin:2em;background:#f4f6fa}"
           "h1{color:#1f4e79}table{border-collapse:collapse}td,th{border:1px solid #888;padding:4px 10px}"
           ".box{background:#fff;border:2px solid #1f4e79;padding:1em;margin:1em 0}</style></head>\n"
           "<body><h1>Hello from thttpd on node 0</h1>\n"
           "<div class=box>This page was fetched by <b>Dillo</b> running on the client station,\n"
           "over a simulated 802.11n network, from <b>thttpd</b> running on the server station.\n"
           "Both are real, unmodified programs executed by DCE inside ns-3, on the real Linux\n"
           "kernel stack.</div>\n"
           "<table><tr><th>Node</th><th>Role</th><th>Address</th></tr>\n"
           "<tr><td>0</td><td>server STA (thttpd)</td><td>10.1.1.1</td></tr>\n"
           "<tr><td>1</td><td>access point</td><td>10.1.1.3</td></tr>\n"
           "<tr><td>2</td><td>client STA (dillo)</td><td>10.1.1.2</td></tr></table>\n"
           "<p><a href=\"big.html\">A 1 MB page</a> to watch the Wi-Fi link work, and\n"
           "<a href=\"about.html\">how this works</a>.</p>\n"
           "<p><a href=\"http://10.1.1.1:8080/\">Live number series</a>: a page whose JavaScript keeps\n"
           "asking a second server on this station for the next number.</p>\n"
           "<p><a href=\"video.html\">Watch the video sample</a> in the browser's own player\n"
           "(MPEG-1), or download <a href=\"video.ts\">the MPEG-TS</a> (1.2 MB, H.264/AAC) that\n"
           "dce-wifi-video streams.</p>\n"
           "<div class=box id=js>JavaScript is <b>off</b> in this browser (Dillo has none).</div>\n"
           "<script>document.getElementById('js').innerHTML = 'JavaScript is <b>running</b> in this browser: '"
           " + navigator.userAgent + ' says 6 * 7 = ' + (6 * 7) + '.';</script>\n"
           "</body></html>\n";
  std::ofstream about ("files-0/browser/about.html");
  about << "<html><head><title>How this works</title></head><body>\n"
           "<h1>How this works</h1><ul>\n"
           "<li>DCE loads dillo, FLTK, Xft, fontconfig and freetype into the simulator and\n"
           "runs them against its own libc, with a cooperative scheduler driven by simulated time.</li>\n"
           "<li>Sockets are the ones of the simulated node: the HTTP connection crosses the\n"
           "ns-3 Wi-Fi model and the Linux 4.4 TCP/IP stack of each station.</li>\n"
           "<li>The connection to the X server is the only one handed to the host\n"
           "(DceHostUnixSocketPaths), so the window you see is drawn by the simulated browser.</li>\n"
           "</ul><p><a href=\"index.html\">Back</a></p></body></html>\n";
  std::ofstream big ("files-0/browser/big.html");
  big << "<html><head><title>1 MB page</title></head><body><h1>Lorem ipsum, 1 MB of it</h1>\n";
  for (int i = 0; i < 6000; i++)
    {
      big << "<p>Paragraph " << i << ": lorem ipsum dolor sit amet, consectetur adipiscing elit, "
             "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
             "veniam, quis nostrud exercitation ullamco laboris.</p>\n";
    }
  big << "<p><a href=\"index.html\">Back</a></p></body></html>\n";
}

int
main (int argc, char *argv[])
{
  std::string url = "http://10.1.1.1/browser/";
  std::string browser = "dillo";
  std::string browserArgs = "";
  std::string browserEnv = "";
  double distance = 10.0;
  double stopTime = 600.0;
  uint32_t minRequests = 1;
  std::string tap = "";
  std::string tapHost = "10.99.0.1";
  std::string tapRouter = "10.99.0.2";
  bool youtubeConsent = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("url", "Page the browser opens", url);
  bool numbers = false;
  cmd.AddValue ("numbers", "Open the number series page of numbers-server (http://10.1.1.1:8080/)", numbers);
  cmd.AddValue ("browser", "Browser binary (a DCE build; default dillo)", browser);
  cmd.AddValue ("browserArgs", "Extra command line options for the browser", browserArgs);
  cmd.AddValue ("browserEnv", "Extra environment for the browser, comma separated KEY=VALUE pairs", browserEnv);
  cmd.AddValue ("distance", "Distance in meters between the AP and the client STA", distance);
  cmd.AddValue ("stopTime", "Give up after this many seconds if the browser is still running", stopTime);
  cmd.AddValue ("minRequests", "Fail unless at least this many HTTP requests are seen in the capture", minRequests);
  cmd.AddValue ("tap", "Connect the AP to the real network through this host tap device "
                "(made by utils/dce-tap-setup.sh); the AP routes the stations to the host", tap);
  cmd.AddValue ("tapHost", "Address of the host on the tap device", tapHost);
  cmd.AddValue ("tapRouter", "Address of the AP on the tap network (a /24 with tapHost)", tapRouter);
  cmd.AddValue ("youtubeConsent", "With --tap and northstar: start with YouTube's cookie consent answered "
                "(\"reject all\") instead of showing its dialog", youtubeConsent);
  cmd.Parse (argc, argv);
  if (numbers)
    {
      url = "http://10.1.1.1:8080/";
    }

  if (getenv ("DISPLAY") == 0)
    {
      std::cerr << "DISPLAY is not set: the browser needs an X display" << std::endl;
      return 1;
    }
#ifndef LINUX_STACK
  if (tap != "")
    {
      std::cerr << "--tap needs the Linux stack (configure DCE --with-lkl): the AP routes" << std::endl;
      return 1;
    }
#endif
  // The X server and the user live in wall-clock time.
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  // Frames from the real network carry real checksums.
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  if (tap != "")
    {
      // The simulated clock starts on January 1st, 2010 by default, before
      // any certificate a real server presents is valid: start it now.
      GlobalValue::Bind ("SimulationTimeBase", UintegerValue ((uint32_t) time (0)));
    }
  Config::SetDefault ("ns3::RealtimeSimulatorImpl::SynchronizationMode",
                      EnumValue (RealtimeSimulatorImpl::SYNC_BEST_EFFORT));
  DceX11Helper::Enable ();

  CreateSite ();
  // The browser's file system: fonts and their configuration come from the host.
  mkdir ("files-2", 0755);
  unlink ("files-2/usr");
  unlink ("files-2/etc");
  symlink ("/usr", "files-2/usr");
  symlink ("/etc", "files-2/etc");
  mkdir ("files-2/.cache", 0755); // fontconfig's cache (HOME=/)
  if (tap != "" && browser == "northstar" && youtubeConsent)
    {
      // Northstar keeps one curl cookie file per site, named after the
      // first 32 hex digits of the SHA-256 of the site key, here
      // "https:://youtube.com" (the protocol keeps its colon). SOCS=CAI is
      // the consent cookie YouTube sets for "Reject all".
      std::filesystem::create_directories ("files-2/.config/northstar/cookies");
      std::ofstream jar ("files-2/.config/northstar/cookies/a07643249bc7771912f15c759aa3ccb8.txt");
      jar << "# Netscape HTTP Cookie File\n"
          << ".youtube.com\tTRUE\t/\tTRUE\t" << time (0) + 365 * 24 * 3600 << "\tSOCS\tCAI\n";
    }
  mkdir ("files-2/.dillo", 0755);
  std::ofstream rc ("files-2/.dillo/dillorc");
  rc << "# written by dce-dillo\nshow_tooltip=NO\n";
  rc.close ();

  NodeContainer nodes;
  nodes.Create (3);
  Ptr<Node> server = nodes.Get (0);
  Ptr<Node> ap = nodes.Get (1);
  Ptr<Node> client = nodes.Get (2);
  DceX11Helper::InstallAuthority (client);

  // --- 802.11n infrastructure BSS ---------------------------------------------
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
  phy.Set ("ChannelSettings", StringValue ("{36, 20, BAND_5GHZ, 0}"));
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211n);
  wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");
  Ssid ssid = Ssid ("dce-browser");
  WifiMacHelper mac;
  mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevices = wifi.Install (phy, mac, NodeContainer (server, client));
  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (phy, mac, ap);
  SetPosition (server, -5.0, 0.0);
  SetPosition (ap, 0.0, 0.0);
  SetPosition (client, distance, 0.0);

  NetDeviceContainer devices;
  devices.Add (staDevices.Get (0));
  devices.Add (staDevices.Get (1));
  devices.Add (apDevice.Get (0));

  // --- the real network, through a tap device on the host ---------------------
  // A wired segment between the AP and a "ghost" node whose device is bridged
  // to the host's tap: the host is the AP's neighbour on 10.99.0.0/24 and
  // routes (and NATs) the simulated networks to the Internet.
  NetDeviceContainer wired;
  if (tap != "")
    {
      Ptr<Node> ghost = CreateObject<Node> ();
      CsmaHelper csma;
      csma.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
      csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (50)));
      wired = csma.Install (NodeContainer (ghost, ap));
      TapBridgeHelper bridge;
      bridge.SetAttribute ("Mode", StringValue ("UseLocal"));
      bridge.SetAttribute ("DeviceName", StringValue (tap));
      bridge.Install (ghost, wired.Get (0));
    }

  // --- DCE with the Linux kernel stack ----------------------------------------
  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType", StringValue ("UcontextFiberManager"));
#ifdef LINUX_STACK
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
  dceManager.Install (nodes);
  LinuxStackHelper linux;
  linux.Install (nodes);
#else
  InternetStackHelper internet;
  internet.Install (nodes);
  dceManager.Install (nodes);
#endif
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);
#ifdef LINUX_STACK
  if (tap != "")
    {
      std::string net = tapRouter.substr (0, tapRouter.rfind ('.')) + ".0";
      Ipv4AddressHelper tapAddress;
      tapAddress.SetBase (net.c_str (), "255.255.255.0", ("0.0.0." + tapRouter.substr (tapRouter.rfind ('.') + 1)).c_str ());
      tapAddress.Assign (NetDeviceContainer (wired.Get (1)));
      linux.SysctlSet (NodeContainer (ap), ".net.ipv4.ip_forward", "1");
      LinuxStackHelper::RunIp (ap, Seconds (0.5), "route add default via " + tapHost);
      std::ostringstream via;
      via << "route add default via " << interfaces.GetAddress (2);
      LinuxStackHelper::RunIp (server, Seconds (0.5), via.str ());
      LinuxStackHelper::RunIp (client, Seconds (0.5), via.str ());
    }
#endif

  // --- Applications ------------------------------------------------------------
  DceApplicationHelper dce;
  ApplicationContainer apps;

  // the web server, serving files-0/ on port 80 (the pages are in browser/)
  dce.SetStackSize (1 << 20);
  dce.SetBinary ("thttpd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetUid (1);
  dce.SetEuid (1);
  apps = dce.Install (server);
  apps.Start (Seconds (1.0));

  // the JSON number server polled by the JavaScript of http://10.1.1.1:8080/
  dce.SetBinary ("numbers-server");
  dce.ResetArguments ();
  dce.AddArgument ("8080");
  dce.ResetEnvironment ();
  apps = dce.Install (server);
  apps.Start (Seconds (1.0));

  // the browser
  dce.SetStackSize (1 << 24);
  dce.SetBinary (browser);
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  DceX11Helper::SetEnvironment (dce);
  if (browser == "northstar")
    {
      // GTK4 inside the simulation: software rendering, X11, no dconf, no
      // accessibility bus; no Landlock/seccomp sandbox (they would apply to
      // the simulator), no supervisor process.
      dce.AddEnvironment ("GSK_RENDERER", "cairo");
      dce.AddEnvironment ("GDK_BACKEND", "x11");
      dce.AddEnvironment ("GSETTINGS_BACKEND", "memory");
      dce.AddEnvironment ("NO_AT_BRIDGE", "1");
      dce.AddEnvironment ("GTK_A11Y", "none");
      dce.AddEnvironment ("LANG", "C.UTF-8");
      dce.AddEnvironment ("LC_ALL", "C.UTF-8");
      dce.AddEnvironment ("NS_NO_SANDBOX", "1");
      dce.AddArgument ("--no-watchdog");
      // GApplication registers on the session bus; GIO would otherwise try
      // to spawn dbus-launch, impossible from the simulation.
      if (!DceX11Helper::UseSessionBus (dce))
        {
          std::cerr << "northstar needs the host's D-Bus session bus (DBUS_SESSION_BUS_ADDRESS)" << std::endl;
          return 1;
        }
      // sound of the <video> player: SDL2 on the host's PulseAudio server
      if (DceX11Helper::UsePulseAudio (client, dce))
        {
          dce.AddEnvironment ("SDL_AUDIODRIVER", "pulseaudio");
        }
      else
        {
          std::cout << "No PulseAudio socket on the host: the browser will be silent" << std::endl;
          dce.AddEnvironment ("SDL_AUDIODRIVER", "dummy");
        }
    }
  {
    std::istringstream is (browserEnv);
    std::string pair;
    while (std::getline (is, pair, ','))
      {
        size_t eq = pair.find ('=');
        if (eq != std::string::npos && eq > 0)
          {
            dce.AddEnvironment (pair.substr (0, eq), pair.substr (eq + 1));
          }
      }
  }
  {
    std::istringstream is (browserArgs);
    std::string arg;
    while (is >> arg)
      {
        dce.AddArgument (arg);
      }
  }
  dce.AddArgument (url);
  // browsers refuse to run as root
  dce.SetUid (1000);
  dce.SetEuid (1000);
  dce.SetFinishedCallback (MakeCallback (&BrowserFinished));
  apps = dce.Install (client);
  apps.Start (Seconds (2.0));

  phy.EnablePcap ("dce-browser", apDevice.Get (0));

  std::cout << "thttpd on " << interfaces.GetAddress (0) << " serves files-0/browser/, "
            << browser << " on " << interfaces.GetAddress (1) << " opens " << url
            << " and draws on " << getenv ("DISPLAY") << std::endl
            << "Close the browser window to end the simulation (or wait " << stopTime << " s)." << std::endl;
  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();

  // The capture at the AP must show the browser's HTTP requests.
  DcePcapCheck capture ("dce-browser-1-0.pcap");
  if (tap != "")
    {
      uint64_t https = capture.PayloadBytes (IPPROTO_TCP, 443);
      uint64_t http = capture.PayloadBytes (IPPROTO_TCP, 80);
      std::cout << "Capture dce-browser-1-0.pcap: " << https << " bytes of HTTPS and "
                << http << " bytes of HTTP payload through the AP" << std::endl;
      if (!capture.Ok () || https + http == 0)
        {
          std::cout << "FAIL: no web traffic between the browser and the real network" << std::endl;
          return 1;
        }
      std::cout << "PASS: the browser reached the real network through the simulated Wi-Fi" << std::endl;
      return 0;
    }
  uint32_t requests = 0;
  uint16_t ports[] = { 80, 8080 };
  for (unsigned i = 0; i < 2; i++)
    {
      std::map<std::string, uint32_t> lines = capture.RequestLines (ports[i], "GET ");
      for (std::map<std::string, uint32_t>::iterator l = lines.begin (); l != lines.end (); ++l)
        {
          std::cout << "  port " << ports[i] << ": " << l->second << " x " << l->first << std::endl;
          requests += l->second;
        }
    }
  std::cout << "Capture dce-browser-1-0.pcap: " << requests << " HTTP requests from the browser, "
            << capture.PayloadBytes (IPPROTO_TCP, 80) + capture.PayloadBytes (IPPROTO_TCP, 8080)
            << " TCP payload bytes" << std::endl;
  if (!capture.Ok () || requests < minRequests)
    {
      std::cout << "FAIL: expected at least " << minRequests << " HTTP requests in the capture" << std::endl;
      return 1;
    }
  std::cout << "PASS: the browser fetched from the simulated server over the Wi-Fi link" << std::endl;
  return 0;
}
