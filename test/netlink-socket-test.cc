/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 Liu Jian
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
 * Author: Liu Jian <liujatp@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/node-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-dce-routing-helper.h"
#include "ns3/socket-factory.h"
#include "ns3/string.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "netlink-message.h"
#include "netlink-socket-address.h"
#include <sys/socket.h>
#include <string>
#include <list>


NS_LOG_COMPONENT_DEFINE ("NetlinkSocketTest");

namespace ns3 {


class NetlinkSocketTestCase : public TestCase
{
public:
  NetlinkSocketTestCase ();
  virtual void DoRun (void);
private:
  NetlinkMessage BuildGetMessage (uint16_t type, uint16_t flags);
  NetlinkMessage BuildAddressMessage (uint16_t type, uint16_t flags);
  NetlinkMessage BuildLinkChangeMessage (uint16_t type, uint16_t flags);
  NetlinkMessage BuildRouteMessage (uint16_t type, uint16_t flags);
  MultipartNetlinkMessage BuildMultipartMessage (uint16_t type, uint16_t flags);

  bool CheckIsAck (NetlinkMessage nlmsg);
  bool CheckIsDump (MultipartNetlinkMessage nlmsg);
  bool CheckIsEqual (NetlinkMessageHeader nhr1, NetlinkMessageHeader nhr2);
  bool CheckIsEqual (NetlinkMessage nlmsg1, NetlinkMessage nlmsg2);
  bool CheckIsEqual (MultipartNetlinkMessage mulmsg1, MultipartNetlinkMessage mulmsg2);

  void TestNetlinkSerialization ();
  void TestInterfaceAddressMessage ();
  void TestInferfaceInfoMessage ();
  void TestRouteMessage ();
  void TestBroadcastMessage ();

  void ReceiveUnicastPacket (Ptr<Socket> socket);
  void ReceiveMulticastPacket (Ptr<Socket> socket);
  void SendCmdToKernel (uint16_t type);
  void SendNetlinkMessage (NetlinkMessage nlmsg);
  void MonitorKernelChanges ();
  Ptr<SocketFactory> CreateNetlinkFactory (void);

  std::list<MultipartNetlinkMessage> m_unicastList;
  std::list<MultipartNetlinkMessage> m_multicastList;
  Ptr<Socket> m_cmdSock;
  Ptr<Socket> m_groupSock;
  int m_pid;
};



NetlinkMessage
NetlinkSocketTestCase::BuildGetMessage (uint16_t type, uint16_t flags)
{
  NS_ASSERT (type == NETLINK_RTM_GETLINK || type == NETLINK_RTM_GETADDR || type == NETLINK_RTM_GETROUTE);

  NetlinkMessage nlmsg;
  flags |= (NETLINK_MSG_F_DUMP | NETLINK_MSG_F_ACK | NETLINK_MSG_F_REQUEST);
  nlmsg.SetHeader (NetlinkMessageHeader (type, flags, 0, 0));
  GeneralMessage genmsg;
  genmsg.SetFamily (AF_INET);
  nlmsg.SetGeneralMessage (genmsg);
  //no attributes appended
  return nlmsg;
}

NetlinkMessage
NetlinkSocketTestCase::BuildAddressMessage (uint16_t type, uint16_t flags)
{
  NS_ASSERT (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR);

  flags |= (NETLINK_MSG_F_ACK | NETLINK_MSG_F_REQUEST);
  if (type == NETLINK_RTM_NEWADDR)
    {
      flags |= NETLINK_MSG_F_CREATE;
    }

  NetlinkMessage nlmsg;
  //set header
  nlmsg.SetHeader (NetlinkMessageHeader (type, flags, 0, 0));

  //set service module
  InterfaceAddressMessage ifamsg;
  ifamsg.SetFamily (AF_INET);
  ifamsg.SetLength (24);
  ifamsg.SetInterfaceIndex (3);

  ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LOCAL, ADDRESS, Ipv4Address ("192.168.0.1")));
  ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_ADDRESS, ADDRESS, Ipv4Address ("192.168.0.2")));
  std::string value = "TESTSTRING";
  ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LABEL, STRING, value));

  nlmsg.SetInterfaceAddressMessage (ifamsg);
  return nlmsg;
}

