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

 #undef NS_LOG_APPEND_CONTEXT

#define NS_LOG_APPEND_CONTEXT \
    std::clog << Simulator::Now ().GetSeconds () << " ";


#include "netlink-socket.h"
#include "netlink-socket-address.h"
#include "netlink-message.h"
#include "netlink-message-route.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4.h"
#include "ns3/simple-net-device.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/names.h"
#include <iostream>
#include <sstream>
#include "ns3/ipv6-address.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/socket.h"
#include "ns3/sll-header.h"
#include "ns3/mac48-address.h"
#include "../model/ipv4-dce-routing.h"
#include "../model/process.h"
#include "../model/utils.h"
#include <sys/socket.h>
#include <linux/if.h>
#include <errno.h>
#include "netlink-socket-factory.h"
//#include "ns3/ipv4-list-routing.h"

NS_LOG_COMPONENT_DEFINE ("DceNetlinkSocket");

namespace ns3 {

const int ARPHRD_NETLINK = 824;

// GroupSockets store the netlinksocket with noero group value
// it was due to the mulitcast netlink messages.
class GroupSockets
{
public:
  static uint32_t GetNSockets (void)
  {
    return m_Sockets.size ();
  }
  static Ptr<NetlinkSocket> GetSocket (uint32_t index)
  {
    NS_ASSERT (index < m_Sockets.size ());
    return m_Sockets[index];
  }
  static void AddSocket (Ptr<NetlinkSocket>sock)
  {
    m_Sockets.push_back (sock);
  }
private:
  /*use a std::vector to store the sockets with nozero group value*/
  static std::vector<Ptr<NetlinkSocket> >m_Sockets;
};
std::vector<Ptr<NetlinkSocket> >GroupSockets::m_Sockets;

NS_OBJECT_ENSURE_REGISTERED (NetlinkSocket);

/*
Netlink Socket
*/
TypeId
NetlinkSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetlinkSocket")
    .SetParent<Socket> ()
    .SetGroupName ("Netlink")
    .AddConstructor<NetlinkSocket> ()
    .AddTraceSource ("Drop", "Drop packet due to receive buffer overflow",
                     MakeTraceSourceAccessor (&NetlinkSocket::m_dropTrace),
                     "ns3::NetlinkSocket::PacketTracedCallback")
    .AddAttribute ("RcvBufSize",
                   "NetlinkSocket maximum receive buffer size (bytes)",
                   UintegerValue (0xffffffffl),
                   MakeUintegerAccessor (&NetlinkSocket::m_rcvBufSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("IcmpCallback", "Callback invoked whenever an icmp error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&NetlinkSocket::m_icmpCallback),
                   MakeCallbackChecker ())
    .AddTraceSource ("PromiscSniffer",
                     "Trace source simulating a promiscuous "
                     "packet sniffer attached to the device",
                     MakeTraceSourceAccessor (&NetlinkSocket::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

NetlinkSocket::NetlinkSocket ()
  : m_shutdownSend (false),
    m_shutdownRecv (false),
    m_rxAvailable (0),
    m_Pid (0),
    m_Groups (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_errno = ERROR_NOTERROR;
}
NetlinkSocket::~NetlinkSocket ()
{
  NS_LOG_FUNCTION (this);
}
void
NetlinkSocket::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

int
NetlinkSocket::ErrnoToSimuErrno (void)
{
  switch (m_errno)
    {
    case Socket::ERROR_ISCONN:
      return EISCONN;
    case Socket::ERROR_NOTCONN:
      return ENOTCONN;
    case Socket::ERROR_MSGSIZE:
      return EMSGSIZE;
    case Socket::ERROR_AGAIN:
      return EAGAIN;
    case Socket::ERROR_SHUTDOWN:
      return ESHUTDOWN;
    case Socket::ERROR_OPNOTSUPP:
      return EOPNOTSUPP;
    case Socket::ERROR_AFNOSUPPORT:
      return EAFNOSUPPORT;
    case Socket::ERROR_INVAL:
      return EINVAL;
    case Socket::ERROR_BADF:
      return EBADF;
    case Socket::ERROR_NOROUTETOHOST:
      return EHOSTUNREACH;
    case Socket::ERROR_NODEV:
      return ENODEV;
    case Socket::ERROR_ADDRNOTAVAIL:
      return EADDRNOTAVAIL;
    case Socket::SOCKET_ERRNO_LAST:
    case Socket::ERROR_NOTERROR:
    default:
      NS_ASSERT (false);
      return 0; // quiet compiler
      break;
    }
}

void
NetlinkSocket::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;

  NS_LOG_DEBUG ("SetNode: " << node->GetId ());
  // defer initialization of routing protocol pointers to DoBind call
}


enum Socket::SocketErrno
NetlinkSocket::GetErrno (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_errno;
}
enum Socket::SocketType
NetlinkSocket::GetSocketType (void) const
{
  return Socket::NS3_SOCK_DGRAM;
}

Ptr<Node>
NetlinkSocket::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

uint32_t
NetlinkSocket::GetPid (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_Pid;
}
uint32_t
NetlinkSocket::GetGroups (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_Groups;
}

int
NetlinkSocket::Bind (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  NetlinkSocketAddress address;
  return DoBind (address);
}
int
NetlinkSocket::Bind6 (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return -1;
}
int
NetlinkSocket::Bind (const Address &address)
{
  NS_LOG_FUNCTION (this << address);

  if (!NetlinkSocketAddress::IsMatchingType (address))
    {
      m_errno = ERROR_INVAL;
      return -1;
    }

  NetlinkSocketAddress ad = NetlinkSocketAddress::ConvertFrom (address);
  return DoBind (ad);
}
int
NetlinkSocket::DoBind (const NetlinkSocketAddress &address)
{
  NS_LOG_FUNCTION (this << address);

  m_Pid = address.GetProcessID ();
  m_Groups = address.GetGroupsMask ();

  Thread *current = Current ();
  if (current != 0) // will be 0 when opening socket from NS3 (e.g., Ipv4DceRouting)
    {
      NS_ASSERT (m_node != 0);
      Ptr<NetlinkSocketFactory> nsf = m_node->GetObject<NetlinkSocketFactory> ();

      if (m_Pid == 0)
        {
          m_Pid = current->process->pid;

          while (nsf->m_pidsList.count (m_Pid) > 0)
            {
              m_Pid++;
            }
        }

      nsf->m_pidsList.insert (m_Pid);
    }

  if (m_Groups)
    {
      GroupSockets::AddSocket (this);
    }

  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ipv4 != 0, "Netlink Socket requires IPv4 stack to be installed on the node");

  // We only care about staticRouting for netlink support
  m_ipv4Routing = Ipv4DceRouting::GetRouting (ipv4->GetRoutingProtocol (),
                                              (Ipv4DceRouting*)0);
  NS_ASSERT_MSG (m_ipv4Routing != 0,
                 "Netlink Socket requires Ipv4DceRouting to be installed on the node");

  return 0;
}

int
NetlinkSocket::Listen (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_errno = Socket::ERROR_OPNOTSUPP;
  return -1;
}

uint32_t
NetlinkSocket::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return 0;
}
uint32_t
NetlinkSocket::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // We separately maintain this state to avoid walking the queue
  // every time this might be called
  return m_rxAvailable;
}

int
NetlinkSocket::ShutdownSend (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_shutdownSend = true;
  return 0;
}
int
NetlinkSocket::ShutdownRecv (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_shutdownRecv = true;
  return 0;
}

int
NetlinkSocket::Close (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  ShutdownSend ();
  ShutdownRecv ();

  // a little bit complicated, but this will allow us to keep track of every open netlink socket
  if (m_node != 0)
    {
      Ptr<NetlinkSocketFactory> nsf = m_node->GetObject<NetlinkSocketFactory> ();

      std::multiset<uint32_t>::iterator i = nsf->m_pidsList.find (m_Pid);
      if (i != nsf->m_pidsList.end ())
        {
          nsf->m_pidsList.erase (i);
        }
    }

  return 0;
}

int
NetlinkSocket::Connect (const Address &address)
{
  NS_LOG_FUNCTION (this << address);
  m_errno = Socket::ERROR_OPNOTSUPP;
  return 0;
}

Ptr<Packet>
NetlinkSocket::Recv (uint32_t maxSize, uint32_t flags)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  Address dummy;

