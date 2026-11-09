/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simple-net-device.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-dce-routing.h"
#include "ns3/ipv4-dce-routing-helper.h"

using namespace ns3;

namespace {

int g_local = 0;
int g_forwarded = 0;

void
Unicast (Ptr<Ipv4Route> route, Ptr<const Packet> p, const Ipv4Header &header)
{
}

void
Multicast (Ptr<Ipv4MulticastRoute> route, Ptr<const Packet> p, const Ipv4Header &header)
{
  g_forwarded++;
}

void
Local (Ptr<const Packet> p, const Ipv4Header &header, uint32_t iif)
{
  g_local++;
}

void
Error (Ptr<const Packet> p, const Ipv4Header &header, Socket::SocketErrno error)
{
}

} // namespace

/**
 * Multicast packets are delivered locally (to raw sockets, e.g. of routing
 * daemons), with or without a multicast route to forward them.
 */
class Ipv4DceRoutingMulticastTestCase : public TestCase
{
public:
  Ipv4DceRoutingMulticastTestCase ()
    : TestCase ("multicast local delivery")
  {
  }

private:
  virtual void
  DoRun (void)
  {
    Ptr<Node> node = CreateObject<Node> ();
    Ptr<SimpleNetDevice> in = CreateObject<SimpleNetDevice> ();
    Ptr<SimpleNetDevice> out = CreateObject<SimpleNetDevice> ();
    in->SetAddress (Mac48Address::Allocate ());
    out->SetAddress (Mac48Address::Allocate ());
    node->AddDevice (in);
    node->AddDevice (out);
    InternetStackHelper stack;
    Ipv4DceRoutingHelper routingHelper;
    stack.SetRoutingHelper (routingHelper);
    stack.SetIpv6StackInstall (false);
    stack.Install (node);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    uint32_t inIf = ipv4->AddInterface (in);
    uint32_t outIf = ipv4->AddInterface (out);
    ipv4->AddAddress (inIf, Ipv4InterfaceAddress ("10.0.0.1", "255.255.255.0"));
    ipv4->AddAddress (outIf, Ipv4InterfaceAddress ("10.0.1.1", "255.255.255.0"));
    ipv4->SetUp (inIf);
    ipv4->SetUp (outIf);
    Ptr<Ipv4DceRouting> routing = DynamicCast<Ipv4DceRouting> (ipv4->GetRoutingProtocol ());
    NS_TEST_ASSERT_MSG_EQ (bool (routing), true, "Ipv4DceRouting");

    Ipv4Header header;
    header.SetSource ("10.0.0.2");
    header.SetDestination ("224.0.0.5");
    Ptr<Packet> p = Create<Packet> (10);

    g_local = g_forwarded = 0;
    routing->RouteInput (p, header, in, MakeCallback (&Unicast), MakeCallback (&Multicast),
                         MakeCallback (&Local), MakeCallback (&Error));
    NS_TEST_EXPECT_MSG_EQ (g_local, 1, "delivered without a multicast route");
    NS_TEST_EXPECT_MSG_EQ (g_forwarded, 0, "nothing to forward without a multicast route");

    std::vector<uint32_t> outputs (1, outIf);
    routing->AddMulticastRoute ("10.0.0.2", "224.0.0.5", inIf, outputs);
    g_local = g_forwarded = 0;
    routing->RouteInput (p, header, in, MakeCallback (&Unicast), MakeCallback (&Multicast),
                         MakeCallback (&Local), MakeCallback (&Error));
    NS_TEST_EXPECT_MSG_EQ (g_local, 1, "delivered with a multicast route");
    NS_TEST_EXPECT_MSG_EQ (g_forwarded, 1, "forwarded with a multicast route");
  }
};

static class Ipv4DceRoutingTestSuite : public TestSuite
{
public:
  Ipv4DceRoutingTestSuite ()
    : TestSuite ("dce-ipv4-routing", Type::UNIT)
  {
    AddTestCase (new Ipv4DceRoutingMulticastTestCase (), TestCase::Duration::QUICK);
  }
} g_ipv4DceRoutingTestSuite;
