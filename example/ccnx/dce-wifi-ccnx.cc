/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 *  Tested with ccnx 0.8.1 (compiled with MORE_LDLIBS=-pie)
 *  To enable extended log set 
 *           export NS_LOG=dce-wifi-ccnx 
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/dce-module.h"

#include "ns3/rectangle.h"



NS_LOG_COMPONENT_DEFINE ("DceWifiCcnx");

using namespace ns3;


// **************************************************************
//                   Simulation configuration
// **************************************************************
const double SIMULATION_LENGTH = 20;

const char *CCN_NAME = "ccnx:/NODE0/LeData";
const char *TMP_FILE_NAME = "/tmp/DATAFILE";


// **************************************************************
//                   CCNX experiment
// **************************************************************

class ManetCcnx
{
public:
  // Configuration parameters
  std::string phyMode;

  uint32_t numNodes;
  bool verbose;
  bool tracing;
    std::string animFile;
  bool useKernel;
  bool useTcp;
  double distance;

    ManetCcnx (int argc, char *argv[]);
   ~ManetCcnx ();
  void Simulation ();

private:
  NodeContainer nodeContainer;
  Ipv4InterfaceContainer interfaceContainer;
  CcnClientHelper dce;
  ApplicationContainer apps;

  void ConfigureTopology ();
  void ConfigureCCN ();

  void ScheduleCcnpoke (const uint32_t node, const double time,
			const char *ccnname, const char *fname);
  void ScheduleCcnpeek (const uint32_t node, const double time,
			const char *ccnname);
  void ScheduleCcndc (const uint32_t node, const double time,
		     const uint32_t destNode);
  void ScheduleCcnd (const uint32_t node, const double time);
  void ScheduleCcndsmoketest (const uint32_t node, const double time);

  static void CreateDataFile ();
  static std::string Ipv4AddressToString (const ns3::Ipv4Address & ad);

};


// **************************************************************

ManetCcnx::ManetCcnx (int argc, char *argv[]):
phyMode ("DsssRate1Mbps"),
numNodes (25),
verbose (false),
tracing (true),
animFile ("netanim-dce-wifi-ccnx.xml"),
useKernel (false),
useTcp (true),
distance (400)
{
  CreateDataFile ();
  CommandLine cmd;

  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components",
		verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numNodes", "number of nodes", numNodes);
  cmd.AddValue ("useKernel", "Use kernel stack", useKernel);
  cmd.AddValue ("tcp", "Use TCP to link ccnd daemons.", useTcp);

  cmd.Parse (argc, argv);
}

ManetCcnx::~ManetCcnx ()
{
}

// Create a temporary file in temp dir. It will be used for ccnpoke stdin
void
ManetCcnx::CreateDataFile ()
{
  mkdir ("files-0", 0777);
  mkdir ("files-0/tmp", 0777);

  std::string flname ("files-0");
  flname.append (TMP_FILE_NAME);
  NS_LOG_UNCOND ("Creating data file: " << flname);

  std::ofstream osf (flname.c_str ());
  osf << "THE CCN DATA! ABCDEFG\n";
  osf.close ();
}



std::string ManetCcnx::Ipv4AddressToString (const ns3::Ipv4Address & ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}


// **************************************************************
//                   CCNx experiment
// **************************************************************

void
ManetCcnx::ScheduleCcnpoke (const uint32_t node, const double time,
			    const char *ccnname, const char *fname)
{
  NS_LOG_INFO ("Scheduling ccnpoke " << ccnname <<
	       " on node " << node << " at " << time << " s");
  NS_LOG_DEBUG ("ccnpoke -v -x 354000 " << ccnname << " < " << fname);
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root");
  dce.SetBinary ("ccnpoke");
  dce.SetStdinFile (fname);
  dce.AddArgument ("-v");
  dce.AddArgument ("-x");
  dce.AddArgument ("354000");
  dce.AddArgument (ccnname);

  apps = dce.Install (nodeContainer.Get (node));
  apps.Start (Seconds (time));
}

void
ManetCcnx::ScheduleCcnpeek (const uint32_t node, const double time,
			    const char *ccnname)
{
  NS_LOG_INFO ("Scheduling ccnpeek " << ccnname <<
	       " on node " << node << " at " << time << " s");
  NS_LOG_DEBUG ("ccnpeek -c -a " << ccnname );

  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root");
  dce.AddEnvironment ("CCN_LINGER", "3540");
  dce.SetBinary ("ccnpeek");
  dce.SetStdinFile ("");
  dce.AddArgument ("-c");
  dce.AddArgument ("-a");
  dce.AddArgument (ccnname);

  apps = dce.Install (nodeContainer.Get (node));
  apps.Start (Seconds (time));
}

void
ManetCcnx::ScheduleCcndc (const uint32_t node, const double time,
			 const uint32_t destNode)
{
  std::string dest =
    Ipv4AddressToString (interfaceContainer.GetAddress (destNode, 0));
  NS_LOG_INFO ("Scheduling ccndc on node " << node <<
	       " for node " << destNode << " (" << dest << ") at " <<
	       time << " s");
  NS_LOG_DEBUG ("ccndc -v add ccnx:/NODE0 " << (useTcp ? "tcp" : "udp") <<
                 dest );

  dce.SetBinary ("ccndc");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("-v");
  dce.AddArgument ("add");
  dce.AddArgument ("ccnx:/NODE0");
  dce.AddArgument (useTcp ? "tcp" : "udp");

  dce.AddArgument (dest);

  apps = dce.Install (nodeContainer.Get (node));
  apps.Start (Seconds (time));

}