  return RecvFrom (maxSize, flags, dummy);
}

Ptr<Packet>
NetlinkSocket::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags << fromAddress);

  if (m_dataReceiveQueue.empty ())
    {
      return 0;
    }

  Ptr<Packet> p = m_dataReceiveQueue.front ().first;
  fromAddress = m_dataReceiveQueue.front ().second;
  if (p->GetSize () <= maxSize)
    {
      m_dataReceiveQueue.pop ();
      m_rxAvailable -= p->GetSize ();
    }
  else
    {
      p = 0;
    }
  return p;
}

int
NetlinkSocket::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p << flags);
  NetlinkSocketAddress address = NetlinkSocketAddress (m_Pid, m_Groups);
  return SendTo (p, flags, address);
}

int
NetlinkSocket::SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress)
{
  NS_LOG_FUNCTION (this << p << flags << toAddress);
  NetlinkSocketAddress ad;

  if (!NetlinkSocketAddress::IsMatchingType (toAddress))
    {
      NS_LOG_LOGIC ("ERROR_AFNOSUPPORT");
      m_errno = ERROR_AFNOSUPPORT;
      return -1;
    }

  NS_LOG_INFO ("send netlink message to pid = " <<
               NetlinkSocketAddress::ConvertFrom (toAddress));
  NS_LOG_DEBUG ("At " << Simulator::Now ().GetSeconds () << "s "
                "sending netlink message from node " << m_node->GetId ());

  //Ptr<NetlinkSocket>kernel_socket = GetNetlinkSocketByAddress(ad);
  //kernel_socket->m_receivedData.push_back(p);
  //kernel_socket->NotifyDataReceived(p);

  //when netlink socket send packet, the first step is to find the dest netlink socket through address
  //then send the packet to it. For we partly implement the netlink-family, the dest address
  //is always the kernel(pid = 0), (Actually, there must be one static kernel netlink socket to
  //receive/handle messages), we do not setup a kernel socket to receive packet.
  //

  MultipartNetlinkMessage multipartnlmsg;
  uint32_t packet_len, remain_len;

  packet_len = p->GetSize ();
  remain_len = packet_len;


  SllHeader header = SllHeader ();
  header.SetArpType (ARPHRD_NETLINK);
  header.SetPacketType (SllHeader::UNICAST_FROM_PEER_TO_ME);
  m_promiscSnifferTrace (header, p);

  while (remain_len > NetlinkMessageHeader::GetHeaderSize ())
    {
      remain_len -= p->RemoveHeader (multipartnlmsg);
      NS_ASSERT (remain_len == p->GetSize ());

      //actually, message to kernel contains single one netlink message
      for (uint32_t i = 0; i < multipartnlmsg.GetNMessages (); i++)
        {
          NetlinkMessage nlmsg = multipartnlmsg.GetMessage (i);
          if (HandleMessage (nlmsg) < 0)
            {
              if (m_errno)
                {
                  SendAckMessage (nlmsg, -ErrnoToSimuErrno ());
                }
            }
          else if (NetlinkMessage::IsMessageFlagsAck (nlmsg.GetHeader ().GetMsgFlags ()))
            {
              SendAckMessage (nlmsg, 0);
            }
        }
    }

  NotifyDataSent (packet_len);
  if (m_errno != 0)
    {
      NS_LOG_INFO ("netlink socket kernel error " << -m_errno);
    }

  return packet_len;
}

int
NetlinkSocket::GetSockName (Address &address) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NetlinkSocketAddress ad;

  ad.SetProcessID (GetPid ());
  ad.SetGroupsMask (GetGroups ());
  address = ad;
  return 0;
}
int
NetlinkSocket::GetPeerName (Address &address) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // XXX
  NS_ASSERT (false);
  return -1;
}
bool
NetlinkSocket::SetAllowBroadcast (bool allowBroadcast)
{
  NS_ASSERT (false);
  return false;
}
bool
NetlinkSocket::GetAllowBroadcast () const
{
  NS_ASSERT (false);
  return false;
}


void
NetlinkSocket::ForwardUp (Ptr<Packet> packet, const NetlinkSocketAddress &address)
{
  NS_LOG_FUNCTION (this << packet << address);

  if (m_shutdownRecv)
    {
      return;
    }
  if ((m_rxAvailable + packet->GetSize ()) <= m_rcvBufSize)
    {
      m_dataReceiveQueue.push (std::make_pair (packet, address));

      SllHeader header = SllHeader ();
      header.SetArpType (ARPHRD_NETLINK);
      header.SetPacketType (SllHeader::UNICAST_FROM_PEER_TO_ME);
      m_promiscSnifferTrace (header, packet);

      m_rxAvailable += packet->GetSize ();
      NotifyDataRecv ();
    }
  else
    {
      NS_LOG_WARN ("No receive buffer space available.  Drop.");
      m_dropTrace (packet);
    }
}

int32_t
NetlinkSocket::SendMessageUnicast (const MultipartNetlinkMessage &nlmsg, int32_t nonblock)
{
  NS_LOG_FUNCTION (this << nonblock);

  //here we send message instantly
  Ptr<Packet> p = Create<Packet> ();
  p->AddHeader (nlmsg);

  // NetlinkSocketAddress address;
  // address.SetProcessID (pid);

  //send packet to user space
  ForwardUp (p, NetlinkSocketAddress (m_kernelPid,0));
  return 0;
}

