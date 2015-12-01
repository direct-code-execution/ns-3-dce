#include "ns3/test.h"
#include "ns3/dce-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

static std::string g_testError;

using namespace ns3;
namespace ns3 {

class DceCradleTestCase : public TestCase
{
public:
  DceCradleTestCase (std::string testname, Time maxDuration, std::string sock_factory, bool skip);
private:
  virtual void DoRun (void);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_testname;
  Time m_maxDuration;
  std::string m_sockf;
  bool m_skip;
};

DceCradleTestCase::DceCradleTestCase (std::string testname, Time maxDuration, std::string sock_factory, bool skip)
  : TestCase (std::string ("") + (skip ? "(SKIP) " : "" ) + testname),
    m_testname (testname), 
    m_maxDuration (maxDuration),
    m_sockf (sock_factory), 
    m_skip (skip)
{
//  mtrace ();
}
void
DceCradleTestCase::Finished (int *pstatus, uint16_t pid, int status)
{
  *pstatus = status;
}
void
DceCradleTestCase::DoRun (void)
{
  if (m_skip)
    {
      return;
    }


  std::string m_rate = "100Bps";
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("100ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.SetNetworkStack("ns3::LinuxSocketFdFactory",
                             "Library", StringValue ("liblinux.so"));
  dceManager.Install (nodes);

  LinuxStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Ipv6AddressHelper address6;
  address6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer interfaces6 = address6.Assign (devices);


  ApplicationContainer apps;
  PacketSinkHelper sink = PacketSinkHelper (m_sockf,
                                            InetSocketAddress (Ipv4Address::GetAny (), 9));
  OnOffHelper onoff = OnOffHelper (m_sockf,
                                   InetSocketAddress (interfaces.GetAddress (1), 9));

  if (m_testname.find ("6", 0) == std::string::npos)
    {
      onoff = OnOffHelper (m_sockf,
                           InetSocketAddress (interfaces.GetAddress (1), 9));
      sink = PacketSinkHelper (m_sockf,
                               InetSocketAddress (Ipv4Address::GetAny (), 9));
    }
  else
    {
      onoff = OnOffHelper (m_sockf,
                           Inet6SocketAddress (interfaces6.GetAddress (1, 1), 9));
      sink = PacketSinkHelper (m_sockf,
                               Inet6SocketAddress (Ipv6Address::GetAny (), 9));
    }

  onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute ("PacketSize", StringValue ("1024"));
  onoff.SetAttribute ("DataRate", StringValue (m_rate));
  apps = onoff.Install (nodes.Get (0));
  apps.Start (Seconds (4.0));

  apps = sink.Install (nodes.Get (1));
  apps.Start (Seconds (3.9999));

  if (m_maxDuration.IsStrictlyPositive ()) {
      Simulator::Stop (m_maxDuration);
    }
  Simulator::Run ();

  Ptr<PacketSink> pktsink;
  pktsink = apps.Get (0)->GetObject<PacketSink> ();;

#if 0
  std::cout << "Total Rx(0) = " << pktsink->GetTotalRx () << 
    " bytes";
  std::cout << std::endl;
  pointToPoint.EnablePcapAll ("dce-cradle-test");
#endif
  Simulator::Destroy ();

  int status = (pktsink->GetTotalRx () > 0);
  NS_TEST_ASSERT_MSG_EQ (status, 1, "Process did not return successfully: " << g_testError);
}

static class DceCradleTestSuite : public TestSuite
{
public:
  DceCradleTestSuite ();
private:
} g_processTests;
//


DceCradleTestSuite::DceCradleTestSuite ()
  : TestSuite ("dce-cradle", UNIT)
{
  typedef struct {
    const char *name;
    const char *sockf;
    int duration;
    bool isSkip;
  } testPair;

  testPair tests[] = {
    {"raw", "ns3::LinuxIpv4RawSocketFactory", 30, false},
    {"udp", "ns3::LinuxUdpSocketFactory", 30, false},
    {"tcp", "ns3::LinuxTcpSocketFactory", 30, false},
    {"dccp", "ns3::LinuxDccpSocketFactory", 30, false},
    {"sctp", "ns3::LinuxSctpSocketFactory", 20, false},
    {"raw6", "ns3::LinuxIpv6RawSocketFactory", 20, false},
    {"udp6", "ns3::LinuxUdp6SocketFactory", 20, false},
    {"tcp6", "ns3::LinuxTcp6SocketFactory", 20, false},
    {"dccp6", "ns3::LinuxDccp6SocketFactory", 20, false},
    {"sctp6", "ns3::LinuxSctp6SocketFactory", 20, false},
  };

  // for the moment: not supported mptcp for freebsd
  std::string filePath = SearchExecFile ("DCE_PATH", "liblinux.so", 0);
  for (unsigned int i = 0; i < sizeof(tests)/sizeof(testPair); i++)
    {
      if (filePath.length () <= 0)
        {
          tests[i].isSkip = true;
        }

      AddTestCase (new DceCradleTestCase (tests[i].name,
                                          Seconds (tests[i].duration),
                                          tests[i].sockf,
                                          tests[i].isSkip
                                          ), 
                   TestCase::QUICK);
    }
}

} // namespace ns3