NetlinkMessage
NetlinkSocketTestCase::BuildLinkChangeMessage (uint16_t type, uint16_t flags)
{
  NetlinkMessage nlmsg;
  NS_LOG_WARN (this << type << flags << " not supported");
  return nlmsg;
}

NetlinkMessage
NetlinkSocketTestCase::BuildRouteMessage (uint16_t type, uint16_t flags)
{
  NS_ASSERT (type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE);

  flags |= (NETLINK_MSG_F_ACK | NETLINK_MSG_F_REQUEST);
  if (type == NETLINK_RTM_NEWROUTE)
    {
      flags |= NETLINK_MSG_F_CREATE;
    }

  NetlinkMessage nlmsg;
  //set header
  nlmsg.SetHeader (NetlinkMessageHeader (type, flags, 0, m_pid));

  //set service module
  RouteMessage rtmsg;
  //set attribute
  rtmsg.SetFamily (AF_INET);
  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_DST, ADDRESS, Ipv4Address ("192.168.0.10")));
  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_SRC, ADDRESS, Ipv4Address ("192.168.2.10")));
  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_GATEWAY, ADDRESS, Ipv4Address ("10.1.1.10")));
  uint32_t value = 2;
  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_OIF, U32, value));

  nlmsg.SetRouteMessage (rtmsg);
  return nlmsg;
}

MultipartNetlinkMessage
NetlinkSocketTestCase::BuildMultipartMessage (uint16_t type, uint16_t flags)
{
  //usually multi-part message used for message dump, kernel return to user space for NETLINK_RTM_GETxxx
  //type = NETLINK_RTM_NEWxxx, flags = NETLINK_MSG_F_MULTI, terminated by NETLINK_MSG_DONE,
  //here this example contain 2 NETLINK_MSG_F_MULTI
  NS_ASSERT (flags & NETLINK_MSG_F_MULTI);

  MultipartNetlinkMessage nlmsg;
  NetlinkMessage nlmsg1, nlmsg2, nlmsg3;
  nlmsg1 = BuildAddressMessage (NETLINK_RTM_NEWADDR, NETLINK_MSG_F_MULTI);
  nlmsg.AppendMessage (nlmsg1);
  //the first nlmsg
  nlmsg2 = BuildRouteMessage (NETLINK_RTM_NEWROUTE, NETLINK_MSG_F_MULTI);
  nlmsg.AppendMessage (nlmsg2);
  //the second nlmsg
  nlmsg3.SetHeader (NetlinkMessageHeader (NETLINK_MSG_DONE, flags, 1, m_pid));
  nlmsg.AppendMessage (nlmsg3);

  return nlmsg;
}




bool
NetlinkSocketTestCase::CheckIsAck (NetlinkMessage nlmsg)
{
  return (nlmsg.GetMsgType () == NETLINK_MSG_ERROR && nlmsg.GetErrorMessage ().GetError () == 0);
}

bool
NetlinkSocketTestCase::CheckIsDump (MultipartNetlinkMessage mulmsg)
{
  return (mulmsg.GetNMessages () > 0 && mulmsg.GetMessage (0).GetHeader ().GetMsgFlags () & NETLINK_MSG_F_MULTI
          && mulmsg.GetMessage (mulmsg.GetNMessages () - 1).GetMsgType () == NETLINK_MSG_DONE);
}

bool
NetlinkSocketTestCase::CheckIsEqual (NetlinkMessageHeader nhr1, NetlinkMessageHeader nhr2)
{
  return (nhr1.GetMsgType () == nhr2.GetMsgType () && nhr1.GetMsgFlags () == nhr2.GetMsgFlags ()
          && nhr1.GetMsgLen () == nhr2.GetMsgLen ());
}

bool
NetlinkSocketTestCase::CheckIsEqual (NetlinkMessage nlmsg1, NetlinkMessage nlmsg2)
{
  return CheckIsEqual (nlmsg1.GetHeader (), nlmsg2.GetHeader ());
}