int32_t
NetlinkSocket::SendMessageBroadcast (const MultipartNetlinkMessage &nlmsg,
                                     uint32_t group,
                                     Ptr<Node> node)
{
  NS_LOG_FUNCTION ("SendMessageBroadcast" << group);
  //fisrt find the dest netlink socket through group value, then attach this nlmsg to its recv-queue
  for (uint32_t i = 0; i < GroupSockets::GetNSockets (); i++)
    {
      Ptr<NetlinkSocket> nlsock = GroupSockets::GetSocket (i);

      if ((nlsock->GetGroups () & group)
          && (nlsock->GetPid () != m_kernelPid)
          && node == nlsock->GetNode ())
        {
          NS_LOG_DEBUG ("SendMessageBroadcast to pid " << nlsock->GetPid ());

          //here we send message instantly
          Ptr<Packet> p = Create<Packet> ();
          p->AddHeader (nlmsg);

          //send packet to user space
          nlsock->ForwardUp (p, NetlinkSocketAddress (m_kernelPid,group));
        }
    }
  return 0;
}
void
NetlinkSocket::SendAckMessage (const NetlinkMessage &nlmsg, int32_t err)
{
  NS_LOG_FUNCTION (this << err);
  NetlinkMessageHeader rep;
  NetlinkMessage ackmsg;
  NetlinkMessageError errmsg;

  rep.SetMsgPid (nlmsg.GetHeader ().GetMsgPid ());
  rep.SetMsgSeq (nlmsg.GetHeader ().GetMsgSeq ());
  rep.SetMsgType (NETLINK_MSG_ERROR);
  rep.SetMsgFlags (0);

  errmsg.SetError (err);
  //kernel send the whole nlmsg back if error != 0, here we just send the header back
  errmsg.SetMsg (nlmsg.GetHeader ());

  //then send errmsg back to user space
  ackmsg.SetHeader (rep);
  ackmsg.SetErrorMessage (errmsg);

  SendMessageUnicast (ackmsg, 1);
}

int32_t
NetlinkSocket::HandleMessage (const NetlinkMessage &nlmsg)
{
  NS_LOG_FUNCTION (this);
  uint16_t type = nlmsg.GetMsgType ();
  NetlinkMessageHeader nhr = nlmsg.GetHeader ();

  if (nhr.GetMsgLen () < NetlinkMessageHeader::GetHeaderSize ())
    {
      m_errno = ERROR_INVAL;
      return -1;
    }

  if (NetlinkMessage::IsMessageNetlinkControl (type))
    {
      NS_LOG_INFO ("netlink control message type not parsed in kernel");
      return 0;
    }
  else if (NetlinkMessage::IsMessageNetlinkRoute (type))
    {
      return HandleNetlinkRouteMessage (nlmsg);
    }
  else
    {
      NS_LOG_INFO ("netlink message type not parsed in kernel");
      m_errno = ERROR_INVAL;
      return -1;
    }
}

int32_t
NetlinkSocket::HandleNetlinkRouteMessage (const NetlinkMessage &nlmsg)
{
  NS_LOG_FUNCTION (this);
  uint8_t family;
  int16_t type;
  int32_t err;

  /* Only requests are handled by kernel now */
  if (!NetlinkMessage::IsMessageFlagsRequest (nlmsg.GetHeader ().GetMsgFlags ()))
    {
      NS_LOG_WARN("Only requests are handled by kernel now");
      return 0;
    }

  type = nlmsg.GetMsgType ();

  /* A control message: ignore them */
  if (NetlinkMessage::IsMessageNetlinkControl (type))
    {
      NS_LOG_WARN("Ignoring message control");
      return 0;
    }
  else if (NetlinkMessage::IsMessageNetlinkRoute (type))
    {
      /* All the messages must have at least 1 byte length */
      if (nlmsg.GetPayloadSize () < 1)
        {
          return 0;
        }

      family = nlmsg.GetFamily ();
      /*here we do not deal with different family, default for AF_NET*/
      NS_ASSERT (family == AF_INET || family == AF_UNSPEC || family == AF_PACKET || family == AF_INET6);

      /*for GET*** message, dump it to userspace*/
      if (NetlinkMessage::IsMessageTypeGet (type)
          && NetlinkMessage::IsMessageFlagsDump (nlmsg.GetHeader ().GetMsgFlags ()))
        {
          DumpNetlinkRouteMessage (nlmsg, type, family);
          return -1;
        }

      /* other types of messages*/
      return DoNetlinkRouteMessage (nlmsg, type, family);
    }
  else /* Unknown message: reply with EINVAL */
    {
      err = ERROR_INVAL;
      return -1;
    }
}

int32_t
NetlinkSocket::DumpNetlinkRouteMessage (const NetlinkMessage &nlmsg, uint16_t type, uint8_t family)
{
  NS_LOG_FUNCTION (this << "type=" << type << "family=" << NetlinkRtmTypeToStr(type));

  NS_ASSERT (type == NETLINK_RTM_GETADDR || type == NETLINK_RTM_GETROUTE || type == NETLINK_RTM_GETLINK);

  MultipartNetlinkMessage nlmsg_dump;
  NetlinkMessageHeader nhr = nlmsg.GetHeader ();
  uint32_t seq = nhr.GetMsgSeq();
  int32_t err;

  if (type == NETLINK_RTM_GETADDR)
    {
      nlmsg_dump = BuildInterfaceAddressDumpMessages (seq);
    }
  else if (type == NETLINK_RTM_GETLINK)
    {
      nlmsg_dump = BuildInterfaceInfoDumpMessages (seq);
    }
  else if (type == NETLINK_RTM_GETROUTE)
    {
      nlmsg_dump = BuildRouteDumpMessages (seq);
    }
  else
    {
      NS_LOG_ERROR ("Unknown type " << type);
      m_errno = ERROR_INVAL;
      return -1;
    }

  //then append netlink message with type NLMSG_DONE
  NetlinkMessage nlmsg_done;
  NetlinkMessageHeader nhr2 = NetlinkMessageHeader (NETLINK_MSG_DONE, NETLINK_MSG_F_MULTI,
                                                    nhr.GetMsgSeq (), m_Pid);

  nlmsg_done.SetHeader (nhr2);
  //kernel append nlmsg_dump size to it, here we omit it
  nlmsg_dump.AppendMessage (nlmsg_done);
  err = SendMessageUnicast (nlmsg_dump, 1);
  return err;
}

/*here only for ADD/DEL/GET*** types*/
int32_t
NetlinkSocket::DoNetlinkRouteMessage (const NetlinkMessage &nlmsg, uint16_t type, uint8_t family)
{
  NS_LOG_FUNCTION (this << type << family);
  int32_t err;

  if (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR)
    {
      err = DoInterfaceAddressMessage (nlmsg, type, family);
    }
  else if (type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE || type == NETLINK_RTM_GETROUTE)
    {
      err = DoRouteMessage (nlmsg, type, family);
    }
  else if (type == NETLINK_RTM_GETLINK || type == NETLINK_RTM_SETLINK)
    {
      err = DoInterfaceInfoMessage (nlmsg, type, family);
    }
  else
    {
      NS_LOG_LOGIC ("netlink message:type( " << type << ") not processed by ns3 now.");
      m_errno = ERROR_INVAL;
      err = -1;
    }

  return err;
}

