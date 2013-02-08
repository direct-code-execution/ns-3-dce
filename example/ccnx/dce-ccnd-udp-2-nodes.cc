#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "misc-tools.h"

using namespace ns3;


void
CreateFiles ()
{
  std::ofstream osf ("/tmp/README", std::fstream::trunc);
  osf << "The wanted data is here :)" ;
  osf.close ();

  std::ofstream osf2 ("/tmp/getSlash.txt", std::fstream::trunc);
  osf2 << "GET / ";
  osf2.close ();

  std::ofstream osf3 ("/tmp/ccnd1.conf", std::fstream::trunc);
  osf3 << "add ccnx:/ udp 10.1.1.1 9695" << std::endl;
  osf3.close ();

  std::ofstream osf4 ("/tmp/ccnd0.conf", std::fstream::trunc);
  osf4 << "add ccnx:/ udp 10.1.1.2 9695" << std::endl;
  osf4.close ();
}


// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
// Each node run a CCN daemon and each one forward interrests to other one,
//
// then a ccnput is launch on node 0 and a ccnget is launch on node 1
//
// ===========================================================================
int main (int argc, char *argv[])
{
  std::string animFile = "NetAnim.xml";
  bool useKernel = 0;
  int ccnxVersion = 4;

  CommandLine cmd;
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.AddValue ("cv", "Ccnx version 4 for 0.4.x variantes and 5 for 0.5.x variantes, default: 4",
                ccnxVersion);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  CreateFiles ();

  PointToPointHelper pointToPoint;
  //pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  DceManagerHelper dceManager;

  if (!useKernel)
    {
      InternetStackHelper stack;
      stack.Install (nodes);
    }
  else
    {
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      LinuxStackHelper stack;
      stack.Install (nodes);
    }

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  dceManager.Install (nodes);

  //  Names::Add ("NODE_Zero", nodes.Get (0));
  Names::Add ("NODE_One", nodes.Get (1));

  CcnClientHelper dce;
  ApplicationContainer apps, putter, getter;

  dce.SetStackSize (1 << 20);

  // Launch ccn daemon on node 0
  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.AddEnvironment ("CCND_CAP", "50000");
  dce.AddEnvironment ("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment ("CCN_LOCAL_PORT", "9695");

  dce.AddEnvironment ("CCND_CAP", "");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));

  // Configure ccn daemon on node 0 to forward interrests to node 1
  dce.SetBinary ("ccndc");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root");  // USE TO FIND keystore under $HOME/.ccnx/.ccnx_keystore
  dce.AddArgument ("-f");
  dce.AddArgument ("/tmp/ccnd0.conf");
  dce.AddFile ("/tmp/ccnd0.conf","/tmp/ccnd0.conf");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (2.0));

  // Launch ccn daemon on node 1
  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.AddEnvironment ("CCND_CAP", "50000");
  dce.AddEnvironment ("CCND_DEBUG", "-1");
  dce.AddEnvironment ("CCN_LOCAL_PORT", "9695");

  dce.AddEnvironment ("CCND_CAP", "");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (1.0));

  // Configure ccn daemon on node 1 to forward interrests to node 0
  dce.SetBinary ("ccndc");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("-f");
  dce.AddArgument ("/tmp/ccnd1.conf");
  dce.AddFile ("/tmp/ccnd1.conf","/tmp/ccnd1.conf");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (2.0));

  // put a file somewhere on the Internet !
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ((ccnxVersion == 4) ? "ccnput" : "ccnpoke");
  dce.SetStdinFile ("/tmp/README");
  dce.AddFile ("/tmp/README", "/tmp/README");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment ("HOME", "/root");

  putter = dce.Install (nodes.Get (0));
  putter.Start (Seconds (3.0));

  // Try to retrieve the file !
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ((ccnxVersion == 4) ? "ccnget" : "ccnpeek");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment ("HOME", "/root");

  getter = dce.Install (nodes.Get (1));
  getter.Start (Seconds (4.0));

  //  RETRIEVE NODE 0 STATUS : ccndsmoketest -b  send getSlash.txt recv recv
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("-b");
  dce.AddArgument ("send");
  dce.AddArgument ("/tmp/getSlash.txt");
  dce.AddArgument ("timeo");
  dce.AddArgument ("1000");
  dce.AddArgument ("recv");
  dce.AddArgument ("recv");
  dce.AddFile ("/tmp/getSlash.txt","/tmp/getSlash.txt");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (5.0));

  //  RETRIEVE NODE 1 STATUS : ccndsmoketest -b  send getSlash.txt recv recv
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("-b");
  dce.AddArgument ("send");
  dce.AddArgument ("/tmp/getSlash.txt");
  dce.AddArgument ("recv");
  dce.AddArgument ("recv");
  dce.AddFile ("/tmp/getSlash.txt","/tmp/getSlash.txt");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (6.0));

  // Stop node 0's ccnd
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (7.0));

  // Stop node 1's ccnd
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (8.0));

  setPos (nodes.Get (0), 1, 10, 0);
  setPos (nodes.Get (1), 50,10, 0);

  // Create the animation object and configure for specified output
  AnimationInterface anim (animFile);

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


