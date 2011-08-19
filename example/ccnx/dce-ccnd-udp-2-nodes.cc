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
  std::string animFile = "NetAnim.tr";
  bool useKernel = 0;
  CommandLine cmd;
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  //pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  if (!useKernel)
    {
      InternetStackHelper stack;
      stack.Install (nodes);

      Ipv4AddressHelper address;
      address.SetBase ("10.1.1.0", "255.255.255.252");
      Ipv4InterfaceContainer interfaces = address.Assign (devices);

      // setup ip routes
      Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    }

  DceManagerHelper dceManager;

  if (useKernel)
    {
      dceManager.SetNetworkStack("ns3::LinuxSocketFdFactory", "Library", StringValue ("libnet-next-2.6.so"));

      AddAddress (nodes.Get (0), Seconds (0.1), "sim0", "10.1.1.1/8");
      RunIp (nodes.Get (0), Seconds (0.2), "link set sim0 up arp off");

      AddAddress (nodes.Get (1), Seconds (0.3), "sim0","10.1.1.2/8");
      RunIp (nodes.Get (1), Seconds (0.4), "link set sim0 up arp off");

    }
  dceManager.Install (nodes);

  //  Names::Add ("NODE_Zero", nodes.Get (0));
  Names::Add ("NODE_One", nodes.Get (1));

  DceApplicationHelper dce;
  ApplicationContainer apps, putter, getter;

  dce.SetStackSize (1<<20);

  // Launch ccn daemon on node 0
  dce.SetBinary ("ccnd");
  dce.ResetArguments();
  dce.ResetEnvironment();

  dce.AddEnvironment("CCND_CAP", "50000");
  dce.AddEnvironment("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment("CCN_LOCAL_PORT", "9695");

  dce.AddEnvironment("CCND_CAP", "");
  dce.AddEnvironment("CCND_AUTOREG", "");
  dce.AddEnvironment("CCND_LISTEN_ON", "");
  dce.AddEnvironment("CCND_MTU", "");
  dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));

  // Configure ccn daemon on node 0 to forward interrests to node 1
  dce.SetBinary ("ccndc");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("HOME", "/home/furbani");  // USE TO FIND keystore under $HOME/.ccnx/.ccnx_keystore
  dce.AddArgument ("-f");
  dce.AddArgument ("/tmp/ccnd0.conf");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (2.0));

  // Launch ccn daemon on node 1
  dce.SetBinary ("ccnd");
  dce.ResetArguments();
  dce.ResetEnvironment();

  dce.AddEnvironment("CCND_CAP", "50000");
  dce.AddEnvironment("CCND_DEBUG", "-1");
  dce.AddEnvironment("CCN_LOCAL_PORT", "9695");

  dce.AddEnvironment("CCND_CAP", "");
  dce.AddEnvironment("CCND_AUTOREG", "");
  dce.AddEnvironment("CCND_LISTEN_ON", "");
  dce.AddEnvironment("CCND_MTU", "");
  dce.AddEnvironment("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment("CCND_KEYSTORE_DIRECTORY", "");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (1.0));

  // Configure ccn daemon on node 1 to forward interrests to node 0
  dce.SetBinary ("ccndc");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddEnvironment("HOME", "/home/furbani");
  dce.AddArgument ("-f");
  dce.AddArgument ("/tmp/ccnd1.conf");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (2.0));

  // put a file somewhere on the Internet !
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccnput");
  dce.SetStdinFile ("/tmp/README");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment("HOME", "/home/furbani");

  putter = dce.Install (nodes.Get (0));
  putter.Start (Seconds (3.0));

  // Try to retrieve the file !
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccnget");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("ccnx:/LeReadme");
  dce.AddEnvironment("HOME", "/home/furbani");

  getter = dce.Install (nodes.Get (1));
  getter.Start (Seconds (4.0));

  //  RETRIEVE NODE 0 STATUS : ccndsmoketest -b  send getSlash.txt recv recv
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("-b");
  dce.AddArgument ("send");
  dce.AddArgument ("/tmp/getSlash.txt");
  dce.AddArgument ("timeo");
  dce.AddArgument ("1000");
  dce.AddArgument ("recv");
  dce.AddArgument ("recv");

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (5.0));

  //  RETRIEVE NODE 1 STATUS : ccndsmoketest -b  send getSlash.txt recv recv
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("-b");
  dce.AddArgument ("send");
  dce.AddArgument ("/tmp/getSlash.txt");
  dce.AddArgument ("recv");
  dce.AddArgument ("recv");

  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (6.0));

  // Stop node 0's ccnd
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (7.0));

  // Stop node 1's ccnd
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  apps = dce.Install (nodes.Get (1));
  apps.Start (Seconds (8.0));

  setPos (nodes.Get (0), 1, 10, 0);
  setPos (nodes.Get (1), 50,10, 0);

  // Create the animation object and configure for specified output
  AnimationInterface anim;

  anim.SetOutputFile (animFile);

  anim.StartAnimation ();

  Simulator::Stop (Seconds(10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  anim.StopAnimation ();

  return 0;
}


