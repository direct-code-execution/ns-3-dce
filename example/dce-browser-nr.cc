/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// An unmodified Chromium inside ns-3, on a 5G NR UE that is handed over
// between two gNBs, watching a real web page through a tap device.
//
//   Chromium (UE, Linux stack) -- NR -- gNB A/B -- core (ns-3 stack)
//     -- CSMA -- TapBridge -- tap-dce -- host NAT -- real network
//
// X11 and PulseAudio leave the simulation through DceX11Helper, so they do
// not cross NR: a handover interrupts the data, not the window.
//
// tap-dce comes from utils/dce-tap-setup.sh, which routes 10.1.1.0/24 to
// 10.99.0.2: the UE takes its address from that network.
//
// --hoTimes lists real X2 handovers. --gap adds an injected outage of the
// data path to each one, for an interruption longer than the handover takes;
// it is not a result of the NR model.
//
// dce-browser-nr-timeline.csv has, every 100 ms, the downlink bytes entering
// the core from the real network and how late the simulator is against the
// wall clock: a stall with a large lag comes from the CPU, not from NR.
//
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/antenna-module.h"
#include "ns3/nr-module.h"
#include "ns3/nr-epc-pgw-application.h"

#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceBrowserNr");

// Chrome's own thread local variables are at fixed offsets below the thread pointer.
DCE_RESERVE_EXECUTABLE_TLS (64 * 1024);

static std::ofstream g_events;
static std::ofstream g_timeline;
static uint64_t g_dlBytes = 0;
static uint64_t g_handovers = 0;
static std::ofstream g_rsrp;
static Time g_lastRx;
static Time g_lastRxAtHandover;
static bool g_awaitFirstRx = false;
static double g_gap = 0;
static std::vector<Ptr<RateErrorModel>> g_gapModels;
static auto g_wallStart = std::chrono::steady_clock::now ();

static double
Lag (void)
{
  double wall = std::chrono::duration<double> (std::chrono::steady_clock::now () - g_wallStart).count ();
  return wall - Simulator::Now ().GetSeconds ();
}

static void
Event (const std::string &what)
{
  std::cout << "t=" << Simulator::Now ().GetSeconds () << "s lag=" << Lag () << "s " << what << std::endl;
  g_events << Simulator::Now ().GetSeconds () << "," << Lag () << "," << what << std::endl;
}

static void
SetGap (std::vector<Ptr<RateErrorModel>> models, bool on)
{
  for (auto &m : models)
    {
      if (on)
        {
          m->Enable ();
        }
      else
        {
          m->Disable ();
        }
    }
  Event (on ? "injected gap start" : "injected gap end");
}

static void
HandoverStart (std::string, uint64_t, uint16_t cell, uint16_t, uint16_t target)
{
  std::ostringstream os;
  os << "handover start cell " << cell << " to " << target;
  Event (os.str ());
  g_lastRxAtHandover = g_lastRx;
  g_awaitFirstRx = true;
  if (g_gap > 0)
    {
      Simulator::ScheduleNow (&SetGap, g_gapModels, true);
      Simulator::Schedule (Seconds (g_gap), &SetGap, g_gapModels, false);
    }
}

static void
HandoverEnd (std::string, uint64_t, uint16_t cell, uint16_t)
{
  g_handovers++;
  std::ostringstream os;
  os << "handover end cell " << cell;
  Event (os.str ());
}

// Every packet the UE receives: the data rate, and the interruption at a
// handover, from the last packet before it to the first one after it.
static void
UeRx (Ptr<NetDevice>, Ptr<const Packet> packet, uint16_t, const Address &, const Address &, NetDevice::PacketType)
{
  g_dlBytes += packet->GetSize ();
  g_lastRx = Simulator::Now ();
  if (g_awaitFirstRx && g_handovers > 0)
    {
      g_awaitFirstRx = false;
      std::ostringstream os;
      os << "interruption " << (g_lastRx - g_lastRxAtHandover).GetMilliSeconds () << " ms";
      Event (os.str ());
    }
}

static void
Rsrp (uint16_t, uint16_t cell, double rsrp, double, bool serving, uint8_t)
{
  g_rsrp << Simulator::Now ().GetSeconds () << "," << cell << "," << rsrp << "," << serving << "\n";
  g_rsrp.flush ();
}

static void
Sample (void)
{
  g_timeline << Simulator::Now ().GetSeconds () << "," << g_dlBytes << "," << Lag () << "\n";
  g_timeline.flush ();
  g_dlBytes = 0;
  Simulator::Schedule (MilliSeconds (100), &Sample);
}

static void
BrowserFinished (uint16_t, int status)
{
  std::cout << "the browser exited with status " << status << std::endl;
  Simulator::Stop ();
}