MultipartNetlinkMessage
NetlinkSocket::BuildInterfaceAddressDumpMessages (uint32_t received_seq)
{
  NS_LOG_FUNCTION (this);
  MultipartNetlinkMessage nlmsg_dump;
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();

  for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++)
    {
      if (!ipv4->IsUp (i))
        {
          continue;
        }

      Ipv4Address addri = ipv4->GetAddress (i, 0).GetLocal ();
      Ipv4Mask maski = ipv4->GetAddress (i, 0).GetMask ();
      Ipv4Address bcast = ipv4->GetAddress (i, 0).GetBroadcast ();

      NS_LOG_DEBUG (ipv4->GetAddress (i, 0));

      //here get the address mask length
      uint32_t mask = maski.Get ();
      uint8_t mask_len = 0;
      while (mask)
        {
          mask = mask << 1;
          mask_len++;
        }

      //next fill the message body
      NetlinkMessage nlmsg_ifa;
      NetlinkMessageHeader nhr = NetlinkMessageHeader (NETLINK_RTM_NEWADDR, NETLINK_MSG_F_MULTI,
                                received_seq,
                                m_Pid
                                );
      InterfaceAddressMessage ifamsg;

      ifamsg.SetInterfaceIndex (i);
      ifamsg.SetFamily (AF_INET); //default AF_INET
      ifamsg.SetLength (mask_len);
      ifamsg.SetFlags (0);
      ifamsg.SetScope (RouteMessage::RT_SCOPE_UNIVERSE);

      std::string name = UtilsGenerateIfNameFromIndex (i);
      ifamsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_IFNAME,    STRING, name ));
      ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LOCAL,    ADDRESS, addri));
      ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_ADDRESS,  ADDRESS, addri));
      ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_BROADCAST,ADDRESS, bcast));
      //      ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LABEL,    STRING,  "ns3-ifaddr"));//not used in ns3
      //ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_ANYCAST,  ADDRESS, Ipv4Address("0.0.0.0")));//not used in ns3
      //XXXother attributes not used by ns3

      nlmsg_ifa.SetHeader (nhr);
      nlmsg_ifa.SetInterfaceAddressMessage (ifamsg);
      nlmsg_dump.AppendMessage (nlmsg_ifa);
    }

  // For IPv6
  Ptr<Ipv6>ipv6 = m_node->GetObject<Ipv6> ();

  for (uint32_t i = 0; i < ipv6->GetNInterfaces (); i++)
    {
      if (!ipv6->IsUp (i))
        {
          continue;
        }

      for (uint32_t j = 0; j < ipv6->GetNAddresses (i); j++)
        {
          Ipv6Address addri = ipv6->GetAddress (i, j).GetAddress ();
          Ipv6Prefix prefix = ipv6->GetAddress (i, j).GetPrefix ();

          //here get the address mask length
          uint8_t mask_len = prefix.GetPrefixLength ();

          //loopback address's prefix is wrong... FIXME
          if (addri.IsEqual (Ipv6Address::GetLoopback ()))
            {
              mask_len = 128;
            }

          //next fill the message body
          NetlinkMessage nlmsg_ifa;
          NetlinkMessageHeader nhr = NetlinkMessageHeader (
            NETLINK_RTM_NEWADDR, NETLINK_MSG_F_MULTI, received_seq, m_Pid);
          InterfaceAddressMessage ifamsg;

          ifamsg.SetInterfaceIndex (i);
          ifamsg.SetFamily (AF_INET6);
          ifamsg.SetFlags (0);

          if (addri.IsLinkLocal ())
            {
              ifamsg.SetLength (64);
              ifamsg.SetScope (RouteMessage::RT_SCOPE_LINK);
            }
          else
            {
              ifamsg.SetLength (mask_len);
              ifamsg.SetScope (RouteMessage::RT_SCOPE_UNIVERSE);
            }

          std::string name = UtilsGenerateIfNameFromIndex (i);

          ifamsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_IFNAME,    STRING,  name));
          ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LOCAL,    ADDRESS, addri));
          ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LOCAL,    ADDRESS, addri));
          ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_ADDRESS,  ADDRESS, addri));
          //XXXother attributes not used by ns3

          nlmsg_ifa.SetHeader (nhr);
          nlmsg_ifa.SetInterfaceAddressMessage (ifamsg);
          nlmsg_dump.AppendMessage (nlmsg_ifa);
        }
    }
  return nlmsg_dump;
}

NetlinkMessage
NetlinkSocket::BuildInterfaceInfoDumpMessage (uint32_t interface_num, uint32_t seq)
{
  NS_LOG_FUNCTION (this << interface_num << seq);

  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  Ptr<NetDevice> dev = m_node->GetDevice (interface_num);

  int32_t ipv4_link = ipv4->GetInterfaceForDevice (dev);
  NS_ASSERT (ipv4_link >= 0);

  Address mac;
  Address bcast;
  uint32_t mtu;
  uint32_t flags = 0;

  mac = dev->GetAddress ();
  bcast = dev->GetBroadcast ();
  mtu = (uint32_t)dev->GetMtu ();

  //  if (dev->IsLinkUp ())
  // Currently, there is not an easy way to set interface down. Use ipv4 stack's link instead
  // This is not a really nice solution, but what else can we do for now?
  if (ipv4->IsUp (ipv4_link))
    {
      flags |= IFF_RUNNING;
      flags |= IFF_UP;
    }
  if (dev->IsBroadcast ())
    {
      flags |= IFF_BROADCAST;
    }
  if (dev->IsMulticast ())
    {
      flags |= IFF_MULTICAST;
    }

  NetlinkMessage nlmsg_ifinfo;
  NetlinkMessageHeader nhr = NetlinkMessageHeader (NETLINK_RTM_NEWLINK, NETLINK_MSG_F_MULTI, seq, m_Pid);
  InterfaceInfoMessage ifinfomsg;

  ifinfomsg.SetFamily (0);      // AF_UNSPEC
  ifinfomsg.SetDeviceType (0); // not clear
  ifinfomsg.SetInterfaceIndex (interface_num);
  ifinfomsg.SetDeviceFlags (flags); // not clear
  ifinfomsg.SetChangeMask (0xffffffff);

  // the ns3 device have no  name, here we set "ns3-device i" for test
  std::stringstream ss;
  ss <<  "ns3-device" << interface_num;

  ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_IFNAME,    STRING,  ss.str ()));
  //not used in ns3
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_TXQLEN,    U32,     0));
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_WEIGHT,    U32,     0));
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_OPERSTATE, U8,      0));
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_LINKMODE,  U8,      0));
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_MAP,       UNSPEC,  0));
  ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_ADDRESS,   ADDRESS, mac));
  ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_BROADCAST, ADDRESS, bcast));
  ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_MTU,       U32,     mtu));
  ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_LINK,      U32,     interface_num));
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_QDISC,     STRING,  ""));
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_MASTER,    U32,     0));
  //ifinfomsg.AppendAttribute (NetlinkAttribute (InterfaceInfoMessage::IFL_A_STATS,     UNSPEC,  0));

  nlmsg_ifinfo.SetHeader (nhr);
  nlmsg_ifinfo.SetInterfaceInfoMessage (ifinfomsg);

  return nlmsg_ifinfo;
}

