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

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/event-id.h"
#include "linux-socket-fd-factory.h"
#include "kernel-socket-fd.h"
#include "linux-socket-impl.h"
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include "file-usage.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"

NS_LOG_COMPONENT_DEFINE ("DceLinuxSocketImpl");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LinuxSocketImpl);

Address
LinuxSocketImpl::PosixAddressToNs3Address (const struct sockaddr *my_addr, socklen_t addrlen)
{
  if (my_addr->sa_family == AF_INET)
    {
      const struct sockaddr_in *addr = (const struct sockaddr_in *)my_addr;
      Ipv4Address ipv4;
      ipv4.Set (ntohl (addr->sin_addr.s_addr));
      uint16_t port = ntohs (addr->sin_port);
      InetSocketAddress inet = InetSocketAddress (ipv4, port);
      return inet;
    }
  else if (my_addr->sa_family == AF_INET6)
    {
      const struct sockaddr_in6 *addr = (const struct sockaddr_in6 *)my_addr;
      Ipv6Address ipv6;
      ipv6.Set ((uint8_t *)addr->sin6_addr.s6_addr);
      uint16_t port = ntohs (addr->sin6_port);
      Inet6SocketAddress inet = Inet6SocketAddress (ipv6, port);
      return inet;
    }
  else
    {
      NS_LOG_WARN ("Unsupported address familiy");
    }
  return Address ();
}

int
LinuxSocketImpl::Ns3AddressToPosixAddress (const Address& nsaddr,
                                           struct sockaddr *addr, socklen_t *addrlen)
{
  if (addr == 0 || addrlen == 0)
    {
      return 0;
    }
  if (InetSocketAddress::IsMatchingType (nsaddr))
    {
      InetSocketAddress ns_inetaddr = InetSocketAddress::ConvertFrom (nsaddr);
      if (*addrlen < sizeof (struct sockaddr_in))
        {
          return -1;
        }
      struct sockaddr_in *inet_addr = (struct sockaddr_in *)addr;
      memset (inet_addr, 0, sizeof (struct sockaddr_in));
      inet_addr->sin_family = AF_INET;
      inet_addr->sin_port = htons (ns_inetaddr.GetPort ());
      inet_addr->sin_addr.s_addr = htonl (ns_inetaddr.GetIpv4 ().Get ());
      *addrlen = sizeof(struct sockaddr_in);
    }
  else if (Inet6SocketAddress::IsMatchingType (nsaddr))
    {
      Inet6SocketAddress ns_inetaddr = Inet6SocketAddress::ConvertFrom (nsaddr);
      if (*addrlen < sizeof (struct sockaddr_in6))
        {
          return -1;
        }
      struct sockaddr_in6 *inet_addr = (struct sockaddr_in6 *)addr;
      inet_addr->sin6_family = AF_INET6;
      inet_addr->sin6_port = htons (ns_inetaddr.GetPort ());
      ns_inetaddr.GetIpv6 ().GetBytes (inet_addr->sin6_addr.s6_addr);
      *addrlen = sizeof(struct sockaddr_in6);
    }
  else
    {
      NS_LOG_WARN ("Unsupported address familiy");
    }
  return 0;
}

// Add attributes generic to all Sockets to base class Socket
TypeId
LinuxSocketImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinuxSocketImpl")
    .SetParent<Socket> ()
    .AddConstructor<LinuxSocketImpl> ()
    .AddAttribute ("Family", "Protocol family.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&LinuxSocketImpl::m_family),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SockType", "Socket type.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&LinuxSocketImpl::m_socktype),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Protocol", "Protocol number to match.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&LinuxSocketImpl::m_protocol),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

LinuxSocketImpl::LinuxSocketImpl ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_listening = false;
  m_conn_inprogress = false;
  m_pid = -1;
  SetNs3ToPosixConverter (MakeCallback (&LinuxSocketImpl::Ns3AddressToPosixAddress, this));
  SetPosixToNs3Converter (MakeCallback (&LinuxSocketImpl::PosixAddressToNs3Address, this));
}

LinuxSocketImpl::~LinuxSocketImpl ()
{
  if (m_poll.IsRunning ())
    {
      m_poll.Cancel ();
    }
  Ptr<Node> node = GetNode ();
  Ptr<DceManager> manager = node->GetObject<DceManager> ();
}

