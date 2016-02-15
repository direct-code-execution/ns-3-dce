#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

// This is a test case for a couple of bugs that I found.
// It mixes NS3 and DCE-LINUX nodes in a multi-hop network. DCE-linux assumes all nodes are running DCE-linux, but it shouldn't
// It also allows setting memory available to TCP in DCE-Linux, and the requested iperf window size (-w option)


NS_LOG_COMPONENT_DEFINE ("DceIperfMultihop");
// ======================================================================================================================================
// 
//           < - - - - - - Left Side Links - - - - - >    < - - CenterLink - - >    < - - - - - Right Side Links - - - - - >   
// 
//        client               router1               router2                 router N-1             router N             server
//  +----------------+    +----------------+    +----------------+       +----------------+   +----------------+   +----------------+                
//  | iperf(Client)  |    |    router      |    |    router      |       |     router     |   |     router     |   | iperf(Server)  |                
//  +----------------+    +----------------+    +----------------+  ...  +----------------+   +----------------+   +----------------+                
//  |   DCE Linux    |    |      NS3       |    |      NS3       |       |       NS3      |   |       NS3      |   |   DCE Linux    |                
//  +----------------+    +----------------+    +----------------+       +----------------+   +----------------+   +----------------+                
//             |    10.1.0.*    |   |     10.1.1.*   |    |      10.3.0.*      |    |    10.2.1.*   |    |    10.2.0.*    |        
//             +----------------+   +--------//------+    +----------//--------+    +---------------+    +----------------+        
//               point-to-point       point-to-point          point-to-point          point-to-point       point-to-point 
//                  fastLink             slowLink                slowLink              slowLink               fastLink     
// 
// ======================================================================================================================================

// ########################################################################
// Declare callbacks for printing Sysctl values
// ########################################################################
static void PrintSysctlResult_client (std::string key, std::string value);
static void PrintSysctlResult_server (std::string key, std::string value);

