/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Hajime Tazaki
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/dce-module.h"
#include "ns3/csma-helper.h"
#include "ns3/ethernet-header.h"
#include "ns3/icmpv4.h"
#include "ns3/icmpv6-header.h"
#include "ns3/udp-header.h"

#define OUTPUT(x)                                                       \
  {                                                                     \
    std::ostringstream oss;                                             \
    oss << "file=" << __FILE__ << " line=" << __LINE__ << " "           \
        << x << std::endl;                                              \
    std::string s = oss.str ();                                         \
    std::cerr << s.c_str ();                                            \
  }


static std::string g_testError;

extern "C" void dce_linux_ip6_test_store_test_error (const char *s)
{
  g_testError = s;
}

using namespace ns3;
namespace ns3 {

class DceLinuxIp6TestCase : public TestCase
{
public:
  DceLinuxIp6TestCase (std::string testname, Time maxDuration, bool skip);
  void CsmaRxCallback (std::string context, Ptr<const Packet> packet);
private:
  static void Finished (int *pstatus, uint16_t pid, int status);
  virtual void DoRun (void);

  std::string m_testname;
  Time m_maxDuration;
  bool m_pingStatus;
  bool m_debug;
  bool m_skip;
};

void
DceLinuxIp6TestCase::CsmaRxCallback (std::string context, Ptr<const Packet> originalPacket)
{
  if (m_pingStatus)
    {
      return;
    }
  uint16_t protocol;
  Ptr<Packet> packet = originalPacket->Copy ();
  EthernetHeader header (false);
  packet->RemoveHeader (header);
  protocol = header.GetLengthType ();
  Ipv4Header v4hdr;
  Icmpv4Header icmphdr;
  Ipv6Header v6hdr;
  UdpHeader udphdr;
  Icmpv6Header icmp6hdr;
  bool found = false;

  switch (protocol)
    {
    case 0x0800:   //IPv4
      packet->RemoveHeader (v4hdr);
      packet->RemoveHeader (icmphdr);
      if (icmphdr.GetType () == Icmpv4Header::ECHO_REPLY)
        {
          m_pingStatus = true;
        }
      break;
    case 0x86DD:   //IPv6
      packet->RemoveHeader (v6hdr);

      while (true)
        {
          if (v6hdr.GetNextHeader () == Ipv6Header::IPV6_EXT_END)
            {
              break;
            }
          else if (v6hdr.GetNextHeader () == Ipv6Header::IPV6_IPV6)
            {
              packet->RemoveHeader (v6hdr);
              continue;
            }
          else if (v6hdr.GetNextHeader () == Ipv6Header::IPV6_UDP)
            {
              packet->RemoveHeader (udphdr);
              continue;
            }
          else if (v6hdr.GetNextHeader () == Ipv6Header::IPV6_ICMPV6)
            {
              packet->RemoveHeader (icmp6hdr);
              found = true;
              break;
            }
          break;
        }

      if (found && icmp6hdr.GetType () == Icmpv6Header::ICMPV6_ECHO_REPLY)
        {
          m_pingStatus = true;
        }
      break;
    default:
      break;
    }

  //  std::cout << context << " " << packet << " protocol " << protocol << std::endl;
}

DceLinuxIp6TestCase::DceLinuxIp6TestCase (std::string testname, Time maxDuration, bool skip)
  : TestCase (skip ? "(SKIP) " + testname : testname),
    m_testname (testname),
    m_maxDuration (maxDuration),
    m_pingStatus (false),
    m_debug (false),
    m_skip (skip)
{
}

void
DceLinuxIp6TestCase::Finished (int *pstatus, uint16_t pid, int status)
{
  *pstatus = status;
}

void
DceLinuxIp6TestCase::DoRun (void)
{
  if (m_skip)
    {
      // XXX: crash with optimized build if there is no Simulator::Run()
      Simulator::Run ();
      Simulator::Destroy ();
      return;
    }

  //
  //  Step 1
  //  Node Basic Configuration
  //
  NodeContainer nodes;
  nodes.Create (2);

  CsmaHelper csma;

  NetDeviceContainer devices, dev1, dev2;
  devices = csma.Install (nodes);
  dev1 = csma.Install (nodes.Get (0));
  dev2 = csma.Install (nodes.Get (1));
  DceManagerHelper processManager;

  //
  // Step 2
  // Address Configuration
  //
  //
  processManager.SetNetworkStack ("ns3::LinuxSocketFdFactory",
                                  "Library", StringValue ("libsim-linux.so"));
  processManager.Install (nodes);

  // IP address configuration
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "-f inet6 addr add 2001:db8:0:1::1/64 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.1), "-f inet6 addr add 2001:db8:0:2::1/64 dev sim1");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.11), "-f inet addr add 10.0.0.1/24 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.11), "-f inet addr add 11.0.0.1/24 dev sim1");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.11), "link set sim1 up");

  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), "-f inet6 addr add 2001:db8:0:1::2/64 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.1), "-f inet6 addr add 2001:db8:0:3::2/64 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.11), "-f inet addr add 10.0.0.2/24 dev sim0");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.11), "-f inet addr add 12.0.0.1/24 dev sim1");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.11), "link set lo up");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.11), "link set sim0 up");
  LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.11), "link set sim1 up");

  if (m_testname == "ip6ip6")
    {
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.5),
                               "-6 tunnel add tun1 remote 2001:db8:0:1::2 local 2001:db8:0:1::1 dev sim0");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.5), "-f inet6 addr add 2001:db8:0:5::1/64 dev tun1");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.5), "link set tun1 up");

      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.5),
                               "-6 tunnel add tun1 remote 2001:db8:0:1::1 local 2001:db8:0:1::2 dev sim0");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.5), "-f inet6 addr add 2001:db8:0:5::2/64 dev tun1");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (0.5), "link set tun1 up");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (10.2), "link show");
    }
  else if (m_testname == "l2tp")
    {
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5),
                               "l2tp add tunnel tunnel_id 10 peer_tunnel_id 20 encap udp "
                               "remote 2001:db8:0:1::2 local 2001:db8:0:1::1 udp_sport 1701 udp_dport 1701");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5),
                               "l2tp add session tunnel_id 10 session_id 3000 peer_session_id 4000");

      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "-f inet6 addr add 2001:db8:0:5::1/64 dev l2tpeth0");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "link set l2tpeth0 up");

      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5),
                               "l2tp add tunnel remote 2001:db8:0:1::1 local 2001:db8:0:1::2 tunnel_id 20 "
                               "peer_tunnel_id 10 encap udp udp_sport 1701 udp_dport 1701");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5),
                               "l2tp add session tunnel_id 20 session_id 4000 peer_session_id 3000");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "-f inet6 addr add 2001:db8:0:5::2/64 dev l2tpeth0");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "link set l2tpeth0 up");

      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (10.2), "l2tp show tunnel");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (10.2), "link show");
    }
  else if (m_testname == "fou")
    {
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5),
                               "fou add port 5555 ipproto 6");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "link add name tun1 type ipip remote 10.0.0.2 "
                               "local 10.0.0.1 ttl 225 encap fou encap-sport auto encap-dport 5555");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "-f inet addr add 1.1.1.1/24 dev tun1");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "link set tun1 up");

      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5),
                               "fou add port 5555 ipproto 6");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "link add name tun1 type ipip remote 10.0.0.1 "
                               "local 10.0.0.2 ttl 225 encap fou encap-sport auto encap-dport 5555");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "-f inet addr add 1.1.1.2/24 dev tun1");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "link set tun1 up");

      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (10.2), "addr show");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (10.2), "link show");
    }
  else if (m_testname == "ip6gre")
    {
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "link add name tun1 type ip6gre remote 2001:db8:0:1::2 "
                               "local 2001:db8:0:1::1 dev sim0");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "-f inet6 addr add 2001:db8:0:5::1/64 dev tun1");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (5.5), "link set tun1 up");

      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "link add name tun1 type ip6gre remote 2001:db8:0:1::1 "
                               "local 2001:db8:0:1::2 dev sim0");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "-f inet6 addr add 2001:db8:0:5::2/64 dev tun1");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (5.5), "link set tun1 up");
    }

  if (m_debug)
    {
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.2), "link show");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (60.3), "route show table all");
      LinuxStackHelper::RunIp (nodes.Get (1), Seconds (60.3), "route show table all");
      LinuxStackHelper::RunIp (nodes.Get (0), Seconds (0.4), "addr list");
    }


  csma.EnablePcapAll ("dce-linux-ip6-test-" + m_testname);

  //
  // Step 3
  // Set up ping application
  //
  DceApplicationHelper dce;
  ApplicationContainer apps;
  int status = -1;

  if (m_testname == "plain")
    {
      dce.SetBinary ("ping6");
      dce.SetStackSize (1 << 20);
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetFinishedCallback (MakeBoundCallback (&DceLinuxIp6TestCase::Finished, &status));
      dce.AddArgument ("2001:db8:0:1::1");
      apps = dce.Install (nodes.Get (1));
      apps.Start (Seconds (10.0));
    }
  else if (m_testname == "ip6ip6" ||
           m_testname == "l2tp" ||
           m_testname == "NIU-fou" ||
           m_testname == "ip6gre")
    {
      dce.SetBinary ("ping6");
      dce.SetStackSize (1 << 20);
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetFinishedCallback (MakeBoundCallback (&DceLinuxIp6TestCase::Finished, &status));
      dce.AddArgument ("2001:db8:0:5::1");
      apps = dce.Install (nodes.Get (1));
      apps.Start (Seconds (10.0));
    }
  else if (m_testname == "fou")
    {
      dce.SetBinary ("ping");
      dce.SetStackSize (1 << 20);
      dce.ResetArguments ();
      dce.ResetEnvironment ();
      dce.SetFinishedCallback (MakeBoundCallback (&DceLinuxIp6TestCase::Finished, &status));
      dce.AddArgument ("1.1.1.1");
      apps = dce.Install (nodes.Get (1));
      apps.Start (Seconds (10.0));
    }