void
ManetCcnx::ScheduleCcnd (const uint32_t node, const double time)
{
  NS_LOG_INFO ("Scheduling ccnd on node " << node << " at " << time << " s");

  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.AddEnvironment ("CCND_CAP", "50000");
  dce.AddEnvironment ("CCND_DEBUG", "64");
  dce.AddEnvironment ("CCN_LOCAL_PORT", "");

  dce.AddEnvironment ("CCND_CAP", "");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");


  apps = dce.Install (nodeContainer.Get (node));
  apps.Start (Seconds (time));
}

void
ManetCcnx::ScheduleCcndsmoketest (const uint32_t node, const double time)
{
  NS_LOG_INFO ("Scheduling ccndsmoketest on node " << node << " at " << time
	       << " s");

  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");

  apps = dce.Install (nodeContainer.Get (node));
  apps.Start (Seconds (time));
}

/**
 * Configuration of CCN daemons and programs.
 */
void
ManetCcnx::ConfigureCCN ()
{
  NS_LOG_INFO ("Scheduling ccnd on all nodes");
  double schedulingTime = 0.5;
  for (int n = 0; n < numNodes; n++)
    {
      dce.SetStackSize (1 << 20);
      ScheduleCcnd (n, schedulingTime);
      schedulingTime += 0.01;
    }

  // After 0.5 seconds, start to schedule  ccndc
  schedulingTime += 5.0;

  NS_LOG_UNCOND ("Scheduling ccndc instances");
  const int rowlen = (int) sqrt (numNodes);
  for (uint32_t n0 = 1; n0 < numNodes; n0++)
    {
      // Forward /NODE0 interrest to prec node
      ScheduleCcndc (n0, schedulingTime, n0 - 1);
      schedulingTime += 0.01;

      if (n0 >= rowlen)
	{
	  ScheduleCcndc (n0, schedulingTime, n0 - rowlen);
	  schedulingTime += 0.01;
	}
    }
   
 
  NS_LOG_UNCOND ("Scheduling command to publish and retrieve data");
  // Publish a file on node 0 : ccnpoke CCN_NAME < TMP_FILE_NAME
  schedulingTime += 0.5;
  ScheduleCcnpoke (0, schedulingTime, CCN_NAME, TMP_FILE_NAME);

  // Retrieve the data: ccnpeek CCN_NAME
  schedulingTime += 5;
  ScheduleCcnpeek (numNodes - 2, schedulingTime, CCN_NAME);

  schedulingTime += 5;
  ScheduleCcnpeek (numNodes - 1, schedulingTime, CCN_NAME);


  // Stop ccnd before end of simu.
  for (int n = 0; n < numNodes; n++)
    {
      ScheduleCcndsmoketest (n, SIMULATION_LENGTH - 1.0);
    }
}



// **************************************************************
//                   Topology and simulation
// **************************************************************

/**
 * Configure and run the simulation.
 */
void
ManetCcnx::ConfigureTopology ()
{
  NS_LOG_UNCOND ("Network Configuration");
  // -----------------------------------------------
  // Wifi configuration

  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
		      StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
		      StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
		      StringValue (phyMode));

  NS_LOG_DEBUG ("Creating " << numNodes << " nodes");
  nodeContainer.Create (numNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi = WifiHelper::Default ();
  if (verbose)
    {
      // Turn on all Wifi logging
      wifi.EnableLogComponents ();
    }

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10));
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
				"DataMode", StringValue (phyMode),
				"ControlMode", StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodeContainer);

  // Topology configuration
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				 "MinX", DoubleValue (0.0),
				 "MinY", DoubleValue (0.0),
				 "DeltaX", DoubleValue (distance),
				 "DeltaY", DoubleValue (distance),
				 "GridWidth", UintegerValue (5),
				 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodeContainer);

  InternetStackHelper internet;
  internet.Install (nodeContainer);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  interfaceContainer = ipv4.Assign (devices);

  if (tracing)
    {
      AsciiTraceHelper ascii;
      wifiPhy.EnablePcap ("dce-wifi-ccn", devices);
    }

}




void
ManetCcnx::Simulation ()
{
  ConfigureTopology ();

  NS_LOG_UNCOND ("DCE initialization");
  DceManagerHelper dceManager;
  if (useKernel)
    {
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library",
				  StringValue ("liblinux.so"));
    }

  dceManager.Install (nodeContainer);

  // Create the animation object and configure for specified output
  AnimationInterface anim (animFile);
  anim.EnablePacketMetadata (true);

  ConfigureCCN ();

  Simulator::Stop (Seconds (SIMULATION_LENGTH));
  Simulator::Run ();
  Simulator::Destroy ();
}



// **************************************************************


int
main (int argc, char *argv[])
{
  std::cout << "CCN over WiFi" << std::endl;
  ManetCcnx experiment (argc, argv);
  experiment.Simulation ();

  return 0;
}