MultipartNetlinkMessage
NetlinkSocket::BuildInterfaceInfoDumpMessages (uint32_t seq)
{
  MultipartNetlinkMessage nlmsg_dump;
  for (uint32_t i = 0; i < m_node->GetNDevices (); i++)
    {
      NetlinkMessage msg = BuildInterfaceInfoDumpMessage (i, seq);
      nlmsg_dump.AppendMessage (msg);
    }
  return nlmsg_dump;
}
MultipartNetlinkMessage
NetlinkSocket::BuildRouteDumpMessages (uint32_t seq)
{
  NS_LOG_FUNCTION (this);
  MultipartNetlinkMessage nlmsg_dump;

  if (0 == m_ipv4Routing)
    {
      return nlmsg_dump;
    }

  NS_ASSERT_MSG (m_ipv4Routing != 0, "Should not happen");

  // We only care about staticRouting for netlink support
  for (uint32_t i = 0; i < m_ipv4Routing->GetNRoutes (); i++)
    {
      NetlinkMessage nlmsg_rt;
      NetlinkMessageHeader nhr = NetlinkMessageHeader (NETLINK_RTM_NEWROUTE, NETLINK_MSG_F_MULTI,
                            seq, m_Pid);
      RouteMessage rtmsg;
      Ipv4RoutingTableEntry route = m_ipv4Routing->GetRoute (i);

      rtmsg.SetFamily (AF_INET);
      rtmsg.SetDstLength (32);
      rtmsg.SetSrcLength (0);
      rtmsg.SetTos (0); //not clear
      rtmsg.SetTableId (RouteMessage::RT_TABLE_MAIN);
      rtmsg.SetScope (RouteMessage::RT_SCOPE_UNIVERSE);
      rtmsg.SetProtocol (RouteMessage::RT_PROT_UNSPEC);
      rtmsg.SetFlags (RouteMessage::RT_F_CLONED);

      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_DST, ADDRESS, route.GetDest ()));
      // ns3 use local address as the route src address
      //      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_SRC, ADDRESS, route.GetSource()));
      //      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_PREFSRC, ADDRESS, route.GetSource()));//not used in ns3
      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_IIF, U32, route.GetInterface ()));
      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_OIF, U32, route.GetInterface ()));
      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_GATEWAY, ADDRESS, route.GetGateway ()));

      nlmsg_rt.SetHeader (nhr);
      nlmsg_rt.SetRouteMessage (rtmsg);
      nlmsg_dump.AppendMessage (nlmsg_rt);
    }

  Ptr<Ipv6> ipv6 = m_node->GetObject<Ipv6> ();
  // We only care about staticRouting for netlink support
  Ipv6StaticRoutingHelper routingHelper6;
  Ptr<Ipv6StaticRouting> ipv6Static = routingHelper6.GetStaticRouting (ipv6);
  for (uint32_t i = 0; i < ipv6Static->GetNRoutes (); i++)
    {
      NetlinkMessage nlmsg_rt;
      NetlinkMessageHeader nhr = NetlinkMessageHeader (NETLINK_RTM_NEWROUTE, NETLINK_MSG_F_MULTI, 0, m_kernelPid);
      RouteMessage rtmsg;
      Ipv6RoutingTableEntry route = ipv6Static->GetRoute (i);

      rtmsg.SetFamily (AF_INET6);
      rtmsg.SetDstLength (128);
      rtmsg.SetSrcLength (0);
      rtmsg.SetTos (0); //not clear
      rtmsg.SetTableId (RouteMessage::RT_TABLE_MAIN);
      rtmsg.SetScope (RouteMessage::RT_SCOPE_UNIVERSE);
      rtmsg.SetProtocol (RouteMessage::RT_PROT_UNSPEC);
      rtmsg.SetFlags (RouteMessage::RT_F_CLONED);

      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_DST, ADDRESS, route.GetDest ()));
      //ns3 use local address as the route src address
      // rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_SRC, ADDRESS,
      //                                          ipv6->GetSourceAddress(route.GetDest ())));
      // rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_PREFSRC, ADDRESS,
      //                                          ipv6->GetSourceAddress(route.GetDest ())));
      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_IIF, U32, route.GetInterface ()));
      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_OIF, U32, route.GetInterface ()));
      rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_GATEWAY, ADDRESS, route.GetGateway ()));

      nlmsg_rt.SetHeader (nhr);
      nlmsg_rt.SetRouteMessage (rtmsg);
      nlmsg_dump.AppendMessage (nlmsg_rt);
    }

  return nlmsg_dump;
}

int32_t
NetlinkSocket::DoInterfaceAddressMessage (const NetlinkMessage &nlmsg, uint16_t type, uint8_t family)
{
  NS_LOG_FUNCTION (this << type << family);
  NS_ASSERT (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR);

  // XXX
  NS_ASSERT_MSG (false, "Not implemented yet (RTM_NEWADDR/RTM_DELADDR)");

  InterfaceAddressMessage ifamsg = nlmsg.GetInterfaceAddressMessage ();
  Ipv4Address addri, addr_local, bcast;
  NetlinkAttribute attr_local;
  uint32_t index = ifamsg.GetInterfaceIndex ();
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  int flag4 = 0, flag6 = 0;

  if (type == NETLINK_RTM_NEWADDR)
    {
      //when adding an interface address, it should check the input arguments
      //prefix-len and local address attribute
      if (ifamsg.GetLength () > 32
          || ifamsg.GetAttributeByType (attr_local, InterfaceAddressMessage::IF_A_LOCAL) == false)
        {
          m_errno = ERROR_INVAL;
          return -1;
        }
    }

  //get necessary information for add/del, many attributes we not used
  for (uint32_t i = 0; i < ifamsg.GetNNetlinkAttribute (); i++)
    {
      NetlinkAttribute attr = ifamsg.GetNetlinkAttribute (i);
      uint32_t attr_type = attr.GetAttrType ();

      switch (attr_type)
        {
        case InterfaceAddressMessage::IF_A_ADDRESS:
          addri = Ipv4Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
          break;
        case InterfaceAddressMessage::IF_A_BROADCAST:
          bcast = Ipv4Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
          break;
        case InterfaceAddressMessage::IF_A_LOCAL:
          addr_local = Ipv4Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
          break;
        case InterfaceAddressMessage::IF_A_LABEL:
        case InterfaceAddressMessage::IF_A_ANYCAST:
          break;
        }
    }

  if (type == NETLINK_RTM_NEWADDR)
    {
      //when adding an interface address by index, if the indexed interface was not exist,
      //create an new NetDevice with an new index and set the address
      //otherwise set the indexed interface directly
      if (index >= ipv4->GetNInterfaces ())
        {
          Ptr<SimpleNetDevice> dev;
          dev = CreateObject<SimpleNetDevice> ();
          dev->SetAddress (Mac48Address::Allocate ());
          m_node->AddDevice (dev);

          uint32_t netdev_idx = ipv4->AddInterface (dev);
          // FIXME!
          Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (addri, Ipv4Mask ());
          ipv4->AddAddress (netdev_idx, ipv4Addr);
          ipv4->SetUp (netdev_idx);
          NS_LOG_INFO ("Add an interface address at index " << netdev_idx << "but not the ifamsg input" << index);
        }
      else
        {
          Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (addri, Ipv4Mask ());
          ipv4->AddAddress (index, ipv4Addr);
          if (!ipv4->IsUp (index))
            {
              ipv4->SetUp (index);
            }
        }
      flag4 = 1;
    }
  else //type == NETLINK_RTM_DELADDR
    {
      //when delete an interface address by index, if the indexed interface  was not exist
      //return an error EINVAL, otherwise set down the interface which has the addri
      if (index >= ipv4->GetNInterfaces ())
        {
          m_errno = ERROR_NODEV;
          return -1;
        }
      else
        {
          for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++)
            {
              Ipv4Address ad = ipv4->GetAddress (i, 0).GetLocal ();
              if (ad == addri && ipv4->IsUp (i))
                {
                  ipv4->SetDown (i);
                  break;
                }
              if (i == ipv4->GetNInterfaces () - 1)
                {
                  m_errno = ERROR_ADDRNOTAVAIL;
                  return -1;
                }
            }
          flag4 = 1;
        }
    }

  //then send an broadcast message, let all user know this operation happened
  NetlinkMessage nlmsg_broadcast = nlmsg;
  NetlinkMessageHeader nhr;
  nhr.SetMsgLen (nlmsg.GetHeader ().GetMsgLen ());
  nhr.SetMsgType (nlmsg.GetHeader ().GetMsgType ());
  nlmsg_broadcast.SetHeader (nhr);
  if (flag4)
    {
      SendMessageBroadcast (nlmsg_broadcast, NETLINK_RTM_GRP_IPV4_IFADDR, GetNode ());
    }
  else if (flag6)
    {
      SendMessageBroadcast (nlmsg_broadcast, RTMGRP_IPV6_IFADDR, GetNode ());
    }

  return 0;
}

