#include "ns3/dce-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/data-rate.h"

using namespace ns3;

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
  bool useKernel = 1;
  cmd.AddValue ("Delay", "link delay", delay);
  cmd.AddValue ("Bandwidth", "link bandwidth", rate);
  cmd.AddValue ("nHops", "number of hops between source and destination", nHops);
  cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (nHops + 1);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (rate));
  p2p.SetChannelAttribute ("Delay", StringValue (delay));
  std::vector<NetDeviceContainer> devs;
  for (uint32_t i = 0; i < nHops; i++)
    {
      NodeContainer linkNodes;
      linkNodes.Add (nodes.Get (i));
      linkNodes.Add (nodes.Get (i + 1));
      NetDeviceContainer dev = p2p.Install (linkNodes);
      devs.push_back (dev);
    }

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
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  // start the ping binaries.
  dceManager.Install (nodes);

  Ipv4Address serverAddress = interfaces[nHops - 1].GetAddress (1, 0);
  Ipv4Address clientAddress = interfaces[0].GetAddress (0, 0);

  if (useKernel)
    {
      // Set the routes
      for (uint32_t n = 0; n < nHops + 1; n++)
        {
          if (n < nHops)
            {
              Ipv4Address monte = interfaces[n].GetAddress (0, 0);
              LinuxStackHelper::RunIp (nodes.Get (n), Seconds (0.1), "route add "
                                       + Ipv4AddressToString (serverAddress) + "/32 via " + Ipv4AddressToString (monte));
            }
          if (n > 1)
            {
              Ipv4Address desc = interfaces[n - 1].GetAddress (1, 0);
              LinuxStackHelper::RunIp (nodes.Get (n), Seconds (0.1), "route add "
                                       + Ipv4AddressToString (clientAddress) + "/32 via " + Ipv4AddressToString (desc));
            }
        }
    }
  else
    {
      Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    }

  DceApplicationHelper process;
  process.SetStackSize (1 << 16);
  ApplicationContainer apps;

  Ptr<Node> serverNode = nodes.Get (nHops);
  Ptr<Node> clientNode = nodes.Get (0);
  //Ipv4Address serverAddress = interfaces[nHops-1].GetAddress (1, 0);

  std::ostringstream oss;

  process.SetBinary ("udp-perf");
  process.AddArgument ("--duration=10");
  oss << "--nodes=" << (nHops + 1);
  process.AddArgument (oss.str ().c_str ());
  apps = process.Install (serverNode);
  apps.Start (Seconds (1.0));

  process.SetBinary ("udp-perf");
  process.ResetArguments ();
  process.AddArgument ("--client");
  process.AddArgument (oss.str ().c_str ());
  oss.clear ();
  oss.str ("");
  oss << "--host=" << Ipv4AddressToString (serverAddress);
  process.AddArgument (oss.str ().c_str ());
  oss.clear ();
  oss.str ("");
  oss << "--bandwidth=" << DataRate (rate).GetBitRate ();
  process.AddArgument (oss.str ().c_str ());
  process.AddArgument ("--duration=10");
  apps = process.Install (clientNode);
  apps.Start (Seconds (2.0));

  //PointToPointHelper::EnablePcapAll ("process-iperf");

  Simulator::Stop (Seconds (14.0));
  Simulator::Run ();

  std::vector <ProcStatus> v = dceManager.GetProcStatus ();

  for (std::vector <ProcStatus>::iterator i = v.begin (); i != v.end () ; ++i)
    {
      ProcStatus st = *i;
      const char *interest = "udp-perf";
      if (st.GetCmdLine ().compare (0, strlen (interest), interest) == 0)
        {
          if (0 != st.GetNode ())
            {
              // This is the server
              char stdoutname[1024];
              sprintf (stdoutname, "files-%d/var/log/%d/stdout", st.GetNode (), st.GetPid ());
              FILE *f = fopen (stdoutname,"r");
              char tmp[1024];
              int l = fread (tmp, 1, sizeof (tmp), f);
              tmp[l] = 0;
              fclose (f);
              unsigned long pn = 0;
              l--;
              // Seek the last number must be the total packet number.
              int lvl = 1;
              while ((l >= 0) && (!isdigit (tmp[l])))
                {
                  l--;
                }
              while ((l >= 0) && (isdigit (tmp[l])))
                {
                  pn += lvl * (tmp[l] - '0');
                  lvl *= 10;
                  l--;
                }
              std::cout << "Packet Number:" << pn << std::endl;
              std::cout << "Real duration : " << st.GetRealDuration () << std::endl;

              long theoric = (long)(DataRate (rate).GetBitRate ()) / 8.5; // (long) (DataRate (rate).GetBitRate ())/8.192F;

              std::cout <<  "DataRate:" << theoric << std::endl;
              if (st.GetRealDuration () > 0)
                {
                  // Packets / seconds
                  unsigned long pps = pn / st.GetRealDuration ();
                  unsigned long Bps = pn * 1500 / st.GetRealDuration ();
                  std::cout << "Packets / second :" << pps << std::endl;
                  std::cout << "Bytes / second" << Bps << std::endl;
                  float rapport = (float)Bps / (float)theoric;
                  std::cout << "Ratio : " << rapport << std::endl;
                }
              if (st.GetRealDuration () <= 10)
                {
                  std::cout << "OK " << pn << " TAKE LESS OR EQUAL TIME"  << std::endl;
                }
              else
                {
                  std::cout << "KO " << pn << " TAKE MORE TIME THAT REALITY"  << std::endl;
                }
            }
        }
    }
  Simulator::Destroy ();

  return 0;
}