static bool
FindInDcePath (const std::string &name, std::string &found)
{
  char resolved[PATH_MAX];
  std::istringstream dcePath (getenv ("DCE_PATH") ? getenv ("DCE_PATH") : "");
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

int
main (int argc, char *argv[])
{
  std::string url = "https://www.youtube.com/watch?v=aqz-KE-bpKQ";
  std::string tap = "tap-dce";
  std::string tapHost = "10.99.0.1";
  std::string tapRouter = "10.99.0.2";
  std::string hoTimes = "";
  std::string browserArgs = "";
  double gap = 0;
  double stopTime = 300;
  double centralFrequency = 3.5e9;
  double bandwidth = 40e6;
  uint32_t numerology = 1;
  double isd = 400;
  double uePos = 0.1;
  double speed = 1.6;
  double hysteresis = 3;
  double ttt = 0.256;
  CommandLine cmd;
  cmd.AddValue ("url", "Page the browser opens", url);
  cmd.AddValue ("tap", "Host tap device", tap);
  cmd.AddValue ("hoTimes", "Comma separated times in seconds of forced X2 handovers (none: A3 decides them)", hoTimes);
  cmd.AddValue ("gap", "Injected outage of the data path at each handover, in seconds (0: none)", gap);
  cmd.AddValue ("stopTime", "End of the simulation in seconds", stopTime);
  cmd.AddValue ("bandwidth", "Carrier bandwidth in Hz", bandwidth);
  cmd.AddValue ("numerology", "NR numerology", numerology);
  cmd.AddValue ("isd", "Distance between the gNBs in meters; the UE is halfway", isd);
  cmd.AddValue ("uePos", "Initial position of the UE between the gNBs, as a fraction of isd", uePos);
  cmd.AddValue ("speed", "Speed of the UE towards the second gNB, in m/s", speed);
  cmd.AddValue ("hysteresis", "A3 handover hysteresis in dB", hysteresis);
  cmd.AddValue ("ttt", "A3 time to trigger in seconds", ttt);
  cmd.AddValue ("browserArgs", "Extra command line options for the browser", browserArgs);
  cmd.Parse (argc, argv);

  // The X server, the user and the real network live in wall-clock time.
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  Config::SetDefault ("ns3::RealtimeSimulatorImpl::SynchronizationMode",
                      EnumValue (RealtimeSimulatorImpl::SYNC_BEST_EFFORT));
  // Frames from the real network carry real checksums.
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  // Certificates of real servers must be valid at the simulated date.
  GlobalValue::Bind ("SimulationTimeBase", UintegerValue ((uint32_t) time (0)));
  DceX11Helper::Enable ();

  // Created first: its file system is files-0.
  Ptr<Node> ue = CreateObject<Node> ();
  NodeContainer ueNodes (ue);
  DceX11Helper::InstallAuthority (ue);

  // The browser's file system: fonts and their configuration come from the host.
  mkdir ("files-0", 0755);
  unlink ("files-0/usr");
  unlink ("files-0/etc");
  symlink ("/usr", "files-0/usr");
  symlink ("/etc", "files-0/etc");
  mkdir ("files-0/.cache", 0755);
  mkdir ("files-0/tmp", 01777);
  std::string chrome;
  if (!FindInDcePath ("chrome", chrome))
    {
      std::cerr << "chrome is not in DCE_PATH" << std::endl;
      return 1;
    }
  // TLS keys for Wireshark, straight in the run directory.
  std::ofstream ("tls-keys.log", std::ios::trunc).close ();
  unlink ("files-0/tls-keys.log");
  symlink ((std::filesystem::current_path () / "tls-keys.log").c_str (), "files-0/tls-keys.log");
  mkdir ("files-0/opt", 0755);
  unlink ("files-0/opt/chrome");
  symlink (std::filesystem::path (chrome).parent_path ().c_str (), "files-0/opt/chrome");

  // --- NR -------------------------------------------------------------------
  Config::SetDefault ("ns3::NrNoBackhaulEpcHelper::UeIpv4Network", Ipv4AddressValue ("10.1.1.0"));
  Config::SetDefault ("ns3::NrNoBackhaulEpcHelper::UeIpv4Mask", Ipv4MaskValue ("255.255.255.0"));
  Config::SetDefault ("ns3::LogDistancePropagationLossModel::Exponent", DoubleValue (3.0));
  Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (35));
  Config::SetDefault ("ns3::NrGnbPhy::TxPower", DoubleValue (43));
  Config::SetDefault ("ns3::NrUePhy::TxPower", DoubleValue (23));
  Config::SetDefault ("ns3::NrUePhy::EnableUplinkPowerControl", BooleanValue (false));
  Config::SetDefault ("ns3::NrUeRrc::N310", UintegerValue (20));
  Config::SetDefault ("ns3::NrUeRrc::N311", UintegerValue (10));
  Config::SetDefault ("ns3::NrUeRrc::T310", TimeValue (Seconds (2)));
  Config::SetDefault ("ns3::NrGnbRrc::HandoverTriggeringDelay", TimeValue (Seconds (0)));
  // A web session's flows must not be cut by a small default buffer.
  Config::SetDefault ("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue (10 * 1024 * 1024));

  Config::SetDefault ("ns3::NrHelper::UseIdealRrc", BooleanValue (false));
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();
  Ptr<NrPointToPointEpcHelper> epc = CreateObject<NrPointToPointEpcHelper> ();
  nrHelper->SetEpcHelper (epc);
  if (hoTimes != "")
    {
      nrHelper->SetHandoverAlgorithmType ("ns3::NrNoOpHandoverAlgorithm");
    }
  else
    {
      nrHelper->SetHandoverAlgorithmType ("ns3::NrA3RsrpHandoverAlgorithm");
      nrHelper->SetHandoverAlgorithmAttribute ("Hysteresis", DoubleValue (hysteresis));
      nrHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger", TimeValue (Seconds (ttt)));
    }
  nrHelper->SetUeAntennaTypeId (IsotropicAntennaModel::GetTypeId ().GetName ());
  nrHelper->SetGnbAntennaTypeId (IsotropicAntennaModel::GetTypeId ().GetName ());

  Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper> ();
  channelHelper->ConfigurePropagationFactory (LogDistancePropagationLossModel::GetTypeId ());
  CcBwpCreator ccBwpCreator;
  CcBwpCreator::SimpleOperationBandConf bandConf (centralFrequency, bandwidth, static_cast<uint8_t> (1));
  OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);
  channelHelper->AssignChannelsToBands ({band});
  BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps ({band});

  NodeContainer gnbNodes;
  gnbNodes.Create (2);
  Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator> ();
  positions->Add (Vector (0, 0, 25));
  positions->Add (Vector (isd, 0, 25));
  positions->Add (Vector (isd * uePos, 0, 1.5));
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positions);
  mobility.Install (gnbNodes);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (ue);
  ue->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  NetDeviceContainer gnbDevs = nrHelper->InstallGnbDevice (gnbNodes, allBwps);
  NetDeviceContainer ueDevs = nrHelper->InstallUeDevice (ue, allBwps);
  for (uint32_t i = 0; i < gnbDevs.GetN (); i++)
    {
      NrHelper::GetGnbPhy (gnbDevs.Get (i), 0)->SetAttribute ("Numerology", UintegerValue (numerology));
    }

  // --- The real network -------------------------------------------------------
  Ptr<Node> pgw = epc->GetPgwNode ();
  Ptr<Node> ghost = CreateObject<Node> ();
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (50)));
  NetDeviceContainer wired = csma.Install (NodeContainer (ghost, pgw));
  TapBridgeHelper bridge;
  bridge.SetAttribute ("Mode", StringValue ("UseLocal"));
  bridge.SetAttribute ("DeviceName", StringValue (tap));
  bridge.Install (ghost, wired.Get (0));

  Ipv4AddressHelper tapAddress;
  tapAddress.SetBase ("10.99.0.0", "255.255.255.0", "0.0.0.2");
  tapAddress.Assign (NetDeviceContainer (wired.Get (1)));
  Ptr<Ipv4> pgwIp = pgw->GetObject<Ipv4> ();
  Ipv4StaticRoutingHelper routing;
  routing.GetStaticRouting (pgwIp)->SetDefaultRoute (Ipv4Address (tapHost.c_str ()),
                                                     pgwIp->GetInterfaceForDevice (wired.Get (1)));
  // What Wireshark shows: the UE's traffic, as IP between the core and the real network.
  csma.EnablePcap ("ue-traffic.pcap", wired.Get (1), true, true);


  // --- DCE with the Linux kernel stack on the UE ------------------------------
  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType", StringValue ("UcontextFiberManager"));
  dceManager.SetTaskManagerAttribute ("PerFiberTls", BooleanValue (true));
  dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
  dceManager.Install (ueNodes);
  LinuxStackHelper linux;
  linux.Install (ueNodes);

  Ipv4InterfaceContainer ueIf = epc->AssignUeIpv4Address (NetDeviceContainer (ueDevs));
  std::ostringstream route;
  route << "route add default via " << epc->GetUeDefaultGatewayAddress () << " dev sim0";
  // NR devices default to 30000: real paths through the tap carry 1500.
  LinuxStackHelper::RunIp (ue, Seconds (0.1), "link set sim0 mtu 1400");
  LinuxStackHelper::RunIp (ue, Seconds (0.1), route.str ());

  // Tell the PGW the address the UE's kernel has: otherwise it allocates
  // another one when the UE attaches, and drops the UE's downlink.
  uint64_t imsi = DynamicCast<NrUeNetDevice> (ueDevs.Get (0))->GetImsi ();
  for (uint32_t i = 0; i < pgw->GetNApplications (); i++)
    {
      Ptr<NrEpcPgwApplication> pgwApp = DynamicCast<NrEpcPgwApplication> (pgw->GetApplication (i));
      if (pgwApp)
        {
          if (!pgwApp->HasUe (imsi))
            {
              pgwApp->AddUe (imsi);
            }
          pgwApp->SetUeAddress (imsi, ueIf.GetAddress (0));
        }
    }

  nrHelper->AddX2Interface (gnbNodes);
  nrHelper->AttachToGnb (ueDevs.Get (0), gnbDevs.Get (0));

  g_events.open ("dce-browser-nr-events.csv");
  g_events << "time,lag,event" << std::endl;
  g_timeline.open ("dce-browser-nr-timeline.csv");
  g_timeline << "time,dlBytes,lag\n";
  g_rsrp.open ("dce-browser-nr-rsrp.csv");
  g_rsrp << "time,cell,rsrp,serving\n";
  ue->RegisterProtocolHandler (MakeCallback (&UeRx), 0, ueDevs.Get (0), false);
  Config::ConnectWithoutContextFailSafe ("/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/NrUePhy/ReportUeMeasurements",
                                         MakeCallback (&Rsrp));
  Config::Connect ("/NodeList/*/DeviceList/*/NrUeRrc/HandoverStart", MakeCallback (&HandoverStart));
  Config::Connect ("/NodeList/*/DeviceList/*/NrUeRrc/HandoverEndOk", MakeCallback (&HandoverEnd));

  // Handovers, alternately to each gNB, each with its injected gap if any.
  g_gap = gap;
  for (uint32_t i = 0; i < 2; i++)
    {
      Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
      em->SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
      em->SetRate (1.0);
      em->Disable ();
      wired.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
      g_gapModels.push_back (em);
    }
  {
    std::istringstream is (hoTimes);
    std::string item;
    uint32_t from = 0;
    while (std::getline (is, item, ','))
      {
        Time at = Seconds (std::stod (item));
        nrHelper->HandoverRequest (at, ueDevs.Get (0), gnbDevs.Get (from), gnbDevs.Get (1 - from));
        from = 1 - from;
      }
  }

  // --- The browser ------------------------------------------------------------
  DceApplicationHelper dce;
  dce.SetStackSize (1 << 24);
  dce.SetBinary ("/opt/chrome/chrome");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  DceX11Helper::SetEnvironment (dce);
  dce.AddEnvironment ("LANG", "C.UTF-8");
  dce.AddEnvironment ("SSLKEYLOGFILE", "/tls-keys.log");
  const char *chromeArgs[] = { "--single-process", "--no-zygote", "--no-sandbox",
                               "--disable-gpu", "--disable-gpu-compositing",
                               "--disable-gpu-rasterization", "--ozone-platform=x11",
                               "--disable-crashpad-for-testing", "--disable-breakpad",
                               "--disable-dev-shm-usage", "--no-first-run",
                               "--no-default-browser-check", "--user-data-dir=/chrome-profile",
                               "--password-store=basic", "--use-mock-keychain",
                               "--autoplay-policy=no-user-gesture-required",
                               "--window-size=1024,640", "--window-position=0,0",
                               "--ssl-key-log-file=/tls-keys.log",
                               // Names through getaddrinfo(), see dce-browser.cc
                               "--disable-features=AsyncDns" };
  for (const char *arg : chromeArgs)
    {
      dce.AddArgument (arg);
    }
  DceX11Helper::UseSessionBus (dce);
  if (!DceX11Helper::UsePulseAudio (ue, dce))
    {
      std::cout << "No PulseAudio socket on the host: the browser will be silent" << std::endl;
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
  dce.SetUid (1000);
  dce.SetEuid (1000);
  dce.SetFinishedCallback (MakeCallback (&BrowserFinished));
  ApplicationContainer apps = dce.Install (ue);
  apps.Start (Seconds (2.0));

  std::cout << "UE " << ueIf.GetAddress (0) << ", gateway " << epc->GetUeDefaultGatewayAddress ()
            << ", opening " << url << std::endl;
  g_wallStart = std::chrono::steady_clock::now ();
  {
    // lets the recordings be lined up with the simulated time
    std::ofstream start ("sim-start-epoch-ms.txt");
    start << std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now ().time_since_epoch ()).count () << std::endl;
  }
  Simulator::Schedule (MilliSeconds (100), &Sample);
  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();
  std::cout << g_handovers << " handovers completed" << std::endl;
  return 0;
}
