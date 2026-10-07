/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Runs the x11-hello X11 client (example/x11-hello.cc) as a DCE application
// on a simulated node: the X11 client library runs inside the simulation
// and reaches the host X server ($DISPLAY) through the host socket
// passthrough of DCE (DceHostUnixSocketPaths = /tmp/.X11-unix/). A window
// drawn from inside the simulation appears on the display for --seconds.
//
// Exits with the exit status of the client (1 if it could not open the
// display), so that it can be run as a test, e.g. under xvfb-run.
//
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"

using namespace ns3;

static int g_exitStatus = -1;

static void
ClientFinished (uint16_t pid, int status)
{
  std::cout << "x11-hello (pid " << pid << ") exited with status " << status
            << " at t=" << Simulator::Now ().GetSeconds () << "s" << std::endl;
  g_exitStatus = status;
  Simulator::Stop ();
}

int
main (int argc, char *argv[])
{
  std::string binary = "x11-hello";
  double seconds = 5.0;
  std::string display = getenv ("DISPLAY") ? getenv ("DISPLAY") : "";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("binary", "X11 client (DCE application) to run", binary);
  cmd.AddValue ("seconds", "How long the client keeps its window open", seconds);
  cmd.AddValue ("display", "X display the client connects to (default: $DISPLAY)", display);
  cmd.Parse (argc, argv);

  if (display == "")
    {
      std::cerr << "No display: set DISPLAY or use --display" << std::endl;
      return 1;
    }

  // The X server lives in wall-clock time.
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  Config::SetDefault ("ns3::RealtimeSimulatorImpl::SynchronizationMode",
                      EnumValue (RealtimeSimulatorImpl::SYNC_BEST_EFFORT));
  // AF_UNIX connections of DCE applications to the X server sockets go to the host.
  DceX11Helper::Enable ();

  NodeContainer nodes;
  nodes.Create (1);
  InternetStackHelper stack;
  stack.Install (nodes);
  DceManagerHelper dceManager;
  dceManager.Install (nodes);

  // The client reads its X authority cookies in its own file system.
  DceX11Helper::InstallAuthority (nodes.Get (0));

  DceApplicationHelper dce;
  dce.SetStackSize (1 << 20);
  dce.SetBinary (binary);
  dce.ResetArguments ();
  std::ostringstream secondsArg;
  secondsArg << (int)seconds;
  dce.AddArgument (secondsArg.str ());
  dce.ResetEnvironment ();
  DceX11Helper::SetEnvironment (dce, display);
  dce.SetFinishedCallback (MakeCallback (&ClientFinished));
  ApplicationContainer apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.1));

  std::cout << "Running " << binary << " inside the simulation, window on display " << display << std::endl;
  Simulator::Stop (Seconds (seconds + 10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  if (g_exitStatus != 0)
    {
      std::cerr << "FAIL: " << binary << (g_exitStatus < 0 ? " did not finish" : " failed")
                << ", see files-0/var/log/*/stderr" << std::endl;
      return 1;
    }
  std::cout << "PASS: " << binary << " drew its window from inside the simulation" << std::endl;
  return 0;
}
