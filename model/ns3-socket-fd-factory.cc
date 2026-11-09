/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3-socket-fd-factory.h"
#include "unix-fd.h"
#include "unix-socket-fd.h"
#include "unix-datagram-socket-fd.h"
#include "unix-stream-socket-fd.h"
#include "ns3/netlink-socket-factory.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/uinteger.h"
#include "ns3/packet-socket-address.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include <netpacket/packet.h>
#include <net/ethernet.h>

NS_LOG_COMPONENT_DEFINE ("DceNs3SocketFdFactory");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Ns3SocketFdFactory);

/**
 * ns3::Ipv6L3Protocol only hands packets to raw sockets when an L4 protocol
 * is registered for their next header; otherwise it drops them and answers
 * with an ICMPv6 parameter problem. This protocol accepts packets for a
 * protocol number that only raw sockets handle (e.g. OSPFv3).
 */
class DceIpv6RawOnlyProtocol : public IpL4Protocol
{
public:
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::DceIpv6RawOnlyProtocol")
      .SetParent<IpL4Protocol> ()
      .SetGroupName ("Dce");
    return tid;
  }
  DceIpv6RawOnlyProtocol (int protocol = 0) : m_protocol (protocol) {}
  int GetProtocolNumber (void) const override { return m_protocol; }
  RxStatus Receive (Ptr<Packet> p, const Ipv4Header &header,
                    Ptr<Ipv4Interface> incomingInterface) override { return RX_OK; }
  RxStatus Receive (Ptr<Packet> p, const Ipv6Header &header,
                    Ptr<Ipv6Interface> incomingInterface) override { return RX_OK; }
  void SetDownTarget (DownTargetCallback cb) override { m_downTarget = cb; }
  void SetDownTarget6 (DownTargetCallback6 cb) override { m_downTarget6 = cb; }
  DownTargetCallback GetDownTarget (void) const override { return m_downTarget; }
  DownTargetCallback6 GetDownTarget6 (void) const override { return m_downTarget6; }
private:
  int m_protocol;
  DownTargetCallback m_downTarget;
  DownTargetCallback6 m_downTarget6;
};

TypeId
Ns3SocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ns3SocketFdFactory")
    .SetParent<SocketFdFactory> ()
    .AddConstructor<Ns3SocketFdFactory> ()
  ;
  return tid;
}

Ns3SocketFdFactory::Ns3SocketFdFactory ()
  : m_netlink (0)
{
}

//this is necessary to assign appropriate Node to NetlinkSocketFactory
void
Ns3SocketFdFactory::NotifyNewAggregate (void)
{
  Ptr<Node> node = this->GetObject<Node> ();
  if (!m_netlink)
    {
      m_netlink = CreateObject<NetlinkSocketFactory> ();
      node->AggregateObject (m_netlink);
    }
}

UnixFd *
Ns3SocketFdFactory::CreateSocket (int domain, int type, int protocol)
{
  UnixSocketFd *socket = 0;
  Ptr<Socket> sock;

  if (domain == PF_INET)
    {
      switch (type)
        {
        case SOCK_DGRAM:
          {
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
            Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
            NS_ASSERT_MSG (factory, "InternetStackHelper is not installed. "
                           "Install it before using Ns3SocketFdFactory.");
            sock = factory->CreateSocket ();
            socket = new UnixDatagramSocketFd (sock);
          } break;
        case SOCK_RAW:
          {
            TypeId tid = TypeId::LookupByName ("ns3::Ipv4RawSocketFactory");
            Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
            NS_ASSERT_MSG (factory, "InternetStackHelper is not installed. "
                           "Install it before using Ns3SocketFdFactory.");
            sock = factory->CreateSocket ();
            sock->SetAttribute ("Protocol", UintegerValue (protocol));
            socket = new UnixDatagramSocketFd (sock);
          } break;
        case SOCK_STREAM:
          {
            TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
            Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
            NS_ASSERT_MSG (factory, "InternetStackHelper is not installed. "
                           "Install it before using Ns3SocketFdFactory.");
            sock = factory->CreateSocket ();
            socket = new UnixStreamSocketFd (sock);
          } break;
        default:
          NS_FATAL_ERROR ("missing socket type");
          break;
        }
    }
  else if (domain == PF_INET6)
    {
      switch (type)
        {
        case SOCK_RAW:
          {
            TypeId tid = TypeId::LookupByName ("ns3::Ipv6RawSocketFactory");
            Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
            NS_ASSERT_MSG (factory, "InternetStackHelper (v6) is not installed. "
                           "Install it before using Ns3SocketFdFactory.");
            sock = factory->CreateSocket ();
            sock->SetAttribute ("Protocol", UintegerValue (protocol));
            socket = new UnixDatagramSocketFd (sock);
            Ptr<Ipv6L3Protocol> ipv6 = GetObject<Ipv6L3Protocol> ();
            if (ipv6 && protocol > 0 && !ipv6->GetProtocol (protocol))
              {
                ipv6->Insert (CreateObject<DceIpv6RawOnlyProtocol> (protocol));
              }
          } break;
        case SOCK_DGRAM:
          {
            // ns-3 UDP sockets are dual-stack: binding an IPv6 address makes
            // them IPv6 sockets.
            TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
            Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
            NS_ASSERT_MSG (factory, "InternetStackHelper (v6) is not installed. "
                           "Install it before using Ns3SocketFdFactory.");
            sock = factory->CreateSocket ();
            socket = new UnixDatagramSocketFd (sock);
          } break;
        case SOCK_STREAM:
          {
            TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
            Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
            NS_ASSERT_MSG (factory, "InternetStackHelper (v6) is not installed. "
                           "Install it before using Ns3SocketFdFactory.");
            sock = factory->CreateSocket ();
            socket = new UnixStreamSocketFd (sock);
          } break;
        default:
          NS_FATAL_ERROR ("missing socket type");
          break;
        }
    }
  else if (domain == PF_NETLINK)
    {
      switch (type)
        {
        case SOCK_RAW:
        case SOCK_DGRAM:
          {
            NS_LOG_INFO ("Requesting for PF_NETLINK");
            sock = m_netlink->CreateSocket ();
            socket = new UnixDatagramSocketFd (sock);
          } break;
        default:
          NS_FATAL_ERROR ("missing socket type");
          break;
        }
    }
  else if (domain == AF_PACKET)
    {
      switch (type)
        {
        case SOCK_RAW:
          {
            TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
            Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
            NS_ASSERT_MSG (factory, "InternetStackHelper (packet) is not installed. "
                           "Install it before using Ns3SocketFdFactory.");
            sock = factory->CreateSocket ();

            PacketSocketAddress a;
            a.SetAllDevices ();
            if (protocol == htons (ETH_P_ALL))
              {
                a.SetProtocol (0);
              }
            else
              {
                a.SetProtocol (ntohs (protocol));
              }

            sock->Bind (a);

            socket = new UnixDatagramSocketFd (sock);
          } break;
        default:
          NS_FATAL_ERROR ("missing socket type");
          break;
        }
    }
  else
    {
      NS_FATAL_ERROR ("unsupported domain");
      return 0;
    }

  return socket;
}

} // namespace ns3