#if 0
  Config::Connect ("/NodeList/1/DeviceList/0/$ns3::CsmaNetDevice/MacRx",
                   MakeCallback (&DceLinuxIp6TestCase::CsmaRxCallback, this));
#endif
  //
  // Step 4
  // Now It's ready to GO!
  //
  if (m_maxDuration.IsStrictlyPositive ())
    {
      Simulator::Stop (m_maxDuration);
    }
  Simulator::Run ();
  Simulator::Destroy ();


  //
  // Step 5
  // Vetify the test
  //
  NS_TEST_ASSERT_MSG_EQ (status, 0, "Linux IPv6 test did not return successfully: " << g_testError);
  if (m_debug)
    {
      OUTPUT ("Linux Ipv6 test " << m_testname
              << " stack done. status = " << m_pingStatus);

      ::system (("/bin/mv -f files-0 files-0-" + m_testname).c_str ());
      ::system (("/bin/mv -f files-1 files-1-" + m_testname).c_str ());
    }
}

static class DceLinuxIp6TestSuite : public TestSuite
{
public:
  DceLinuxIp6TestSuite ();
private:
} g_processTests;


DceLinuxIp6TestSuite::DceLinuxIp6TestSuite ()
  : TestSuite ("dce-linux-ip6", UNIT)
{
  typedef struct
  {
    const char *name;
    int duration;
    bool skip;
  } testPair;

  testPair tests[] = {
    {"plain", 120, false},
    {"ip6gre", 120, false},
    {"ip6ip6", 120, false},
    {"l2tp", 120, false},
    {"fou", 120, false},
  };
 
  TypeId tid;
  bool kern = TypeId::LookupByNameFailSafe ("ns3::LinuxSocketFdFactory", &tid);
  std::string filePath = SearchExecFile ("DCE_PATH", "libsim-linux.so", 0);

  for (unsigned int i = 0; i < sizeof(tests) / sizeof(testPair); i++)
    {
      if (!kern || filePath.length () <= 0)
        {
          tests[i].skip = true;
        }
      AddTestCase (new DceLinuxIp6TestCase (std::string (tests[i].name),
                                            Seconds (tests[i].duration),
                                            tests[i].skip),
                   TestCase::QUICK);
    }
}

} // namespace ns3
