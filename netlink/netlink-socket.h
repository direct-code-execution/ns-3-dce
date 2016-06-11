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
#ifndef NETLINK_SOCKET_H
#define NETLINK_SOCKET_H

#include <stdint.h>
#include <queue>
#include "netlink-message.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface.h"

namespace ns3 {

class Node;
class Packet;
class NetlinkSocketAddress;
class Ipv4DceRouting;

/**
* \brief A NetlinkSocket is  used  to transfer information
between kernel and userspace processes .
*
* here we focus on NETLINK_ROUTE: Receives routing and link
* updates and may be used to modify  the  routing  tables
* (both IPv4 and IPv6), IP addresses, link parame- ters, neighbor
* setups, queueing disciplines, traffic classes and packet
* classifiers (see rtnetlink (7)). This socket type is very similar
* to the linux and BSD "packet" sockets.
*
* Here is a summary of the semantics of this class:
* - Bind: Bind uses only the protocol and device fields of the
*       NetlinkSocketAddress.
*
* - Send: send the input packet to the underlying kernel space
*       with its own address. The socket must  be bound.
*
* - Recv: receive packet from the kernel space.
*
* - Accept: not allowed
* - Connect: not allowed
*/
class NetlinkSocket : public Socket
{
public:
  static TypeId GetTypeId (void);

  NetlinkSocket ();
  virtual ~NetlinkSocket ();

  void SetNode (Ptr<Node> node);

  virtual enum SocketErrno GetErrno (void) const;
  virtual enum Socket::SocketType GetSocketType (void) const;
  virtual Ptr<Node> GetNode (void) const;
  virtual int Bind (void);
  virtual int Bind (const Address & address);
  virtual int Bind6 ();
  virtual int Close (void);
  virtual int ShutdownSend (void);
  virtual int ShutdownRecv (void);
  virtual int Connect (const Address &address);
  virtual int Listen (void);
  virtual uint32_t GetTxAvailable (void) const;
  virtual int Send (Ptr<Packet> p, uint32_t flags);
  virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress);
  virtual uint32_t GetRxAvailable (void) const;
  virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags);
  virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags,
                                Address &fromAddress);
  virtual int GetSockName (Address &address) const;
  virtual int GetPeerName (Address &address) const;
  virtual bool SetAllowBroadcast (bool allowBroadcast);
  virtual bool GetAllowBroadcast () const;

  uint32_t GetPid (void) const;
  uint32_t GetGroups (void) const;

  static const uint32_t m_kernelPid = 0; // NS3 pretends to be kernel

  /**
   * \brief Notify subscribed applications about interface info change
   *
   * This notification is send upon Up and Down calls to Ipv4 protocol
   * stack. What it does is simply dumping on the wire the current
   * interface info with or without UP & RUNNING flags.
   *
   * \warn Seq number sent in this case might be wrong
   */
  int32_t NotifyIfLinkMessage (uint32_t interface_num);

  /**
   * \brief Notify subscribed applications about interface address
   * changes (new IP address or removal of IP address)
   */
  // int32_t NotifyIfAddrMessage (uint32_t interface_num);

  //  int32_t NotifyRouteMessage(Ojbect route, uint16_t type, uint8_t family);