bool
NetlinkSocketTestCase::CheckIsEqual (MultipartNetlinkMessage mulmsg1, MultipartNetlinkMessage mulmsg2)
{
  if (mulmsg1.GetNMessages () != mulmsg2.GetNMessages ())
    {
      return false;
    }

  for (uint32_t i = 0; i < mulmsg1.GetNMessages (); i++)
    {
      if (!CheckIsEqual (mulmsg1.GetMessage (i), mulmsg2.GetMessage (i)))
        {
          return false;
        }
    }
  return true;
}

void
NetlinkSocketTestCase::TestNetlinkSerialization ()
{
  MultipartNetlinkMessage multinlmsg1, multinlmsg2;
  Ptr<Packet> p = Create<Packet> ();

  multinlmsg1 = BuildMultipartMessage (NETLINK_RTM_NEWADDR, NETLINK_MSG_F_REQUEST | NETLINK_MSG_F_MULTI);
  p->AddHeader (multinlmsg1);
  p->RemoveHeader (multinlmsg2);
  NS_TEST_ASSERT_MSG_EQ (CheckIsEqual (multinlmsg1, multinlmsg2), true, "Should be equal");
}
void
NetlinkSocketTestCase::TestInterfaceAddressMessage ()
{
  MultipartNetlinkMessage dump1, dump2, dump3;
  NetlinkMessage nlmsg1,nlmsg2;

  //dump interface address
  SendNetlinkMessage (BuildGetMessage (NETLINK_RTM_GETADDR, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue should be 1 (RTM_GETADDR)");
  dump1 = m_unicastList.front ();
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsDump (dump1), true, "Should be dump msg");

  //add interface address
  // Not implemented yet (100325)
#if 0
  SendNetlinkMessage (BuildAddressMessage (NETLINK_RTM_NEWADDR,0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue should be 1 (RTM_NEWADDR)");
  nlmsg1 = m_unicastList.front ().GetMessage (0);
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsAck (nlmsg1), true, "msg should be Ack");
#endif

  //dump interface address
  SendNetlinkMessage (BuildGetMessage (NETLINK_RTM_GETADDR, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue should be 1 (RTM_GETADDR)");
  dump2 = m_unicastList.front ();
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsDump (dump2), true, "msg should be dump");

  //del interface address
  // Not implemented yet (100325)
#if 0
  SendNetlinkMessage (BuildAddressMessage (NETLINK_RTM_DELADDR,0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_DELADDR)");
  nlmsg2 = m_unicastList.front ().GetMessage (0);
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsAck (nlmsg2), true, "msg should be Ack");
#endif

  //dump interface address
  SendNetlinkMessage (BuildGetMessage (NETLINK_RTM_GETADDR, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_GETADDR)");
  dump3 = m_unicastList.front ();
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsDump (dump3), true, "msg should be dump");

  NS_TEST_ASSERT_MSG_EQ (CheckIsEqual (dump1, dump3), true, "Dump msg should be same");
}


void
NetlinkSocketTestCase::TestRouteMessage ()
{
  MultipartNetlinkMessage dump1, dump2, dump3;
  NetlinkMessage nlmsg1,nlmsg2;

  //dump route entry
  SendNetlinkMessage (BuildGetMessage (NETLINK_RTM_GETROUTE, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_GETROUTE)");
  dump1 = m_unicastList.front ();
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsDump (dump1), true, "msg should be dump");

  //add route entry
  SendNetlinkMessage (BuildRouteMessage (NETLINK_RTM_NEWROUTE,0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_NEWROUTE)");
  nlmsg1 = m_unicastList.front ().GetMessage (0);
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsAck (nlmsg1), true, "msg should be Ack");

  //dump route entry
  SendNetlinkMessage (BuildGetMessage (NETLINK_RTM_GETROUTE, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_GETROUTE)");
  dump2 = m_unicastList.front ();
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsDump (dump2), true, "msg should be dump");

  //del route entry
  SendNetlinkMessage (BuildRouteMessage (NETLINK_RTM_DELROUTE, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_DELROUTE)");
  nlmsg2 = m_unicastList.front ().GetMessage (0);
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsAck (nlmsg2), true, "msg should be Ack");

  //dump route entry
  SendNetlinkMessage (BuildGetMessage (NETLINK_RTM_GETROUTE, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_GETROUTE)");
  dump3 = m_unicastList.front ();
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsDump (dump3), true, "msg should be dump");

  NS_TEST_ASSERT_MSG_EQ (CheckIsEqual (dump1, dump3), true, "msg should be same");
}

void
NetlinkSocketTestCase::TestInferfaceInfoMessage ()
{
  //now netlink not support NEWLINK/DELLINK yet
  MultipartNetlinkMessage multinlmsg;

  //dump interface address
  SendNetlinkMessage (BuildGetMessage (NETLINK_RTM_GETLINK, 0));
  NS_TEST_ASSERT_MSG_EQ (m_unicastList.size (), 1, "queue size should be 1 (RTM_GETLINK)");
  multinlmsg = m_unicastList.front ();
  m_unicastList.pop_front ();
  NS_TEST_ASSERT_MSG_EQ (CheckIsDump (multinlmsg), true, "msg should be dump");

  NS_TEST_ASSERT_MSG_EQ ((multinlmsg.GetNMessages () > 1) && (multinlmsg.GetMessage (0).GetMsgType () == NETLINK_RTM_NEWLINK),
                         true, "msg might be incorrect");
}

void
NetlinkSocketTestCase::TestBroadcastMessage ()
{
  //at 2Xs, m_cmdSock send an request to kernel to add/del an interface address
  //and an route entry,  the m_groupSock will recv the changed information
  //through the broadcast way
  m_multicastList.clear ();
  Simulator::Schedule (Seconds (1), &NetlinkSocketTestCase::MonitorKernelChanges, this);
  // Not implemented yet (100325)
#if 0
  Simulator::Schedule (Seconds (2), &NetlinkSocketTestCase::SendCmdToKernel, this, NETLINK_RTM_NEWADDR);
#endif
  Simulator::Schedule (Seconds (4), &NetlinkSocketTestCase::SendCmdToKernel, this, NETLINK_RTM_NEWROUTE);
  // Not implemented yet (100325)
#if 0
  Simulator::Schedule (Seconds (6), &NetlinkSocketTestCase::SendCmdToKernel, this, NETLINK_RTM_DELADDR);
#endif
  Simulator::Schedule (Seconds (8), &NetlinkSocketTestCase::SendCmdToKernel, this, NETLINK_RTM_DELROUTE);
}


void
NetlinkSocketTestCase::SendNetlinkMessage (NetlinkMessage nlmsg)
{
#if 1
  Ptr<Packet> p = Create<Packet> ();
  p->AddHeader (MultipartNetlinkMessage (nlmsg));
#else
  char buf[20] = {
    0x14,0x00,0x00,0x00,0x12,0x00,0x05,0x03,
    0x34,0xb2,0xf5,0x47,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00
  };
  uint32_t count = 20;
  Ptr<Packet> p = ns3::Create<Packet> ((const uint8_t *)buf, (uint32_t)count);
#endif

  m_cmdSock->Send (p);
}
void
NetlinkSocketTestCase::SendCmdToKernel (uint16_t type)
{
  NS_LOG_INFO ("At = " <<  Simulator::Now ().GetSeconds () << "s, user send cmd to kernel, cmd = " << type);

  if (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR)
    {
      SendNetlinkMessage (BuildAddressMessage (type, 0));
    }
  else if (type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE)
    {
      SendNetlinkMessage (BuildRouteMessage (type, 0));
    }
  else
    {
      NS_LOG_ERROR ("netlink cmd not support , type = " << type);
    }
}


void
NetlinkSocketTestCase::ReceiveUnicastPacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  while (packet = socket->Recv ())
    {
      MultipartNetlinkMessage nlmsg;
      packet->RemoveHeader (nlmsg);
      m_unicastList.push_back (nlmsg);
    }
}

void
NetlinkSocketTestCase::ReceiveMulticastPacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  while (packet = socket->Recv ())
    {
      MultipartNetlinkMessage nlmsg;
      packet->RemoveHeader (nlmsg);
      m_multicastList.push_back (nlmsg);
    }
}
void
NetlinkSocketTestCase::MonitorKernelChanges ()
{
  NS_LOG_INFO ("At = " << Simulator::Now ().GetSeconds () << "s, group socket check the recv list");

  if (m_multicastList.size ())
    {
      MultipartNetlinkMessage multinlmsg = m_multicastList.front ();

      if (multinlmsg.GetNMessages () == 1)
        {
          NetlinkMessage nlmsg = multinlmsg.GetMessage (0);
          uint16_t type;
          type = nlmsg.GetMsgType ();
          NS_ASSERT (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR
                     || type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE);
          NS_LOG_INFO ("group socket recv netlink message, type =" << type);
        }
      else
        {
          NS_LOG_WARN ("group socket recv an unwanted message");
        }
      m_multicastList.pop_front ();
    }

  //restart this timer
  if (Simulator::Now ().GetSeconds () < 10)
    {
      Simulator::Schedule (Seconds (1), &NetlinkSocketTestCase::MonitorKernelChanges, this);
    }
}



NetlinkSocketTestCase::NetlinkSocketTestCase ()
  : TestCase ("Netlink"),
    m_pid (1)
{
}

Ptr<SocketFactory>
NetlinkSocketTestCase::CreateNetlinkFactory (void)
{
  ObjectFactory factory;
  factory.SetTypeId ("ns3::NetlinkSocketFactory");
  return factory.Create<SocketFactory> ();
}

void
NetlinkSocketTestCase::DoRun (void)
{
  //init nodes with stacks and device
  // Network topology
  //
  //   n0
  //     \ 5 Mb/s, 2ms
  //      \          1.5Mb/s, 10ms
  //       n1 -------------------------n2


  NodeContainer nodes;
  nodes.Create (3);
  NodeContainer n0n1 = NodeContainer (nodes.Get (0), nodes.Get (1));
  NodeContainer n1n2 = NodeContainer (nodes.Get (1), nodes.Get (2));

  InternetStackHelper stack;
  Ipv4DceRoutingHelper ipv4RoutingHelper;
  stack.SetRoutingHelper (ipv4RoutingHelper);
  stack.Install (nodes);

  //add a p2p device with an ip address
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer d0d1 = p2p.Install (n0n1);
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1.5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer d1d2 = p2p.Install (n1n2);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i01 = ipv4.Assign (d0d1);
  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i12 = ipv4.Assign (d1d2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /*create two netlink sockets in node1
  one is to exchange information between userspace and kernel ,
  one is to monitor the changes happened in kernel
  */
  Ptr<Node> node0 = nodes.Get (1);
  Ptr<SocketFactory> socketFactory = CreateNetlinkFactory ();
  node0->AggregateObject (socketFactory);
  NetlinkSocketAddress addr;

  /*creat an cmd netlink socket, it send cmd to kernel space*/
  m_cmdSock = socketFactory->CreateSocket ();
  m_cmdSock->SetRecvCallback (MakeCallback (&NetlinkSocketTestCase::ReceiveUnicastPacket, this));
  addr.SetProcessID (m_pid);
  addr.SetGroupsMask (0);
  m_cmdSock->Bind (addr);

  /*creat an group netlink socket, it monitor the kernel's changes*/
  m_groupSock = socketFactory->CreateSocket ();
  m_groupSock->SetRecvCallback (MakeCallback (&NetlinkSocketTestCase::ReceiveMulticastPacket, this));
  addr.SetProcessID (m_pid + 1);
  addr.SetGroupsMask (NETLINK_RTM_GRP_IPV4_IFADDR | NETLINK_RTM_GRP_IPV4_ROUTE);
  m_groupSock->Bind (addr);

  /*test 1: for Serialize and Deserialize*/
  TestNetlinkSerialization ();

  /*test 2: for interface address dump/add/get message*/
  TestInterfaceAddressMessage ();

  /*test 3: for interface info dump message*/
  TestInferfaceInfoMessage ();

  /*test 4: for route dump/add/get message*/
  TestRouteMessage ();

  /*test 5: for netlink broadcast */
  TestBroadcastMessage ();

  Simulator::Run ();
}

static class NetlinkSocketTestSuite : public TestSuite
{
public:
  NetlinkSocketTestSuite ();
private:
} g_netlinkTestSuite;

NetlinkSocketTestSuite::NetlinkSocketTestSuite ()
  : TestSuite ("netlink-socket", UNIT)
{
  AddTestCase (new NetlinkSocketTestCase (), TestCase::QUICK);
}

} // namespace ns3


