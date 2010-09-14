#include "ns3/helper-module.h"
#include "ns3/dce-application-helper.h"
#include "ns3/dce-manager-helper.h"
#include "ns3/simulator-module.h"
#include "ns3/core-module.h"
#include "ns3/node-module.h"
#include <fstream>

using namespace ns3;

static std::string GetPeakMemoryUsed (void)
{
  std::ifstream is;
  is.open ("/proc/self/status");
  while (!is.eof ())
    {
      std::string line;
      getline (is, line);
      std::string::size_type i = line.find ("VmPeak");
      if (i == 0)
	{
	  is.close();
	  return line;
	}
    }
  is.close ();
  return "";
}


static std::string Ipv4AddressToString (Ipv4Address ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}


int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::Ipv4L3Protocol::DefaultTtl", UintegerValue (255));
  std::string delay = "1ns";
  std::string rate = "5Mbps";
  uint32_t nHops = 1;
  CommandLine cmd;
  cmd.AddValue ("Delay", "link delay", delay);
  cmd.AddValue ("Bandwidth", "link bandwidth", rate);
  cmd.AddValue ("nHops", "number of hops between source and destination", nHops);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (nHops+1);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (rate));
  p2p.SetChannelAttribute ("Delay", StringValue (delay));
  std::vector<NetDeviceContainer> devs;
  for (uint32_t i = 0; i < nHops; i++)
    {
      NodeContainer linkNodes;
      linkNodes.Add (nodes.Get (i));
      linkNodes.Add (nodes.Get (i+1));
      NetDeviceContainer dev = p2p.Install (linkNodes);
      devs.push_back (dev);
    }
#if 0
  Ipv4NixVectorHelper nixRouting; 
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list; 
  list.Add (staticRouting, 0); 
  list.Add (nixRouting, 10);
#endif
  // add an ip stack
  InternetStackHelper stack; 
  //stack.SetRoutingHelper (list);
  //stack.SetTcp ("ns3::NscTcpL4Protocol", "Library", StringValue ("liblinux2.6.26.so"));
  stack.Install (nodes);

  // start the ping binaries.

  // ip address assignment
  Ipv4AddressHelper ipv4;
  std::vector<Ipv4InterfaceContainer> interfaces;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  for (std::vector<NetDeviceContainer>::const_iterator i = devs.begin (); i != devs.end (); ++i)
    {
      Ipv4InterfaceContainer interface = ipv4.Assign (*i);
      interfaces.push_back (interface);
      ipv4.NewNetwork ();
    }

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // start the ping binaries.
  DceManagerHelper processManager;
  processManager.Install (nodes);

  DceApplicationHelper process;
  process.SetStackSize (1<<16);
  ApplicationContainer apps;

  Ptr<Node> serverNode = nodes.Get (nHops);
  Ipv4Address serverAddress = interfaces[nHops-1].GetAddress (1, 0);

  process.SetBinary ("build/debug/ping");
  process.AddArgument ("-c");
  process.AddArgument ("1");
  process.AddArgument (Ipv4AddressToString (serverAddress));

  for (uint32_t i = 0; i < 1; i++)
    {
      Ptr<Node> clientNode = nodes.Get (0);
      apps = process.Install (clientNode);
      apps.Start (Seconds (2.0));
    }


  //p2p.EnablePcapAll ("process-iperf");
  std::cerr << GetPeakMemoryUsed () << std::endl;
  //Simulator::Stop (Seconds (0.0));
  Simulator::Run ();
  std::cerr << GetPeakMemoryUsed () << std::endl;
  Simulator::Destroy ();


  return 0;
}