private:
  /**
   * DoBind should assign a port id equal to the processus id, except if that processus
   * opens several sockets in which case it should be increased
   **/
  int DoBind (const NetlinkSocketAddress &address);
  virtual void DoDispose (void);
  void ForwardUp (Ptr<Packet> p, const NetlinkSocketAddress &address);



  /**
 * the functions below were for kernel parsing netlink message,set private here
 * when netlink msg sent to kernel through netlink socket, it was parsed in kernel
 * space, then, kernel add/del its route/interface/link table or dump all information
 * to user space
 */

  int32_t HandleMessage (const NetlinkMessage &nlmsg);
  /**
  * when kernel find the message truncated or user need an ACK response,
  * it send ACK back to user space, with the error code(0 for ACK, > 0 for error).
  */
  void SendAckMessage (const NetlinkMessage &nlmsg, int32_t errorcode);

  /**
  * \brief unicast an message to user
  * \param nlmsg the netlink message to transmit
  * \param nonbloack always true
  */
  int32_t SendMessageUnicast (const MultipartNetlinkMessage &nlmsg,
                              int32_t nonblock);
  /**
  * \brief spread message to netlink group user
  * \param nlmsg the netlink message to transmit
  * \param group multicast group id
  * \param node Node
  */
  static int32_t SendMessageBroadcast (const MultipartNetlinkMessage &nlmsg,
                                       uint32_t group, Ptr<Node> node);

  /**
   * these functions below are for NETLINK_ROUTE protocol, it handle the netlink
   * message like linux kernel work.  this implementation follows the kernel code
   * linux/rtnetlink.c, focus on "interface address, interface info and route entry",
   * now we will only simply support three types operations of  NETLINK_ROUTE
   * protocol
   */

  /**
   * \returns 0 if message not processed, < 0 for success or an error.
   */
  int32_t HandleNetlinkRouteMessage (const NetlinkMessage &nlmsg);

  /**
   * \todo see below
   * \param type Should be of type NetlinkRtmType_e
   * \param family
   * \returns 0 if dumping operation is OK, < 0 for an error.
   */
  int32_t
  DumpNetlinkRouteMessage (const NetlinkMessage &nlmsg,
                           uint16_t type, uint8_t family);

  MultipartNetlinkMessage
  BuildInterfaceAddressDumpMessages (uint32_t received_seq);

  /**
   * \brief Build an InterfaceInfo message corresponding to n-th interface
   * \param interface_id n-th interface
   */
  NetlinkMessage
  BuildInterfaceInfoDumpMessage (uint32_t interface_id, uint32_t seq);

  /**
   * \brief Build a multipart netlink message consisting of several
   * (possibly zero) InterfaceInfo dump messages
   */
  MultipartNetlinkMessage
  BuildInterfaceInfoDumpMessages (uint32_t seq);

  MultipartNetlinkMessage
  BuildRouteDumpMessages (uint32_t seq);

  /**
  * \returns 0 if doing operation(ADD/DEL/GET) is OK, < 0 for an error.
  */
  int32_t DoNetlinkRouteMessage (const NetlinkMessage &nlmsg,
                                 uint16_t type, uint8_t family);
  int32_t DoInterfaceAddressMessage (const NetlinkMessage &nlmsg,
                                     uint16_t type, uint8_t family);
  int32_t DoInterfaceInfoMessage (const NetlinkMessage &nlmsg,
                                  uint16_t type, uint8_t family);
  int32_t DoRouteMessage (const NetlinkMessage &nlmsg,
                          uint16_t type, uint8_t family);

  int ErrnoToSimuErrno (void);
  Address ConvertFrom (uint8_t family, const Address &address);

  Ptr<Node> m_node;
  Ptr<Ipv4DceRouting> m_ipv4Routing;
  // Ptr<Ipv6DceRouting> m_ipv6Routing; //not implemented yet

  enum SocketErrno m_errno;
  bool m_shutdownSend;
  bool m_shutdownRecv;

  std::queue<std::pair <Ptr<Packet>, Address> > m_dataReceiveQueue;
  uint32_t m_rxAvailable;
  TracedCallback<Ptr<const Packet> > m_dropTrace;
  // Socket options (attributes)
  uint32_t m_rcvBufSize;

  uint32_t m_Pid;     /**< port id of the socket */
  uint32_t m_Groups;
  Callback<void, Ipv4Address,uint8_t,uint8_t,uint8_t,uint32_t> m_icmpCallback;

  TracedCallback<const Header&, Ptr<const Packet> > m_promiscSnifferTrace;
};

} //namespace ns3

#endif /* NETLINK_SOCKET_H */


