#include "ns3/helper-module.h"
#include "ns3/dce-application-helper.h"
#include "ns3/dce-manager-helper.h"
#include "ns3/simulator-module.h"
#include "ns3/core-module.h"
#include "ns3/node-module.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <fstream>
#include <string>


using namespace ns3;

static std::string Ipv4AddressToString (Ipv4Address ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}

static unsigned long GetUss (void)
{
  pid_t pid = getpid ();
  std::ostringstream oss;
  oss << "examples/process/uss-for-pid.py " << pid;
  FILE *file = popen (oss.str ().c_str (), "r");
  unsigned long uss;
  fscanf (file, "%lu", &uss);
  fclose (file);
  return uss;
}
#if 0
static unsigned long GetNPhysicalPages (void)
{
  return 0;
}
#endif
static unsigned long GetUnsharedSize (void)
{
  struct rusage usage;
  int status;
  status = getrusage (RUSAGE_SELF, &usage);
  NS_ASSERT (status == 0);
  return usage.ru_idrss + usage.ru_isrss;
}

static std::string GetPeakMemoryUsed (void)
{
  std::ifstream is;
  is.open ("/proc/self/status");
  while (!is.eof ())
    {
      std::string line;
      std::getline (is, line);
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

static void PrintMemory (void)
{
  std::cerr << "uss=" << GetUss () << std::endl;
  std::cerr << "peak=" << GetPeakMemoryUsed () << std::endl;
  std::cerr << "unshared=" << GetUnsharedSize () << std::endl;
}


int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::Ipv4L3Protocol::DefaultTtl", UintegerValue (255));
  std::string delay = "1ns";
  std::string rate = "5Mbps";
  uint32_t size = 1;
  CommandLine cmd;
  cmd.AddValue ("Delay", "link delay", delay);
  cmd.AddValue ("Bandwidth", "link bandwidth", rate);
  cmd.AddValue ("Size", "number of nodes in ring", size);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (size);

  Ipv4StaticRoutingHelper staticHelper;
  InternetStackHelper stack;
  stack.SetRoutingHelper (staticHelper);
  stack.Install (nodes);

  DceManagerHelper processManager;
  processManager.Install (nodes);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (rate));
  p2p.SetChannelAttribute ("Delay", StringValue (delay));
  std::vector<NetDeviceContainer> devs;
  for (uint32_t i = 0; i < size; i++)
    {
      NodeContainer linkNodes;
      linkNodes.Add (nodes.Get (i));
      linkNodes.Add (nodes.Get ((i+1)%size));
      NetDeviceContainer dev = p2p.Install (linkNodes);
      devs.push_back (dev);
    }
  
  std::vector<std::pair<Ptr<Node>, Ipv4Address> > pairs;

  Ipv4AddressHelper addressAlloc;
  Ipv4Mask mask = Ipv4Mask ("255.255.0.0");
  addressAlloc.SetBase ("10.1.0.0", mask);
  for (uint32_t i = 0; i < size; i++)
    {
      Ptr<Node> node = nodes.Get (i);
      Ptr<Node> nextNode = nodes.Get ((i+1)%size);
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      Ptr<Ipv4> nextIpv4 = nextNode->GetObject<Ipv4> ();
      Ptr<NetDevice> dev = node->GetDevice (2);
      Ptr<NetDevice> nextDev = nextNode->GetDevice (1);
      Ipv4Address ip = addressAlloc.NewAddress ();
      Ipv4Address nextIp = addressAlloc.NewAddress ();
      uint32_t interf = ipv4->AddInterface (dev);
      uint32_t nextInterf = nextIpv4->AddInterface (nextDev);
      ipv4->AddAddress (interf, Ipv4InterfaceAddress (ip, mask));
      nextIpv4->AddAddress (nextInterf, Ipv4InterfaceAddress (nextIp, mask));
      ipv4->SetUp (interf);
      nextIpv4->SetUp (nextInterf);

      Ptr<Ipv4StaticRouting> routing = DynamicCast<Ipv4StaticRouting> (ipv4->GetRoutingProtocol ());
      routing->SetDefaultRoute (nextIp, interf, 0);

      pairs.push_back (std::make_pair (nextNode, ip));
    }

  for (uint32_t i = 0; i < pairs.size (); i++)
    {
      std::pair<Ptr<Node>,Ipv4Address> pair = pairs[i];
      DceApplicationHelper process;
      process.SetStackSize (1<<16);
      process.SetBinary ("build/debug/ping");
      process.AddArgument ("-c");
      process.AddArgument ("1");
      process.AddArgument (Ipv4AddressToString (pair.second));
      ApplicationContainer apps;
      apps = process.Install (pair.first);
      apps.Start (Seconds (2.0));
    }

  //p2p.EnablePcapAll ("process-ring");

  PrintMemory ();
  Simulator::Schedule (Seconds (2.00001), &PrintMemory);

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
