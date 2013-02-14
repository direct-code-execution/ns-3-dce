// ===========================================================================
//
//   Topology
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
//                100 Mbps, 1 ms
//
// This experience download a 6MB file using a simple TCP socket or
//  CCN over udp (depending on user options)
//
// ===========================================================================

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/trace-helper.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

void
CreateFiles (std::string ccnTransport)
{
  // First create a 6 mb file filled with A.

  FILE *f = fopen ("/tmp/bigone", "w");
  char buffer[1024];

  memset (buffer, 'A', sizeof(buffer));
  for (int i = 0; i < (6 * 1024); i++)
    {
      fwrite (buffer, sizeof(buffer), 1, f);
    }
  fclose (f);

  // 2nd create configuration file for the CCN daemon running on second node
  // content : add ccnx:/ [udp|tcp]  10.1.1.1 9695
  //  ---> Forward interrests using udp or tcp to the other node.
  std::ofstream osf3 ("/tmp/ccnd1.conf", std::fstream::trunc);
  osf3 << "add ccnx:/ " << ccnTransport << " 10.1.1.1 9695" << std::endl;
  osf3.close ();

  // and last create configuration file for the CCN daemon running on first node
  // content : add ccnx:/ [udp|tcp]  10.1.1.2 9695
  //  ---> Forward interrests using udp or tcp to the other node.
  std::ofstream osf4 ("/tmp/ccnd0.conf", std::fstream::trunc);
  osf4 << "add ccnx:/ " << ccnTransport << " 10.1.1.2 9695" << std::endl;
  osf4.close ();
}

int
main (int argc, char *argv[])
{
  std::string animFile = "NetAnim.xml";
  std::string packetSize = "1024";
  std::string ccnTransport = "udp"; // or tcp
  std::string dataRate = "100Mbps";
  bool useKernel = 0;
  bool ccnTest = 0;

  CommandLine cmd;
  bool enableFlowMonitor = false;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.AddValue ("ccnd", "if true use ccnd to exchange the file, else use tcp (default).", ccnTest);
  cmd.AddValue ("size", "packet size for ccnsendchunks. default 1024.", packetSize);
  cmd.AddValue ("ct", "CCN used transport udp or tcp. default udp.", ccnTransport);
  cmd.AddValue ("dr", "DataRate default : 100Mbps", dataRate);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  CreateFiles (ccnTransport);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  devices.Get (0)->SetMtu (10000);
  devices.Get (1)->SetMtu (10000);

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

  Names::Add ("NODE_Zero", nodes.Get (0));
  Names::Add ("NODE_One", nodes.Get (1));

  CcnClientHelper dce;
  ApplicationContainer apps, putter, getter;

  dce.SetStackSize (1 << 20);

  if (ccnTest)
    {
      // Launch ccnd on each nodes.
      dce.SetBinary ("ccnd");
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.AddEnvironment ("CCND_CAP", "50000");
      dce.AddEnvironment ("CCN_LOCAL_PORT", "9695");
      dce.AddEnvironment ("CCND_CAP", "");
      dce.AddEnvironment ("CCND_DEBUG", "2");
      dce.AddEnvironment ("CCND_AUTOREG", "");
      dce.AddEnvironment ("CCND_AUTOREG", "");
      dce.AddEnvironment ("CCND_LISTEN_ON", "");
      dce.AddEnvironment ("CCND_MTU", "");
      dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
      dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
      dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");
      apps = dce.Install (nodes.Get (0));
      apps.Start (Seconds (0.03));
      apps = dce.Install (nodes.Get (1));
      apps.Start (Seconds (0.03));

      // Configure ccn daemon on node 0 to forward interrests to node 1
      dce.SetBinary ("ccndc");
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.AddEnvironment ("HOME", "/root"); // USE TO FIND keystore under $HOME/.ccnx/.ccnx_keystore
      dce.AddArgument ("-f");
      dce.AddArgument ("/tmp/ccnd0.conf");
      dce.AddFile ("/tmp/ccnd0.conf", "/tmp/ccnd0.conf");
      apps = dce.Install (nodes.Get (0));
      apps.Start (Seconds (0.04));

      // PUBLISH DATA FILE ON NODE 1
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetBinary ("ccnsendchunks");
      dce.AddEnvironment ("HOME", "/root");
      dce.SetStdinFile ("/tmp/bigone");
      dce.AddFile ("/tmp/bigone", "/tmp/bigone");
      dce.AddArgument ("-b");
      dce.AddArgument (packetSize);
      dce.AddArgument ("ccnx:/DATAFILE");
      putter = dce.Install (nodes.Get (1));
      putter.Start (Seconds (0.05));

      // NODE 0 RETRIEVE THE DATA FILE
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetBinary ("ccncatchunks2");
      dce.SetStdinFile ("");
      dce.AddArgument ("ccnx:/DATAFILE");
      dce.AddEnvironment ("HOME", "/root");
      getter = dce.Install (nodes.Get (0));
      getter.Start (Seconds (1.0));

      // STOP CCND ON EACH NODES
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetBinary ("ccndsmoketest");
      dce.SetStdinFile ("");
      dce.AddArgument ("kill");
      apps = dce.Install (nodes.Get (0));
      apps.Start (Seconds (59.0));
      apps = dce.Install (nodes.Get (1));
      apps.Start (Seconds (59.0));

      pointToPoint.EnablePcap (std::string ("dce-ccn-xchg-") + packetSize, devices.Get (0), false, false);
    }
  else
    {
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetBinary ("my-serveur");
      dce.AddArgument ("5000");
      dce.AddArgument ("6291456");
      dce.AddArgument ("1");
      apps = dce.Install (nodes.Get (0));
      apps.Start (Seconds (0.1));

      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetBinary ("my-client");
      dce.AddArgument ("10.1.1.1");
      dce.AddArgument ("5000");

      apps = dce.Install (nodes.Get (1));
      apps.Start (Seconds (1.0));

      pointToPoint.EnablePcap ("dce-tcp-xchg", devices.Get (0), false, false);
    }
  // Flow Monitor
  Ptr<FlowMonitor> flowmon;
  if (enableFlowMonitor)
    {
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();

  if (enableFlowMonitor)
    {
      flowmon->SerializeToXmlFile ("dce-ccn-exp1.flowmon", false, false);
    }

  Simulator::Destroy ();

  return 0;
}

