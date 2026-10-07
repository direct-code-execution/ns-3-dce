/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// A real graphical web browser inside ns-3: Dillo runs on a Wi-Fi station,
// fetches pages from a real thttpd running on another station through the
// access point, and draws its window on the host X display.
//
//   node 0 (server STA)         node 1 (AP)          node 2 (client STA)
//   +-------------------+   +--------------+   +------------------------+
//   | thttpd  (files-0) |   |              |   | dillo http://10.1.1.1/ |
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

#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceDillo");

static void
BrowserFinished (uint16_t pid, int status)
{
  std::cout << "dillo exited with status " << status << " at t="
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
static void
CreateSite (void)
{
  mkdir ("files-0", 0755);
  std::ofstream index ("files-0/index.html");
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
           "</body></html>\n";
  std::ofstream about ("files-0/about.html");
  about << "<html><head><title>How this works</title></head><body>\n"
           "<h1>How this works</h1><ul>\n"
           "<li>DCE loads dillo, FLTK, Xft, fontconfig and freetype into the simulator and\n"
           "runs them against its own libc, with a cooperative scheduler driven by simulated time.</li>\n"
           "<li>Sockets are the ones of the simulated node: the HTTP connection crosses the\n"
           "ns-3 Wi-Fi model and the Linux 4.4 TCP/IP stack of each station.</li>\n"
           "<li>The connection to the X server is the only one handed to the host\n"
           "(DceHostUnixSocketPaths), so the window you see is drawn by the simulated browser.</li>\n"
           "</ul><p><a href=\"index.html\">Back</a></p></body></html>\n";
  std::ofstream big ("files-0/big.html");
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
  std::string url = "http://10.1.1.1/";
  std::string browser = "dillo";
  double distance = 10.0;
  double stopTime = 600.0;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("url", "Page the browser opens", url);
  cmd.AddValue ("browser", "Browser binary (a DCE build of dillo)", browser);
  cmd.AddValue ("distance", "Distance in meters between the AP and the client STA", distance);
  cmd.AddValue ("stopTime", "Give up after this many seconds if the browser is still running", stopTime);
  cmd.Parse (argc, argv);

  if (getenv ("DISPLAY") == 0)
    {
      std::cerr << "DISPLAY is not set: the browser needs an X display" << std::endl;
      return 1;
    }
  // The X server and the user live in wall-clock time.
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
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
  Ssid ssid = Ssid ("dce-dillo");
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

  // --- Applications ------------------------------------------------------------
  DceApplicationHelper dce;
  ApplicationContainer apps;

  // the web server, serving files-0/ on port 80
  dce.SetStackSize (1 << 20);
  dce.SetBinary ("thttpd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetUid (1);
  dce.SetEuid (1);
  apps = dce.Install (server);
  apps.Start (Seconds (1.0));

  // the browser
  dce.SetStackSize (1 << 24);
  dce.SetBinary (browser);
  dce.ResetArguments ();
  dce.AddArgument (url);
  dce.ResetEnvironment ();
  DceX11Helper::SetEnvironment (dce);
  dce.SetUid (0);
  dce.SetEuid (0);
  dce.SetFinishedCallback (MakeCallback (&BrowserFinished));
  apps = dce.Install (client);
  apps.Start (Seconds (2.0));

  phy.EnablePcap ("dce-dillo", apDevice.Get (0));

  std::cout << "thttpd on " << interfaces.GetAddress (0) << " serves files-0/, "
            << browser << " on " << interfaces.GetAddress (1) << " opens " << url
            << " and draws on " << getenv ("DISPLAY") << std::endl
            << "Close the browser window to end the simulation (or wait " << stopTime << " s)." << std::endl;
  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