int 
main (int argc, char *argv[])
{
  // ----------------------------------------------------------------------
  // Defaults
  // ----------------------------------------------------------------------
  std::string stack                = "linux";
  std::string slowLinkDataRate     = "50Mbps";
  std::string slowLinkDelay        = "30ms";
  uint32_t    numRouters           = 2;
  int         iperfDurationSeconds = 120;

  uint32_t    snapLen              = PcapFile::SNAPLEN_DEFAULT;

  std::string tcp       	   = "";    
  std::string clientTcp 	   = "";
  std::string serverTcp 	   = "";

  std::string netmem 	    	   = "";  
  std::string clientNetmem  	   = "";
  std::string serverNetmem  	   = "";

  std::string tcpWin       	   = "256k";
  std::string clientTcpWin     	   = "";
  std::string serverTcpWin     	   = "";

  // ----------------------------------------------------------------------
  // Create command line options and get them
  // ----------------------------------------------------------------------
  CommandLine cmd;
  cmd.AddValue ("routers",     	"Number of routers between client&server. Default is 2.",  numRouters);
  cmd.AddValue ("stack",       	"Name of IP stack: ns3/linux. Default is linux",           stack);
  cmd.AddValue ("rate",        	"Slow link data rate. Default is 50Mbps.",                 slowLinkDataRate);
  cmd.AddValue ("delay",       	"Slow link delay. Default 50ms.",                          slowLinkDelay);
  cmd.AddValue ("duration",    	"Duration of iperf session. Default is 120.",              iperfDurationSeconds);
  cmd.AddValue ("tcp",         	"Select TCP congestion alg for iPerf.",                    tcp);
  cmd.AddValue ("clientTcp",   	"Select TCP congestion alg for iPerf(client only).",       clientTcp);
  cmd.AddValue ("serverTcp",   	"Select TCP congestion alg for iPerf(server only).",       serverTcp);

  cmd.AddValue ("netmem",      	"Set dce-linux network memory using Sysctl",               netmem);
  cmd.AddValue ("clientNetmem",	"Set dce-linux network memory (on client only).",          clientNetmem);
  cmd.AddValue ("serverNetmem",	"Set dce-linux network memory (on server only).",          serverNetmem);

  cmd.AddValue ("tcpWin",      	"Set dce-linux network memory using Sysctl (256k)",        tcpWin);
  cmd.AddValue ("clientTcpWin",	"Set dce-linux network memory (on client only).",          clientTcpWin);
  cmd.AddValue ("serverTcpWin",	"Set dce-linux network memory (on server only).",          serverTcpWin);

  cmd.AddValue ("snapLen",      "PCAP packet capture length",                              snapLen);

  cmd.Parse (argc, argv);

  // ----------------------------------------------------------------------
  // Check command line options
  // ----------------------------------------------------------------------
  if ( (numRouters < 2) || ((numRouters % 2) == 1)) 
    {
      NS_LOG_ERROR ("ERROR: Number of routers must be even and no less than 2.");
      return 0;
    }

  std::ostringstream iperfDurationArg;
  iperfDurationArg << iperfDurationSeconds;

  // ----------------------------------------------------------------------
  // Set all TCP stacks to use a common congestion control algorithm, if requested
  // ----------------------------------------------------------------------
  if (tcp != "")
    {
      clientTcp = tcp;
      serverTcp = tcp;
    }


  // ----------------------------------------------------------------------
  // Set all TCP window sizes, if requested
  // ----------------------------------------------------------------------
  if (tcpWin != "")
    {
      clientTcpWin = tcpWin;
      serverTcpWin = tcpWin;
    }


  // ----------------------------------------------------------------------
  // Set network memory for dce-linux using sysctl()
  // ----------------------------------------------------------------------
  if (netmem != "")
    {
      clientNetmem = netmem;
      serverNetmem = netmem;
    }

  // ----------------------------------------------------------------------
  // Set PCAP packet capture maximum packet length
  // ----------------------------------------------------------------------
  Config::SetDefault ("ns3::PcapFileWrapper::CaptureSize",   UintegerValue (snapLen));
    
  // ----------------------------------------------------------------------
  // Create nodes
  // ----------------------------------------------------------------------
  Ptr<Node>     client = CreateObject<Node> ();   // 10.1.0.1

  NodeContainer routersL, routersR;
  routersL.Create (numRouters/2);
  routersR.Create (numRouters/2);

  Ptr<Node>     server = CreateObject<Node> ();   // 10.3.0.1

  // ----------------------------------------------------------------------
  // Set some simulator-wide values
  // ----------------------------------------------------------------------
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType", StringValue ("UcontextFiberManager"));

  // ----------------------------------------------------------------------
  // Install the internet stack on each node
  // client and server endpoints are selectable: ns3 or DCE linux (2.6.36)
  // intermediate routers get NS3 
  // ----------------------------------------------------------------------
  InternetStackHelper ns3stack;
  ns3stack.Install (routersL);
  ns3stack.Install (routersR);

#ifdef KERNEL_STACK
  LinuxStackHelper linuxStack;
#endif

  if (stack == "ns3")
    {
      ns3stack.Install (client);
      ns3stack.Install (server);
      dceManager.Install (client);
      dceManager.Install (server);
    }
  else if (stack == "linux")
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      dceManager.Install (client);
      dceManager.Install (server);
      linuxStack.Install (client);
      linuxStack.Install (server);
#else
      NS_LOG_ERROR ("Linux kernel stack for DCE is not available. Re-build with the dce-linux module.");
      // silently exit
      return 0;
#endif
    }

  // ----------------------------------------------------------------------
  // Create fast and slow link types
  // ----------------------------------------------------------------------
  Ipv4AddressHelper address;

  PointToPointHelper fastLink;
  fastLink.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  fastLink.SetChannelAttribute("Delay", StringValue ("1.0us"));

  PointToPointHelper slowLink;
  slowLink.SetDeviceAttribute ("DataRate", StringValue (slowLinkDataRate));
  slowLink.SetChannelAttribute("Delay", StringValue (slowLinkDelay));

  // ----------------------------------------------------------------------
  // Make left side fast connection and assign addresses (10.1.i.*/24)
  // ----------------------------------------------------------------------
  std::vector<NetDeviceContainer> linkL;

  linkL.push_back(fastLink.Install  (client, routersL.Get(0)));
  address.SetBase ("10.1.0.0", "255.255.255.0");
  address.Assign (linkL[0]);

  // ----------------------------------------------------------------------
  // Make extra left side slow connections and assign addresses (10.1.i.*/24)
  // ----------------------------------------------------------------------
  for(int i=1; i<numRouters/2; i++)
    {
      linkL.push_back(slowLink.Install (routersL.Get(i-1), routersL.Get(i)));

      std::ostringstream oss;
      oss << "10.1." << i << ".0";
      address.SetBase (oss.str ().c_str (), "255.255.255.0");
      address.Assign (linkL[i]);
    }

  // ----------------------------------------------------------------------
  // Make slow connection in the middle and assign addresses (10.2.0.*/24)
  // ----------------------------------------------------------------------
  NetDeviceContainer linkM;
  linkM = slowLink.Install (routersL.Get(numRouters/2-1), routersR.Get(numRouters/2-1));

  address.SetBase ("10.2.0.0", "255.255.255.0");
  address.Assign (linkM);

  // ----------------------------------------------------------------------
  // Make extra right side slow connections and assign addresses (10.1.i.*/24)
  // ----------------------------------------------------------------------
  std::vector<NetDeviceContainer> linkR;

  // ----------------------------------------------------------------------
  // Make right side fast connections and assign addresses  (10.3.i.*/24)
  // ----------------------------------------------------------------------
  linkR.push_back (fastLink.Install (server, routersR.Get(0)));
  address.SetBase ("10.3.0.0", "255.255.255.0");
  address.Assign (linkR[0]);

  for(int i=1; i<numRouters/2; i++)
    {
      linkR.push_back(slowLink.Install (routersR.Get(i-1), routersR.Get(i)));

      std::ostringstream oss;
      oss << "10.3." << i << ".0";
      address.SetBase (oss.str ().c_str (), "255.255.255.0");
      address.Assign (linkR[i]);
    }


  // ----------------------------------------------------------------------
  // Calculate and populate routing tables
  // ----------------------------------------------------------------------
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
#ifdef KERNEL_STACK
  if (stack == "linux")
    {
      LinuxStackHelper::PopulateRoutingTables ();
    }