int32_t
NetlinkSocket::DoInterfaceInfoMessage (const NetlinkMessage &nlmsg, uint16_t type, uint8_t family)
{
  NS_LOG_FUNCTION (this << type << family);
  NS_ASSERT (type == NETLINK_RTM_GETLINK || type == NETLINK_RTM_SETLINK);
  InterfaceInfoMessage ifinfomsg = nlmsg.GetInterfaceInfoMessage ();
  // XXX
  NS_ASSERT_MSG (false, "Not implemented yet (RTM_GETLINK/RTM_SETLINK)");
  return -1;
}

Address
NetlinkSocket::ConvertFrom (uint8_t family, const Address &address)
{
  Address retval;
  if (family == AF_INET)
    {
      retval = Ipv4Address::ConvertFrom (address);
    }
  else if (family == AF_INET6)
    {
      retval = Ipv6Address::ConvertFrom (address);
    }
  return retval;
}

int32_t
NetlinkSocket::DoRouteMessage (const NetlinkMessage &nlmsg, uint16_t type, uint8_t family)
{
  NS_LOG_FUNCTION (this << type << family);
  NS_ASSERT (type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE ||type == NETLINK_RTM_GETROUTE);

  RouteMessage rtmsg = nlmsg.GetRouteMessage ();
  Ipv4Address src, dest, gateway;
  Ipv6Address src6, dest6, gateway6;
  uint32_t index = 0;
  int attr_flags[RouteMessage::RT_A_MAX] = { 0};
  uint8_t dstlen = rtmsg.GetDstLength ();
  uint32_t metric = 0;

  //get necessary information for add/del, many attributes we not used
  for (uint32_t i = 0; i < rtmsg.GetNNetlinkAttribute (); i++)
    {
      NetlinkAttribute attr = rtmsg.GetNetlinkAttribute (i);
      uint32_t attr_type = attr.GetAttrType ();
      attr_flags[attr_type] = 1;

      switch (attr_type)
        {
        case RouteMessage::RT_A_DST:
          if (family == AF_INET)
            {
              dest = Ipv4Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
            }
          else if (family == AF_INET6)
            {
              dest6 = Ipv6Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
            }
          break;
        case RouteMessage::RT_A_SRC:
          if (family == AF_INET)
            {
              src = Ipv4Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
            }
          else if (family == AF_INET6)
            {
              src6 = Ipv6Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
            }
          break;
        case RouteMessage::RT_A_OIF:
          index = attr.GetAttrPayload ().GetU32 ();
          break;
        case RouteMessage::RT_A_GATEWAY:
          if (family == AF_INET)
            {
              gateway = Ipv4Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
            }
          else if (family == AF_INET6)
            {
              gateway6 = Ipv6Address::ConvertFrom (attr.GetAttrPayload ().GetAddress ());
            }
          break;
        case RouteMessage::RT_A_PRIORITY:
          metric = attr.GetAttrPayload ().GetU32 ();
          NS_LOG_INFO ("Got metric: " << metric);
          break;
        case RouteMessage::RT_A_IIF:
        case RouteMessage::RT_A_PREFSRC:
        case RouteMessage::RT_A_METRICS:
        case RouteMessage::RT_A_MULTIPATH:
        case RouteMessage::RT_A_PROTOINFO:
        case RouteMessage::RT_A_FLOW:
        case RouteMessage::RT_A_CACHEINFO:
        case RouteMessage::RT_A_SESSION:
        case RouteMessage::RT_A_MP_ALGO:
        case RouteMessage::RT_A_TABLE:
          NS_LOG_INFO ("route attribute not used by ns3" << attr_type);
          //not used by ns3
          break;
        }
    }

  // Sigh....
  Ptr<Ipv4>ipv4 = m_node->GetObject<Ipv4> ();

  Ptr<Ipv6>ipv6 = m_node->GetObject<Ipv6> ();
  Ipv6StaticRoutingHelper routingHelper6;
  Ptr<Ipv6StaticRouting> ipv6Static = routingHelper6.GetStaticRouting (ipv6);

  NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << " Route message, type: " << type << "; from " << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
                                                << " to " << dest << " through " << gateway);

  if (type == NETLINK_RTM_NEWROUTE)
    {
      //ns3 add a route entry only depends on 2 or 3 attribute
      //other route msg attibute were ignored
      if (attr_flags[RouteMessage::RT_A_DST])
        {
          if (family == AF_INET)
            {
              if (!attr_flags[RouteMessage::RT_A_OIF])
                {
                  bool found = 0;
                  for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i++)
                    {
                      for (uint32_t j = 0; j < ipv4->GetNAddresses (i); j++)
                        {
                          if ((attr_flags[RouteMessage::RT_A_GATEWAY]))
                            {
                              Ipv4Mask mask = ipv4->GetAddress (i, j).GetMask ();
                              if (mask.IsMatch (ipv4->GetAddress (i, j).GetLocal (), gateway))
                                {
                                  index = i;
                                  found = true;
                                  break;
                                }
                            }
                          if (found)
                            {
                              break;
                            }
                        }
                    }
                  if (!found)
                    {
                      NS_LOG_DEBUG ("No suitable interface to add an route entry");
                      m_errno = ERROR_ADDRNOTAVAIL;
                      return -1;
                    }
                }
              if (dstlen == 32)
                {
                  int exist_flag = 0;
                  for (uint32_t i = 0; i < m_ipv4Routing->GetNRoutes (); ++i)
                    {
                      Ipv4RoutingTableEntry rt = m_ipv4Routing->GetRoute (i);
                      if (dest == rt.GetDest ())
                        {
                          exist_flag = 1;
                        }
                    }

                  if (exist_flag)
                    { //route to dest already exists
                      int delete_flag = 0;
                      if (nlmsg.GetHeader ().GetMsgFlags () & NETLINK_MSG_F_REPLACE)
                        {
                          for (uint32_t i = 0; i < m_ipv4Routing->GetNRoutes (); ++i)
                            {
                              Ipv4RoutingTableEntry rt = m_ipv4Routing->GetRoute (i);
                              if (dest == rt.GetDest ())
                                {
                                  m_ipv4Routing->RemoveRoute (i);
                                  NS_LOG_DEBUG ("Route from  " << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () << " to "
                                                               << dest << " through " << gateway << " removed");
                                  delete_flag = 1;
                                }
                            }

                          if (!delete_flag)
                            {
                              NS_LOG_INFO ("no route entry removed by dest address in new route sector " << dest);
                              m_errno = ERROR_INVAL;
                              return -1;
                            }
                        }
                      else
                        {
                          NS_LOG_DEBUG ("Route exists but overwriting declined!");
                        }
                      if ((attr_flags[RouteMessage::RT_A_GATEWAY]))
                        {
                          NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "Overwrite route from "
                                                                        << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () << " to " << dest << " through " << gateway << " with index " << index << " and metric " << metric);
                          m_ipv4Routing->AddHostRouteTo (dest, gateway, index, metric);
                        }
                      else
                        {
                          NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                                      << "Overwrite route from "
                                      << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
                                      << " to " << dest << " through " << "self"
                                      << " with index " << index
                                      << " and metric " << metric);
                          m_ipv4Routing->AddHostRouteTo (dest, index, metric);
                        }
                    }
                  else
                    {
                      //route to dest doesn't exist
                      if (nlmsg.GetHeader ().GetMsgFlags () & NETLINK_MSG_F_CREATE)
                        {
                          if (attr_flags[RouteMessage::RT_A_GATEWAY])
                            {
                              NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "Add new route from " << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
                                                                            << " to " << dest << " through " << gateway << " with index" << index);
                              m_ipv4Routing->AddHostRouteTo (dest, gateway, index, metric);
                            }
                          else
                            {
                              NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "Add new route from " << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
                                                                            << " to " << dest << " through " << "self" << " with index" << index);
                              m_ipv4Routing->AddHostRouteTo (dest, index, metric);
                            }
                        }
                      else
                        {
                          NS_LOG_ERROR ("Route doesn't exist but writing declined!");
                        }
                    }

                  NS_LOG_DEBUG ("=After change attempt=");
                  //Dump of table
                  NS_LOG_DEBUG (m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () << ":");
                  for (uint32_t i = 0; i < m_ipv4Routing->GetNRoutes (); ++i)
                    {
                      Ipv4RoutingTableEntry rt = m_ipv4Routing->GetRoute (i);
                      NS_LOG_DEBUG (rt.GetDest () << " through " << rt.GetGateway ());
                    }
                  NS_LOG_DEBUG ("= = = = = = = = = = =");
                }
              else // dstlen != 32
                {
                  if (attr_flags[RouteMessage::RT_A_GATEWAY])
                    {
                      m_ipv4Routing->AddNetworkRouteTo (dest, Ipv4Mask (~(1 << (32 - dstlen)) + 1), gateway, index, metric);
                    }
                  else
                    {
                      m_ipv4Routing->AddNetworkRouteTo (dest, Ipv4Mask (~(1 << (32 - dstlen)) + 1), index, metric);
                    }
                }
            }
          else if (family == AF_INET6)
            {
              if (!attr_flags[RouteMessage::RT_A_OIF])
                {
#ifdef FIXME
                  if (ipv6->GetIfIndexForDestination (gateway6, index) == false)
                    {
                      NS_LOG_INFO ("No suitable interface to add an route entry");
                      m_errno = ERROR_ADDRNOTAVAIL;
                      return -1;
                    }
#endif
                }

              Ipv6Prefix pref (dstlen);
              if (attr_flags[RouteMessage::RT_A_GATEWAY])
                {
                  ipv6Static->AddNetworkRouteTo (dest6, pref, gateway6, index, metric);
                }
              else
                {
                  ipv6Static->AddNetworkRouteTo (dest6, pref, Ipv6Address ("::"), index, metric);
                }
            }
        }
      else
        {
          NS_LOG_INFO ("too few attributes to add an route entry");
          m_errno = ERROR_INVAL;
          return -1;
        }
    }
  else if (type == NETLINK_RTM_DELROUTE)
    {
      NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "Route delete request from " << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()
                                                    << " to " << dest << " through " << gateway);
      if (attr_flags[RouteMessage::RT_A_DST])
        {
          int delete_flag = 0;

          if (family == AF_INET)
            {
              for (uint32_t i = 0; i < m_ipv4Routing->GetNRoutes (); i++)
                {
                  Ipv4RoutingTableEntry rt = m_ipv4Routing->GetRoute (i);
                  if (gateway == rt.GetGateway () && dest == rt.GetDest ())
                    {
                      m_ipv4Routing->RemoveRoute (i);
                      delete_flag = 1;
                    }
                }
            }
          else if (family == AF_INET6)
            {
              for (uint32_t i = 0; i < ipv6Static->GetNRoutes (); i++)
                {
                  Ipv6RoutingTableEntry rt = ipv6Static->GetRoute (i);
                  if (gateway6 == rt.GetGateway () && dest6 == rt.GetDest ())
                    {
                      ipv6Static->RemoveRoute (i);
                      delete_flag = 1;
                    }
                }
            }

          if (!delete_flag)
            {
              NS_LOG_INFO ("no route entry removed by dest address " << dest);
              m_errno = ERROR_INVAL;
              return -1;
            }
        }
      else
        {
          NS_LOG_INFO ("too few attributes to add an route entry");
          m_errno = ERROR_INVAL;
          return -1;
        }
    }
  else // type == NETLINK_RTM_GETROUTE
    {
      NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "GetRoute " << "from " << m_node->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal () << " to " << dest);
      if (!attr_flags[RouteMessage::RT_A_DST])
        {
          NS_LOG_INFO ("too few attributes to get an route entry");
          m_errno = ERROR_INVAL;
          return -1;
        }

      int get_flag = 0;
      if (family == AF_INET)
        {
          for (uint32_t i = 0; i < m_ipv4Routing->GetNRoutes (); i++)
            {
              Ipv4RoutingTableEntry route = m_ipv4Routing->GetRoute (i);
              //find the route entry with same dest address and send unicast to user space
              if (dest.IsEqual (route.GetDest ()))
                {
                  //                Ptr<Ipv4>ipv4 = m_node->GetObject<Ipv4> ();
                  NetlinkMessage nlmsg_route;
                  NetlinkMessageHeader nhr = NetlinkMessageHeader (NETLINK_RTM_NEWROUTE, 0,
                                                                   nlmsg.GetHeader ().GetMsgSeq (), m_kernelPid);
                  RouteMessage rtmsg;

                  //fill rtmsg and attributes
                  rtmsg.SetFamily (AF_INET);
                  rtmsg.SetDstLength (32);
                  rtmsg.SetSrcLength (0);
                  rtmsg.SetTos (0); //not clear
                  rtmsg.SetTableId (RouteMessage::RT_TABLE_MAIN);
                  rtmsg.SetScope (RouteMessage::RT_SCOPE_UNIVERSE);
                  rtmsg.SetProtocol (RouteMessage::RT_PROT_UNSPEC);
                  rtmsg.SetFlags (RouteMessage::RT_F_CLONED);

                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_DST, ADDRESS, route.GetDest ()));
                  //ns3 use local address as the route src address
                  // rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_SRC, ADDRESS, ipv4->GetSourceAddress(route.GetDest ())));
                  // rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_PREFSRC, ADDRESS, ipv4->GetSourceAddress(route.GetDest ())));
                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_IIF, U32, route.GetInterface ()));
                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_OIF, U32, route.GetInterface ()));
                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_GATEWAY, ADDRESS, route.GetGateway ()));

                  //fill an netlink message body
                  nlmsg_route.SetHeader (nhr);
                  nlmsg_route.SetRouteMessage (rtmsg);

                  SendMessageUnicast (nlmsg_route, 1);
                  get_flag = 1;
                }
            }
        }
      else if (family == AF_INET6)
        {
          for (uint32_t i = 0; i < ipv6Static->GetNRoutes (); i++)
            {
              Ipv6RoutingTableEntry route = ipv6Static->GetRoute (i);
              //find the route entry with same dest address and send unicast to user space
              if (dest6.IsEqual (route.GetDest ()))
                {
                  NetlinkMessage nlmsg_route;
                  NetlinkMessageHeader nhr = NetlinkMessageHeader (NETLINK_RTM_NEWROUTE, 0,
                                                                   nlmsg.GetHeader ().GetMsgSeq (), m_kernelPid);
                  RouteMessage rtmsg;

                  //fill rtmsg and attributes
                  rtmsg.SetFamily (AF_INET6);
                  rtmsg.SetDstLength (32);
                  rtmsg.SetSrcLength (0);
                  rtmsg.SetTos (0); //not clear
                  rtmsg.SetTableId (RouteMessage::RT_TABLE_MAIN);
                  rtmsg.SetScope (RouteMessage::RT_SCOPE_UNIVERSE);
                  rtmsg.SetProtocol (RouteMessage::RT_PROT_UNSPEC);
                  rtmsg.SetFlags (RouteMessage::RT_F_CLONED);

                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_DST, ADDRESS, route.GetDest ()));
                  //ns3 use local address as the route src address
                  // rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_SRC, ADDRESS, ipv6->GetSourceAddress(route.GetDest ())));
                  // rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_PREFSRC, ADDRESS, ipv6->GetSourceAddress(route.GetDest ())));
                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_IIF, U32, route.GetInterface ()));
                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_OIF, U32, route.GetInterface ()));
                  rtmsg.AppendAttribute (NetlinkAttribute (RouteMessage::RT_A_GATEWAY, ADDRESS, route.GetGateway ()));

                  //fill an netlink message body
                  nlmsg_route.SetHeader (nhr);
                  nlmsg_route.SetRouteMessage (rtmsg);

                  SendMessageUnicast (nlmsg_route, 1);
                  get_flag = 1;
                }
            }
        }

      if (!get_flag)
        {
          NS_LOG_INFO ("no route entry exist by dest address" << dest);
          m_errno = ERROR_INVAL;
          return -1;
        }
    }

  //then send an broadcast message, let all user know this operation happened
  MultipartNetlinkMessage nlmsg_multi;
  NetlinkMessage nlmsg_broadcast = nlmsg;
  NetlinkMessage nlmsg_done;
  NetlinkMessageHeader nhr_done = NetlinkMessageHeader (NETLINK_MSG_DONE, NETLINK_MSG_F_MULTI, 0, 0);
  nlmsg_done.SetHeader (nhr_done);
  nlmsg_multi.AppendMessage (nlmsg);
  nlmsg_multi.AppendMessage (nlmsg_done);
  SendMessageBroadcast (nlmsg_multi, NETLINK_RTM_GRP_IPV4_ROUTE, GetNode ());
  return 0;
}