void
LinuxSocketImpl::ScheduleTaskTrampoline (void *context)
{
  Task *current = TaskManager::Current ()->CurrentTask ();
  LinuxSocketImpl *self = (LinuxSocketImpl *) current->GetExtraContext ();
  current->SetExtraContext (0);
  EventImpl *event = (EventImpl *)context;
  event->Invoke ();
  event->Unref ();
  TaskManager::Current ()->Exit ();
}

void
LinuxSocketImpl::ScheduleTask (EventImpl *event)
{
  Ptr<Node> node = GetNode ();
  Ptr<DceManager> manager = node->GetObject<DceManager> ();
  Ptr<TaskManager> taskManager = this->GetObject<TaskManager> ();
  std::vector<std::string> nullargs;
  std::vector<std::pair<std::string,std::string> > envs;
  struct Process *process = manager->CreateProcess ("dummy-task", "dummy-stdin", nullargs, envs, 0);
  struct Thread *thread = manager->CreateThread (process);
  Task *task = TaskManager::Current ()->Start (&LinuxSocketImpl::ScheduleTaskTrampoline,
                                               event, 1 << 17);
  task->SetContext (thread);
  thread->task = task;
  task->SetExtraContext (this);
}

enum Socket::SocketErrno
LinuxSocketImpl::GetErrno (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_errno;
}

enum Socket::SocketType
LinuxSocketImpl::GetSocketType (void) const
{
  switch (m_socktype)
    {
    case SOCK_STREAM:
    case SOCK_DCCP:
      {
        return NS3_SOCK_STREAM;
        break;
      }
    case SOCK_DGRAM:
      {
        return NS3_SOCK_DGRAM;
        break;
      }
    case SOCK_RAW:
      {
        return NS3_SOCK_RAW;
        break;
      }
    default:
        break;
    }

    // To remove compiler warning
    return NS3_SOCK_RAW;
}

uint16_t
LinuxSocketImpl::EnterFakeTask ()
{
  NS_LOG_FUNCTION (this << m_pid);
  Ptr<Node> node = GetNode ();
  Ptr<DceManager> manager = node->GetObject<DceManager> ();
  if (m_pid == 65535)
    {
      m_pid = manager->StartTemporaryTask ();
    }
  else
    {
      manager->ResumeTemporaryTask (m_pid);
    }
  return m_pid;
}

void
LinuxSocketImpl::LeaveFakeTask (uint16_t pid)
{
  NS_LOG_FUNCTION (this << m_pid << pid);
  Ptr<Node> node = GetNode ();
  Ptr<DceManager> manager = node->GetObject<DceManager> ();
  manager->SuspendTemporaryTask (pid);
  return;
}


int
LinuxSocketImpl::Bind (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Bind (NULL, 0);
  LeaveFakeTask (pid);
  return ret;
}

int
LinuxSocketImpl::Bind6 (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Bind (NULL, 0);
  LeaveFakeTask (pid);
  return ret;
}

int
LinuxSocketImpl::Bind (const Address &address)
{
  NS_LOG_FUNCTION (this << address);
  uint16_t pid = EnterFakeTask ();
  struct sockaddr_storage my_addr;
  socklen_t addrlen = sizeof (my_addr);
  m_ns3toposix (address, (struct sockaddr *)&my_addr, &addrlen);

  int ret = this->m_kernsock->Bind ((struct sockaddr *)&my_addr, addrlen);
  LeaveFakeTask (pid);
  return ret;
}

int
LinuxSocketImpl::ShutdownSend (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return 0;
}

int
LinuxSocketImpl::ShutdownRecv (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return 0;
}

int
LinuxSocketImpl::Close (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Close ();
  LeaveFakeTask (pid);
  return ret;
}

int
LinuxSocketImpl::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  uint16_t pid = EnterFakeTask ();
  struct sockaddr_storage my_addr;
  socklen_t addrlen = sizeof (my_addr);
  m_ns3toposix (address, (struct sockaddr *)&my_addr, &addrlen);

  int ret = this->m_kernsock->Connect ((struct sockaddr *)&my_addr, addrlen);
  NS_LOG_INFO ("connect returns " << ret << " errno " << Current ()->err);
  if (ret == -1 && Current ()->err == EINPROGRESS)
    {
      m_conn_inprogress = true;
    }
  LeaveFakeTask (pid);
  return ret;
}