#endif

  // ----------------------------------------------------------------------
  // Launch iperf client on client
  // ----------------------------------------------------------------------
  DceApplicationHelper dce;
  ApplicationContainer apps;

  dce.SetStackSize (1 << 20);

  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
  dce.AddArgument ("10.3.0.1");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument (iperfDurationArg.str().c_str());     

  if (clientTcpWin != "")
    {
      dce.AddArgument ("-w");
      dce.AddArgument (clientTcpWin);
    }

  apps = dce.Install (client);
  apps.Start (Seconds (0.75));
  apps.Stop  (Seconds (iperfDurationSeconds+3.0));

#ifdef KERNEL_STACK
  if (stack == "linux")
    {
      if(clientTcp != "")
	{
	  linuxStack.SysctlSet (client,".net.ipv4.tcp_allowed_congestion_control", clientTcp); // done at 0.1 seconds
	  linuxStack.SysctlSet (client,".net.ipv4.tcp_congestion_control",         clientTcp);
	}

      LinuxStackHelper::SysctlGet (client, Seconds (0.11),
                                   ".net.ipv4.tcp_congestion_control", &PrintSysctlResult_client);

      if(clientNetmem != "")
	{
	  linuxStack.SysctlSet (client, ".net.core.wmem_default", clientNetmem); // done at 0.1 seconds
	  linuxStack.SysctlSet (client, ".net.core.wmem_max",     clientNetmem);
	  linuxStack.SysctlSet (client, ".net.core.rmem_default", clientNetmem);
	  linuxStack.SysctlSet (client, ".net.core.rmem_max",     clientNetmem);
	}

      LinuxStackHelper::SysctlGet (client, Seconds (0.11),
                                   ".net.core.wmem_max",     &PrintSysctlResult_client);
      LinuxStackHelper::SysctlGet (client, Seconds (0.11),
                                   ".net.core.wmem_default", &PrintSysctlResult_client);
      LinuxStackHelper::SysctlGet (client, Seconds (0.11),
                                   ".net.core.rmem_max",     &PrintSysctlResult_client);
      LinuxStackHelper::SysctlGet (client, Seconds (0.11),
                                   ".net.core.rmem_default", &PrintSysctlResult_client);
    }
