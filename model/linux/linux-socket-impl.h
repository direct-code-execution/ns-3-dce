/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 NICT
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
 * Author: Hajime Tazaki <tazaki@nict.go.jp>
 */
#ifndef UDP_SOCKET_IMPL_H
#define UDP_SOCKET_IMPL_H

#include "ns3/socket.h"
#include "ns3/event-id.h"
#include <sys/socket.h>

namespace ns3 {

class Node;
class Packet;
class Task;
class KernelSocketFd;

class LinuxSocketImpl : public Socket
{
public:
  static TypeId GetTypeId (void);
  /**
   * Create an unbound udp socket.
   */
  LinuxSocketImpl ();
  virtual ~LinuxSocketImpl ();
  void CreateSocket ();

  virtual enum SocketErrno GetErrno (void) const;
  virtual enum SocketType GetSocketType (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual Ptr<Node> GetNode (void) const;
  virtual int Bind (void);
  virtual int Bind6 (void);
  virtual int Bind (const Address &address);
  virtual int Close (void);
  virtual int ShutdownSend (void);
  virtual int ShutdownRecv (void);
  virtual int Connect (const Address &address);
  virtual int Listen (void);
  virtual uint32_t GetTxAvailable (void) const;
  virtual int Send (Ptr<Packet> p, uint32_t flags);
  virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &address);
  virtual uint32_t GetRxAvailable (void) const;
  virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags);
  virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags,
                                Address &fromAddress);
  virtual int GetSockName (Address &address) const;
  virtual int MulticastJoinGroup (uint32_t interfaceIndex, const Address &groupAddress);
  virtual int MulticastLeaveGroup (uint32_t interfaceIndex, const Address &groupAddress);
  virtual void BindToNetDevice (Ptr<NetDevice> netdevice);
  virtual bool SetAllowBroadcast (bool allowBroadcast);
  virtual bool GetAllowBroadcast () const;
  void Poll ();

  void Setsockopt (int level, int optname,
                   const void *optval, socklen_t optlen);
  int Getsockopt (int level, int optname,
                void *optval, socklen_t *optlen);

  Address PosixAddressToNs3Address (const struct sockaddr *my_addr, socklen_t addrlen);
  int Ns3AddressToPosixAddress (const Address& nsaddr,
                                struct sockaddr *addr, socklen_t *addrlen);

  typedef Callback<int,const Address&,struct sockaddr *,socklen_t *> Ns3ToPosixConverter;
  typedef Callback<Address,const struct sockaddr *,socklen_t> PosixToNs3Converter;

  void SetNs3ToPosixConverter (LinuxSocketImpl::Ns3ToPosixConverter cb);
  void SetPosixToNs3Converter (LinuxSocketImpl::PosixToNs3Converter cb);

  KernelSocketFd *m_kernsock;
  Task *m_task;
  Ns3ToPosixConverter m_ns3toposix;
  PosixToNs3Converter m_posixtons3;

private:
  // Attributes set through UdpSocket base class
  virtual void SetRcvBufSize (uint32_t size);
  virtual uint32_t GetRcvBufSize (void) const;
  virtual void SetIpTtl (uint8_t ipTtl);
  virtual uint8_t GetIpTtl (void) const;
  virtual void SetIpMulticastTtl (uint8_t ipTtl);
  virtual uint8_t GetIpMulticastTtl (void) const;
  virtual void SetIpMulticastIf (int32_t ipIf);
  virtual int32_t GetIpMulticastIf (void) const;
  virtual void SetIpMulticastLoop (bool loop);
  virtual bool GetIpMulticastLoop (void) const;
  virtual void SetMtuDiscover (bool discover);
  virtual bool GetMtuDiscover (void) const;
  uint16_t EnterFakeTask ();
  void LeaveFakeTask (uint16_t pid);
  void ScheduleTask (EventImpl *event);
  static void ScheduleTaskTrampoline (void *context);

  enum SocketErrno m_errno;
  Ptr<Node> m_node;
  uint16_t m_family;
  uint16_t m_socktype;
  uint16_t m_protocol;
  bool m_listening;
  bool m_conn_inprogress;
  uint16_t m_pid;
  EventId m_poll;

};

} // namespace ns3

#endif /* UDP_SOCKET_IMPL_H */