int
LinuxSocketImpl::Listen (void)
{
  NS_LOG_FUNCTION (this);
  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Listen (5);
  NS_LOG_DEBUG ("listen returns " << ret << " errno " << Current ()->err);
  if (ret == 0)
    {
      m_listening = true;
    }
  LeaveFakeTask (pid);
  return ret;
}

int
LinuxSocketImpl::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p << flags);
  uint16_t pid = EnterFakeTask ();
  uint8_t *buf = new uint8_t[p->GetSize ()];
  p->CopyData (buf, p->GetSize ());
  int len = p->GetSize ();

  int ret = this->m_kernsock->Write (buf, len);
  if (ret < 0)
    {
      NS_LOG_INFO ("send returns " << ret << " errno " << Current ()->err);
    }
  delete[] buf;
  LeaveFakeTask (pid);
  return ret;
}


// XXX maximum message size for UDP broadcast is limited by MTU
// size of underlying link; we are not checking that now.
uint32_t
LinuxSocketImpl::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // No finite send buffer is modelled, but we must respect
  // the maximum size of an IP datagram (65535 bytes - headers).
  return -1;
}

int
LinuxSocketImpl::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
  NS_LOG_FUNCTION (this << p << flags << address);
  uint8_t *buf = new uint8_t[p->GetSize ()];
  p->CopyData (buf, p->GetSize ());
  int len = p->GetSize ();
  struct sockaddr_storage my_addr;
  socklen_t addrlen = sizeof (my_addr);
  m_ns3toposix (address, (struct sockaddr *)&my_addr, &addrlen);

  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = len;
  iov.iov_base = (void *)buf;
  msg.msg_name = (void *)&my_addr;
  msg.msg_namelen = addrlen;

  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Sendmsg (&msg, flags);
  if (ret < 0)
    {
      NS_LOG_INFO ("sendmsg returns " << ret << " errno " << Current ()->err);
    }
  delete[] buf;
  LeaveFakeTask (pid);
  return ret;
}

uint32_t
LinuxSocketImpl::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // We separately maintain this state to avoid walking the queue
  // every time this might be called
  return -1;
}

Ptr<Packet>
LinuxSocketImpl::Recv (uint32_t maxSize, uint32_t flags)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  return 0;
}

Ptr<Packet>
LinuxSocketImpl::RecvFrom (uint32_t maxSize, uint32_t flags,
                           Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  uint8_t buf[4096];
  struct sockaddr_storage my_addr;
  socklen_t addrlen = sizeof (struct sockaddr_storage);
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = sizeof (buf);
  iov.iov_base = buf;
  msg.msg_name = (void *)&my_addr;
  msg.msg_namelen = addrlen;

  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Recvmsg (&msg, flags);
  if (ret < 0)
    {
      NS_LOG_DEBUG ("recvmsg ret " << ret << " errno " << Current ()->err);
      LeaveFakeTask (pid);
      return 0;
    }
  LeaveFakeTask (pid);

  Ptr<Packet> p = Create<Packet> (buf, ret);
  pid = EnterFakeTask ();
  ret = this->m_kernsock->Getpeername ((struct sockaddr *)&my_addr, &addrlen);
  if (ret < 0)
    {
      NS_LOG_DEBUG ("getpeername ret " << ret);
    }
  LeaveFakeTask (pid);
  fromAddress = m_posixtons3 ((struct sockaddr *)&my_addr, addrlen);

  return p;
}

int
LinuxSocketImpl::GetSockName (Address &address) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return -1;
}

int
LinuxSocketImpl::GetPeerName (Address &address) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return -1;
}

int
LinuxSocketImpl::MulticastJoinGroup (uint32_t interface, const Address &groupAddress)
{
  NS_LOG_FUNCTION (interface << groupAddress);
  /*
   1) sanity check interface
   2) sanity check that it has not been called yet on this interface/group
   3) determine address family of groupAddress
   4) locally store a list of (interface, groupAddress)
   5) call ipv4->MulticastJoinGroup () or Ipv6->MulticastJoinGroup ()
  */
  return 0;
}

int
LinuxSocketImpl::MulticastLeaveGroup (uint32_t interface, const Address &groupAddress)
{
  NS_LOG_FUNCTION (interface << groupAddress);
  /*
   1) sanity check interface
   2) determine address family of groupAddress
   3) delete from local list of (interface, groupAddress); raise a LOG_WARN
      if not already present (but return 0)
   5) call ipv4->MulticastLeaveGroup () or Ipv6->MulticastLeaveGroup ()
  */
  return 0;
}