int32_t
NetlinkSocket::NotifyIfLinkMessage (uint32_t interface_num)
{
  if (m_ipv4Routing == 0)
    {
      NS_LOG_ERROR ("No Ipv4 routing set");
      return -1;                     //should be some nicer error code
    }

  NS_LOG_FUNCTION (this << interface_num);

  /////////////////////////////////////////////////////////
  //// WARNING: SEQUENCE NUMBER IS WRONG HERE !!!!
  /////////////////////////////////////////////////////////
  NS_LOG_ERROR ("Sequence number might be wrong here");
  MultipartNetlinkMessage nlmsg_multi;
  nlmsg_multi.AppendMessage (BuildInterfaceInfoDumpMessage (interface_num, 0));

  //then append netlink message with type NLMSG_DONE
  NetlinkMessage nlmsg_done;
  nlmsg_done.SetHeader (NetlinkMessageHeader (NETLINK_MSG_DONE, NETLINK_MSG_F_MULTI,
                                              0, m_kernelPid));
  //kernel append nlmsg_dump size to it, here we omit it
  nlmsg_multi.AppendMessage (nlmsg_done);

  int32_t err = SendMessageBroadcast (nlmsg_multi, NETLINK_RTM_GRP_LINK, GetNode ());
  return err;
}

// int32_t
// NetlinkSocket::NotifyIfAddrMessage (Ipv6Interface* interface, Ipv6Address addr, int cmd)
// {
//   MultipartNetlinkMessage nlmsg_multi;
//   NetlinkMessage nlmsg_ifa;
//   NetlinkMessageHeader nhr = NetlinkMessageHeader (cmd, NETLINK_MSG_F_MULTI, 0, 0);
//   InterfaceAddressMessage ifamsg;