#endif

  // ----------------------------------------------------------------------
  // Launch iperf server on server
  // ----------------------------------------------------------------------
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");

  if (serverTcpWin != "")
    {
      dce.AddArgument ("-w");
      dce.AddArgument (serverTcpWin);
    }

  apps = dce.Install (server);
  apps.Start (Seconds (0.5));
  apps.Stop  (Seconds (iperfDurationSeconds+3.0));

#ifdef KERNEL_STACK
  if (stack == "linux")
    {
      if (serverTcp != "")
	{
	  linuxStack.SysctlSet (server,".net.ipv4.tcp_allowed_congestion_control", serverTcp);
	  linuxStack.SysctlSet (server,".net.ipv4.tcp_congestion_control",         serverTcp);
	}

      LinuxStackHelper::SysctlGet (server, Seconds (0.12),
                                   ".net.ipv4.tcp_congestion_control", &PrintSysctlResult_server);

      if(serverNetmem != "")
	{
	  linuxStack.SysctlSet (server, ".net.core.wmem_default", serverNetmem); // done at 0.1 seconds
	  linuxStack.SysctlSet (server, ".net.core.wmem_max",     serverNetmem);
	  linuxStack.SysctlSet (server, ".net.core.rmem_default", serverNetmem);
	  linuxStack.SysctlSet (server, ".net.core.rmem_max",     serverNetmem);
	}

      LinuxStackHelper::SysctlGet (server, Seconds (0.12),
                                   ".net.core.wmem_max",     &PrintSysctlResult_server);
      LinuxStackHelper::SysctlGet (server, Seconds (0.12),
                                   ".net.core.wmem_default", &PrintSysctlResult_server);
      LinuxStackHelper::SysctlGet (server, Seconds (0.12),
                                   ".net.core.rmem_max",     &PrintSysctlResult_server);
      LinuxStackHelper::SysctlGet (server, Seconds (0.12),
                                   ".net.core.rmem_default", &PrintSysctlResult_server);
    }
#endif

  // ----------------------------------------------------------------------
  // print routing tables
  // ----------------------------------------------------------------------
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("all_the.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (0), routingStream);

  // ----------------------------------------------------------------------
  // Capture packets on the links
  // ----------------------------------------------------------------------
  fastLink.EnablePcap ("client.pcap", linkL[0].Get(0), true, true); // promiscuous=true, explicitFilename=true
//fastLink.EnablePcap ("server.pcap", linkR[0].Get(0), true, true); // promiscuous=true, explicitFilename=true

  apps.Start (Seconds (0.6));

  Simulator::Stop (Seconds (iperfDurationSeconds + 10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

// ########################################################################
// Callbacks for printing Sysctl values
// ########################################################################
static void
PrintSysctlResult(std::string nodename, std::string key, std::string value)
{
  std::cout << "SYSCTL("
            << nodename
            << "): "
            << key
            << " => "
            << value;
}

static void
PrintSysctlResult_client (std::string key, std::string value)
{
  PrintSysctlResult("client",key,value);
}

static void
PrintSysctlResult_server (std::string key, std::string value)
{
  PrintSysctlResult("server",key,value);
}