void
LinuxSocketImpl::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (netdevice->GetIfIndex ());
  std::ostringstream oss;
  oss << "sim" << netdevice->GetIfIndex ();
  struct ifreq ifr;
  strncpy ((char *)&ifr.ifr_name, oss.str ().c_str (), sizeof (ifr.ifr_name));

  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Setsockopt (SOL_SOCKET, SO_BINDTODEVICE,
                                          &ifr, sizeof (ifr));
  NS_LOG_INFO ("sockopt returns " << ret << " errno " << Current ()->err);
  LeaveFakeTask (pid);
  return;
}

void
LinuxSocketImpl::Setsockopt (int level, int optname,
                             const void *optval, socklen_t optlen)
{
  NS_LOG_FUNCTION (level << optname << optval << optlen);
  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Setsockopt (level, optname,
                                          optval, optlen);
  NS_LOG_INFO ("setsockopt returns " << ret << " errno " << Current ()->err);
  LeaveFakeTask (pid);
  return;
}
int
LinuxSocketImpl::Getsockopt (int level, int optname,
                             void *optval, socklen_t *optlen)
{
  NS_LOG_FUNCTION (level << optname << optval << optlen);
  uint16_t pid = EnterFakeTask ();
  int ret = this->m_kernsock->Getsockopt (level, optname,
                                          optval, optlen);
  NS_LOG_INFO ("getsockopt returns " << ret << " errno " << Current ()->err);
  LeaveFakeTask (pid);
  return ret;
}

void
LinuxSocketImpl::Poll ()
{
  NS_LOG_FUNCTION (this);
  PollTable *table = new PollTable ();
  table->SetEventMask (POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDHUP);
  PollTable *pwait = table;

  // Infinite loop
  while (true)
    {
      uint16_t pid = EnterFakeTask ();
      int mask = this->m_kernsock->Poll (pwait);
      if (mask < 0)
        {
          NS_LOG_INFO ("poll returns " << mask << " errno " << Current ()->err);
          continue;
        }
      LeaveFakeTask (pid);

      // Notify the data
      mask &= (POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDHUP);
      if (mask)
        {
          Ptr<LinuxSocketFdFactory> factory = 0;
          Ptr<Node> node = GetNode ();
          Ptr<DceManager> manager = node->GetObject<DceManager> ();
          factory = manager->GetObject<LinuxSocketFdFactory> ();
          if (m_listening)
            {
              struct sockaddr_storage my_addr;
              socklen_t addrlen = sizeof (struct sockaddr_storage);
              Address fromAddress;

              NS_LOG_INFO ("notify accept");
              Ptr<LinuxSocketImpl> newSock = CreateObject<LinuxSocketImpl> ();
              newSock->SetNode (m_node);
              newSock->m_family = m_family;
              newSock->m_socktype = m_socktype;
              newSock->m_protocol = m_protocol;

              pid = EnterFakeTask ();
              int sock = this->m_kernsock->Accept ((struct sockaddr *)&my_addr, &addrlen);
              if (sock < 0)
                {
                  NS_LOG_INFO ("accept error");
                }
              KernelSocketFd *kern_sock;
              FileUsage *fu = Current ()->process->openFiles[sock];
              kern_sock = (KernelSocketFd *)fu->GetFileInc ();
              kern_sock->IncFdCount ();
              kern_sock->Fcntl (F_SETFL, O_NONBLOCK);

              newSock->m_kernsock = kern_sock;
              newSock->m_listening = false;
              fromAddress = m_posixtons3 ((struct sockaddr *)&my_addr, addrlen);
              LeaveFakeTask (pid);
              NotifyNewConnectionCreated (newSock, fromAddress);
              Simulator::ScheduleWithContext (m_node->GetId (), Seconds (0.0),
                                              &LinuxSocketImpl::ScheduleTask, newSock,
                                              MakeEvent (&LinuxSocketImpl::Poll, newSock));
            }
          else if (m_conn_inprogress)
            {
              NS_LOG_INFO ("notify conn inprogress finish");
              NotifyConnectionSucceeded ();
              m_conn_inprogress = false;

              NS_LOG_FUNCTION ("CALL>> Wait mask (after conn) " << mask);
              table->Wait (Seconds (0));
              NS_LOG_FUNCTION ("RTRN<< Wait mask  (after conn) " << mask);
            }
          else
            {
              // FIXME: handle closed socket
              if (mask & POLLRDHUP || mask & POLLHUP || mask & POLLERR)
                {
                  NS_LOG_FUNCTION ("socket has closed ?" << mask);
                  // FIXME: may need m_closed flag
                  // Close ();
                  // XXX:::...
                  Current ()->process->manager->Wait ();
                }

              else if (mask & POLLIN || mask & POLLERR)
                {
                  NS_LOG_INFO ("notify recv");
                  NotifyDataRecv ();
                }
              else if (mask & POLLOUT)
                {
                  Simulator::ScheduleWithContext (m_node->GetId (), Seconds (0.0),
                                                  MakeEvent (&LinuxSocketImpl::NotifySend, this, 0));
                  NS_LOG_INFO ("wait send for next poll event");
                  table->Wait (Seconds (0));
                  NS_LOG_INFO ("awaken");
                }
            }
        }
      // if not masked
      else
        {
          NS_LOG_INFO ("wait for next poll event");
          table->Wait (Seconds (0));
          NS_LOG_INFO ("awaken");
        }

      // next loop
      pwait = NULL;
    }
}