//   NS_ASSERT_MSG (false, "Not implemented yet (NotifyIfAddrMessage)");

//   // FIXME!
//   Ipv6Prefix prefix = Ipv6Prefix(64);

//   //here get the address mask length
//   uint8_t bytes[16];
//   prefix.GetBytes (bytes);
//   uint8_t mask_len = 0;
//   for (int j = 0; j < 16; j++)
//     {
//       while (bytes[j])
//         {
//           bytes[j] = bytes[j] << 1;
//           mask_len ++;
//         }
//     }

//   ifamsg.SetInterfaceIndex (interface->GetDevice ()->GetIfIndex ());
//   ifamsg.SetFamily (AF_INET6);
//   ifamsg.SetLength (mask_len);
//   ifamsg.SetFlags (0);
//   ifamsg.SetScope (RouteMessage::RT_SCOPE_UNIVERSE);

//   ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LOCAL,    ADDRESS, addr));
//   ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_ADDRESS,  ADDRESS, addr));
//   //  ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_BROADCAST,ADDRESS, bcast));
//   //      ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_LABEL,    STRING,  "ns3-ifaddr"));//not used in ns3
//   //ifamsg.AppendAttribute (NetlinkAttribute (InterfaceAddressMessage::IF_A_ANYCAST,  ADDRESS, Ipv4Address("0.0.0.0")));//not used in ns3
//   //XXXother attributes not used by ns3

//   nlmsg_ifa.SetHeader (nhr);
//   nlmsg_ifa.SetInterfaceAddressMessage (ifamsg);

//   NetlinkMessage nlmsg_done;
//   NetlinkMessageHeader nhr_done = NetlinkMessageHeader (NETLINK_MSG_DONE, NETLINK_MSG_F_MULTI, 0, 0);
//   nlmsg_done.SetHeader (nhr_done);

//   nlmsg_multi.AppendMessage (nlmsg_ifa);
//   nlmsg_multi.AppendMessage (nlmsg_done);

//   SendMessageBroadcast (nlmsg_multi, 0, RTMGRP_IPV6_IFADDR, interface->GetDevice ()->GetNode ());
//   return 0;
// }

#ifdef FIXME
int32_t
NetlinkSocket::NotifyRouteMessage (Ojbect route, uint16_t type, uint8_t family)
{
  NetlinkMessage nlmsg_broadcast = nlmsg;
  NetlinkMessageHeader nhr;
  NS_ASSERT_MSG (false, "Not implemented yet");

  nhr.SetMsgLen (nlmsg.GetHeader ().GetMsgLen ());
  nhr.SetMsgType (nlmsg.GetHeader ().GetMsgType ());
  nlmsg_broadcast.SetHeader (nhr);
  SendMessageBroadcast (nlmsg_broadcast, 0, RTMGRP_IPV6_ROUTE);
  return 0;
}
#endif

} //namespace ns3