void
LinuxSocketImpl::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = node;
  return;
}

Ptr<Node>
LinuxSocketImpl::GetNode (void) const
{
  return m_node;
}

void
LinuxSocketImpl::SetNs3ToPosixConverter (LinuxSocketImpl::Ns3ToPosixConverter cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ns3toposix = cb;
  return;
}

void
LinuxSocketImpl::SetPosixToNs3Converter (LinuxSocketImpl::PosixToNs3Converter cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_posixtons3 = cb;
  return;
}

void
LinuxSocketImpl::CreateSocket ()
{
  NS_LOG_FUNCTION_NOARGS ();
  uint16_t pid = EnterFakeTask ();
  Ptr<LinuxSocketFdFactory> factory = m_node->GetObject<LinuxSocketFdFactory> ();
  KernelSocketFd *kern_sock = (KernelSocketFd *)
    factory->CreateSocket (m_family, m_socktype, m_protocol);
  LeaveFakeTask (pid);
  if (!kern_sock)
    {
      NS_ASSERT_MSG (0, "can't create socket: not enabled particular socket ? (af=" 
                     << m_family << ", socktype=" << m_socktype
                     << ", proto=" << m_protocol << ")");
      return;
    }
  kern_sock->Fcntl (F_SETFL, O_NONBLOCK);
  this->m_kernsock = kern_sock;

  // Poll to register recv callback
  Simulator::ScheduleWithContext (m_node->GetId (), Seconds (0.0),
                                  &LinuxSocketImpl::ScheduleTask, this,
                                  MakeEvent (&LinuxSocketImpl::Poll, this));
  return;
}

void
LinuxSocketImpl::SetRcvBufSize (uint32_t size)
{
}

uint32_t
LinuxSocketImpl::GetRcvBufSize (void) const
{
  return -1;
}

void
LinuxSocketImpl::SetIpTtl (uint8_t ipTtl)
{
}

uint8_t
LinuxSocketImpl::GetIpTtl (void) const
{
  return -1;
}

void
LinuxSocketImpl::SetIpMulticastTtl (uint8_t ipTtl)
{
}

uint8_t
LinuxSocketImpl::GetIpMulticastTtl (void) const
{
  return -1;
}

void
LinuxSocketImpl::SetIpMulticastIf (int32_t ipIf)
{
}

int32_t
LinuxSocketImpl::GetIpMulticastIf (void) const
{
  return -1;
}

void
LinuxSocketImpl::SetIpMulticastLoop (bool loop)
{
}

bool
LinuxSocketImpl::GetIpMulticastLoop (void) const
{
  return false;
}

void
LinuxSocketImpl::SetMtuDiscover (bool discover)
{
  // XXX
}
bool
LinuxSocketImpl::GetMtuDiscover (void) const
{
  return false;
}

bool
LinuxSocketImpl::SetAllowBroadcast (bool allowBroadcast)
{
  return false;
}

bool
LinuxSocketImpl::GetAllowBroadcast () const
{
  return false;
}


} // namespace ns3
