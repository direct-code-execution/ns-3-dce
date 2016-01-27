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
#include "unix-socket-fd.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "cmsg.h"
#include "sys/dce-socket.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/netlink-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include <fcntl.h>
#include <errno.h>
#include <linux/icmp.h> // need ICMP_FILTER
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <net/ethernet.h>
#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <poll.h>
#include <linux/netlink.h>
#include <sys/ioctl.h>

NS_LOG_COMPONENT_DEFINE ("DceUnixSocketFd");

namespace ns3 {

UnixSocketFd::UnixSocketFd (Ptr<Socket> socket)
  : m_socket (socket),
    m_sendTimeout (Seconds (0.0)),
    m_recvTimeout (Seconds (0.0)),
    m_peekedData (0)
{
  m_socket->SetRecvCallback (MakeCallback (&UnixSocketFd::RecvSocketData, this));
  m_socket->SetSendCallback (MakeCallback (&UnixSocketFd::SendSocketData, this));
}
UnixSocketFd::~UnixSocketFd ()
{
  NS_LOG_FUNCTION (this);
  ClearSocket ();
}
void *
UnixSocketFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << start << length << prot << flags << offset);
  NS_ASSERT (current != 0);
  current->err = ENODEV;
  return MAP_FAILED;
}
int
UnixSocketFd::Fxstat (int ver, struct stat *buf)
{
  NS_LOG_FUNCTION (this << Current () << buf);
  NS_ASSERT (Current () != 0);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
UnixSocketFd::Fxstat64 (int ver, struct stat64 *buf)
{
  NS_LOG_FUNCTION (this << Current () << buf);
  NS_ASSERT (Current () != 0);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
UnixSocketFd::ErrnoToSimuErrno (void) const
{
  switch (m_socket->GetErrno ())
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
      return EPIPE;
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
    case Socket::SOCKET_ERRNO_LAST:
    case Socket::ERROR_NOTERROR:
      return EAGAIN;
    case Socket::ERROR_ADDRINUSE:
      return EADDRINUSE;
    default:
      NS_LOG_ERROR ("Unknown Errno:" <<  m_socket->GetErrno ());
      NS_ASSERT (false);
      return 0; // quiet compiler
      break;
    }
}
void
UnixSocketFd::RecvSocketData (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << m_socket << socket);

  int pi = POLLIN;
  WakeWaiters (&pi);
}
void
UnixSocketFd::SendSocketData (Ptr<Socket> socket, uint32_t available)
{
  NS_LOG_FUNCTION (this << m_socket << socket);

  int pi = POLLOUT;
  WakeWaiters (&pi);
}
void
UnixSocketFd::MainClose (int *res)
{
  *res = m_socket->Close ();
}
int
UnixSocketFd::Close (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  Callback<void, Ptr<Socket> > nil = MakeNullCallback<void, Ptr<Socket> > ();
  m_socket->SetCloseCallbacks (nil, nil);
  TaskManager *manager = TaskManager::Current ();
  int result = -1;
  manager->ExecOnMain (MakeEvent (&UnixSocketFd::MainClose, this, &result));

  if (result == -1)
    {
      current->err = ErrnoToSimuErrno ();
    }
  ClearSocket ();
  /**
   * Closing a socket while another thread is doing blocking IO
   * on it at the same time is valid behavior according to POSIX.
   * The traditional UNIX semantics are to not wakeup the blocked
   * IO if it was a read but to wakeup blocked IO if it was a write.
   * So, m_readWaiter might well be != 0 here but we don't care.
   * Instead, we check wakeup writers
   */
  int pi = POLLHUP;
  WakeWaiters (&pi);
  return result;
}
ssize_t
UnixSocketFd::Write (const void *buf, size_t count)
{
  NS_LOG_FUNCTION (this << buf << count);
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = count;
  iov.iov_base = (void*)buf;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  ssize_t retval = Sendmsg (&msg, 0);
  return retval;
}
ssize_t
UnixSocketFd::Read (void *buf, size_t count)
{
  NS_LOG_FUNCTION (this << buf << count);
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = count;
  iov.iov_base = buf;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  ssize_t retval = Recvmsg (&msg, 0);
  return retval;
}
ssize_t
UnixSocketFd::Recvmsg (struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking ? MSG_DONTWAIT : 0;
  return DoRecvmsg (msg, flags);
}
ssize_t
UnixSocketFd::Sendmsg (const struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking ? MSG_DONTWAIT : 0;
  return DoSendmsg (msg, flags);
}

bool
UnixSocketFd::Isatty (void) const
{
  return false;
}

int
UnixSocketFd::Setsockopt (int level, int optname,
                          const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  switch (level)
    {
    case SOL_RAW:
      switch (optname)
        {
        case ICMP_FILTER:
          {
            if (optlen != 4)
              {
                current->err = EINVAL;
                return -1;
              }
            uint32_t *data = (uint32_t *)optval;
            if (!m_socket->SetAttributeFailSafe ("IcmpFilter", UintegerValue (*data)))
              {
                current->err = ENOPROTOOPT;
                return -1;
              }
          } break;
        default:
          NS_LOG_WARN ("Unsupported setsockopt requested. level: SOL_RAW, optname: " << optname);
          break;
        }
      break;
    case SOL_SOCKET:
      switch (optname)
        {
        case SO_RCVTIMEO:
          {
            if (optlen != sizeof (struct timeval))
              {
                current->err = EINVAL;
                return -1;
              }
            struct timeval *tv = (struct timeval *)optval;
            m_recvTimeout = UtilsTimevalToTime (*tv);
          } break;
        case SO_SNDTIMEO:
          {
            if (optlen != sizeof (struct timeval))
              {
                current->err = EINVAL;
                return -1;
              }
            struct timeval *tv = (struct timeval *)optval;
            m_sendTimeout = UtilsTimevalToTime (*tv);
          } break;
        case SO_SNDBUF:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *val = (int*)optval;
            if (!m_socket->SetAttributeFailSafe ("SndBufSize", UintegerValue (*val)))
              {
                current->err = EINVAL;
                return -1;
              }
          } break;
        case SO_RCVBUF:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *val = (int*)optval;
            if (!m_socket->SetAttributeFailSafe ("RcvBufSize", UintegerValue (*val)))
              {
                current->err = EINVAL;
                return -1;
              }
          } break;
        default:
          NS_LOG_WARN ("Unsupported setsockopt requested. level: SOL_SOCKET, optname: " << optname);
          break;
        }
      break;
    case SOL_IP:
      switch (optname)
        {
        case IP_RECVERR:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            m_recverr = *v;
          } break;
        case IP_RECVTTL:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            m_recvttl = *v;
          } break;
        case IP_TTL:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            if (!m_socket->SetAttributeFailSafe ("IpTtl", UintegerValue (*v)))
              {
                current->err = ENOPROTOOPT;
                return -1;
              }
          } break;
        case IP_HDRINCL:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            if (!m_socket->SetAttributeFailSafe ("IpHeaderInclude",
                                                 BooleanValue (*v ? true : false)))
              {
                current->err = ENOPROTOOPT;
                return -1;
              }
          } break;
        case IP_PKTINFO:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            m_socket->SetRecvPktInfo (*v ? true : false);
          } break;
        default:
          NS_LOG_WARN ("Unsupported setsockopt requested. level: SOL_IP, optname: " << optname);
          break;
        }
      break;
    case SOL_IPV6:
      switch (optname)
        {
        case IPV6_PKTINFO:
          {
            if (optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            m_socket->SetRecvPktInfo (*v ? true : false);
          } break;
        // case IPV6_RECVPKTINFO: {
        //   if (optlen != sizeof (int))
        //     {
        //       current->err = EINVAL;
        //       return -1;
        //     }
        //   int *v = (int*)optval;
        //   m_recvpktinfo6 = *v;
        // } break;
        // case IPV6_CHECKSUM: {
        //   if (optlen != sizeof (int))
        //     {
        //       current->err = EINVAL;
        //       return -1;
        //     }
        //   int *v = (int*)optval;
        //   m_sockchecksum = *v;
        // } break;
        // case IPV6_MULTICAST_LOOP: {
        //   if (optlen != sizeof (int))
        //     {
        //       current->err = EINVAL;
        //       return -1;
        //     }
        //   int *v = (int*)optval;
        //   m_multicastloop = *v;
        // } break;
        // case IPV6_UNICAST_HOPS: {
        //   if (optlen != sizeof (int))
        //     {
        //       current->err = EINVAL;
        //       return -1;
        //     }
        //   int *v = (int*)optval;
        //   m_unicasthops = *v;
        // } break;
        // case IPV6_MULTICAST_HOPS: {
        //   if (optlen != sizeof (int))
        //     {
        //       current->err = EINVAL;
        //       return -1;
        //     }
        //   int *v = (int*)optval;
        //   m_multicasthops = *v;
        // } break;
        // case IPV6_RECVHOPLIMIT: {
        //   if (optlen != sizeof (int))
        //     {
        //       current->err = EINVAL;
        //       return -1;
        //     }
        //   int *v = (int*)optval;
        //   m_recvhoplimit = *v;
        // } break;
        default:
          NS_LOG_WARN ("Unsupported setsockopt requested. level: SOL_IPV6, optname: " << optname);
          break;
        }
      break;
    default:
      {
        NS_LOG_WARN ("Unsupported sockopt: level = " << level);
        current->err = ENOPROTOOPT;
        return -1;
      } break;
    }
  return 0;
}
int
UnixSocketFd::Getsockopt (int level, int optname,
                          void *optval, socklen_t *optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  switch (level)
    {
    case SOL_RAW:
      switch (optname)
        {
        case ICMP_FILTER:
          {
            if (*optlen < 4)
              {
                current->err = EINVAL;
                return -1;
              }
            UintegerValue data;
            if (!m_socket->GetAttributeFailSafe ("IcmpFilter", data))
              {
                current->err = ENOPROTOOPT;
                return -1;
              }
            uint32_t v = data.Get ();
            memcpy (optval, (void*)&v, 4);
            *optlen = 4;
          } break;
        default:
          NS_LOG_WARN ("Unsupported setsockopt requested. level: SOL_RAW, optname: " << optname);
          break;
        }
      break;
    case SOL_SOCKET:
      switch (optname)
        {
        case SO_RCVTIMEO:
          {
            if (*optlen < sizeof (struct timeval))
              {
                current->err = EINVAL;
                return -1;
              }
            struct timeval *tv = (struct timeval *)optval;
            *tv = UtilsTimeToTimeval (m_recvTimeout);
            *optlen = sizeof (struct timeval);
          } break;
        case SO_SNDTIMEO:
          {
            if (*optlen < sizeof (struct timeval))
              {
                current->err = EINVAL;
                return -1;
              }
            struct timeval *tv = (struct timeval *)optval;
            *tv = UtilsTimeToTimeval (m_sendTimeout);
            *optlen = sizeof (struct timeval);
          } break;
        case SO_SNDBUF:
          {
            if (*optlen < sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *val = (int*)optval;
            UintegerValue attrValue;
            if (!m_socket->GetAttributeFailSafe ("SndBufSize", attrValue))
              {
                current->err = EINVAL;
                return -1;
              }
            *val = attrValue.Get ();
            *optlen = sizeof (int);
          } break;

        case SO_RCVBUF:
          {
            if (*optlen < sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *val = (int*)optval;
            UintegerValue attrValue;
            if (!m_socket->GetAttributeFailSafe ("RcvBufSize", attrValue))
              {
                current->err = EINVAL;
                return -1;
              }
            *val = attrValue.Get ();
            *optlen = sizeof (int);
          } break;

        default:
          NS_LOG_WARN ("Unsupported setsockopt requested. level: SOL_SOCKET, optname: " << optname);
          break;
        }
      break;
    case SOL_IP:
      switch (optname)
        {
        case IP_RECVERR:
          {
            if (*optlen < sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            *v = m_recverr;
            *optlen = sizeof (int);
          } break;
        case IP_RECVTTL:
          {
            if (*optlen < sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            *v = m_recvttl;
            *optlen = sizeof (int);
          } break;
        case IP_TTL:
          {
            if (*optlen < sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            UintegerValue val;
            if (!m_socket->GetAttributeFailSafe ("IpTtl", val))
              {
                current->err = ENOPROTOOPT;
                return -1;
              }
            *v = val.Get ();
            *optlen = sizeof (int);
          } break;
        case IP_HDRINCL:
          {
            if (*optlen != sizeof (int))
              {
                current->err = EINVAL;
                return -1;
              }
            int *v = (int*)optval;
            BooleanValue val;
            if (!m_socket->GetAttributeFailSafe ("IpHeaderInclude", val))
              {
                current->err = ENOPROTOOPT;
                return -1;
              }
            *v = val.Get () ? 1 : 0;
            *optlen = sizeof (int);
          } break;
        default:
          NS_LOG_WARN ("Unsupported setsockopt requested. level: SOL_IP, optname: " << optname);
          break;
        }
      break;
    }
  return 0;
}
int
UnixSocketFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << name << *namelen);
  NS_ASSERT (current != 0);
  Address ad;
  int status = m_socket->GetSockName (ad);
  if (status == -1)
    {
      current->err = ErrnoToSimuErrno ();
      return -1;
    }
  if (Ns3AddressToPosixAddress (ad, name, namelen) == -1)
    {
      current->err = EINVAL;
      return -1;
    }
  return 0;
}
int
UnixSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  //XXX
  return -1;
}
int
UnixSocketFd::Ioctl (unsigned long request, char *argp)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  if (FIONBIO == request)
    {
      const int *arg = (const int *) argp;
      if (*arg)
        {
          m_statusFlags = m_statusFlags | O_NONBLOCK;
        }
      else
        {
          m_statusFlags = m_statusFlags & ~O_NONBLOCK;
        }
      return 0;
    }
  else
    {
      current->err = EINVAL;

      return -1;
    }
}
Address
UnixSocketFd::PosixAddressToNs3Address (const struct sockaddr *my_addr, socklen_t addrlen) const
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
  else if (my_addr->sa_family == AF_NETLINK)
    {
      const struct sockaddr_nl *addr = (const struct sockaddr_nl *)my_addr;
      //user space netlink socket has a nozero process id
      //uint32_t pid = addr->nl_pid ? addr->nl_pid : getpid ();

      //pretend that it is kernel-level netlink socket. is required by unmodified version of quagga
      uint32_t pid = addr->nl_pid;

      NetlinkSocketAddress nladdress = NetlinkSocketAddress (pid, addr->nl_groups);
      return nladdress;
    }
  NS_ASSERT (false);
  return Address ();
}
int
UnixSocketFd::Ns3AddressToPosixAddress (const Address& nsaddr,
                                        struct sockaddr *addr, socklen_t *addrlen) const
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
  else if (NetlinkSocketAddress::IsMatchingType (nsaddr))
    {
      NetlinkSocketAddress ns_nladdr = NetlinkSocketAddress::ConvertFrom (nsaddr);
      NS_ASSERT (*addrlen >= sizeof (struct sockaddr_nl));
      struct sockaddr_nl *nl_addr = (struct sockaddr_nl *)addr;
      nl_addr->nl_family = AF_NETLINK;
      nl_addr->nl_pid = ns_nladdr.GetProcessID ();
      nl_addr->nl_groups = ns_nladdr.GetGroupsMask ();
      *addrlen = sizeof (struct sockaddr_nl);
      return 0;
    }
  else
    {
      NS_ASSERT (false);
    }
  return 0;
}
int
UnixSocketFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  Address ad = PosixAddressToNs3Address (my_addr, addrlen);
  int result = m_socket->Bind (ad);
  if (result == -1)
    {
      current->err = ErrnoToSimuErrno ();
    }
  return result;
}
void
UnixSocketFd::MainConnect (int *r, Address adr)
{
  *r = m_socket->Connect (adr);
}
int
UnixSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  Address ad = PosixAddressToNs3Address (my_addr, addrlen);
  TaskManager *manager = TaskManager::Current ();

  int result = -1;
  manager->ExecOnMain (MakeEvent (&UnixSocketFd::MainConnect, this, &result, ad));

  if (result == -1)
    {
      current->err = ErrnoToSimuErrno ();
    }
  return result;
}
bool
UnixSocketFd::IsRecvErr (void) const
{
  return m_recverr == 1;
}
bool
UnixSocketFd::IsRecvTtl (void) const
{
  return m_recvttl == 1;
}
// bool
// UnixSocketFd::IsRecvPktInfo (void) const
// {
//   return m_recvpktinfo == 1;
// }
// bool
// UnixSocketFd::IsRecvPktInfo6 (void) const
// {
//   return m_recvpktinfo6 == 1;
// }
// bool
// UnixSocketFd::IsIpHdrIncl (void) const
// {
//   return m_iphdrincl == 1;
// }
off64_t
UnixSocketFd::Lseek (off64_t offset, int whence)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << offset << whence);
  NS_ASSERT (current != 0);
  current->err = ESPIPE;
  return -1;
}
bool
UnixSocketFd::WaitRecvDoSignal (bool dontwait)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << dontwait);
  if (dontwait && !CanRecv ())
    {
      current->err = EAGAIN;
      return false;
    }
  if (!CanRecv ())
    {
      bool ok = false;
      WaitQueueEntryTimeout *wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, m_recvTimeout);
      AddWaitQueue (wq, true);
      NS_LOG_DEBUG ("WaitRecvDoSignal: waiting ...");
      PollTable::Result res = wq->Wait ();
      NS_LOG_DEBUG ("WaitRecvDoSignal: wait result:" << res);
      RemoveWaitQueue (wq, true);
      NS_LOG_FUNCTION (this << "DELETING: " << wq);
      delete wq;
      wq = 0;

      if (res == PollTable::INTERRUPTED)
        {
          UtilsDoSignal ();
          current->err = EINTR;
          return false;
        }
      else
        {
          ok = (res == PollTable::OK);
          if (!ok)
            {
              current->err = EAGAIN;
            }
        }

      return ok;
    }
  NS_ASSERT (CanRecv ());
  return true;
}
Time
UnixSocketFd::GetRecvTimeout (void)
{
  return m_recvTimeout;
}
Time
UnixSocketFd::GetSendTimeout (void)
{
  return m_sendTimeout;
}

int
UnixSocketFd::Settime (int flags,
                       const struct itimerspec *new_value,
                       struct itimerspec *old_value)
{
  NS_LOG_FUNCTION (this << Current () << flags << new_value << old_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}
int
UnixSocketFd::Gettime (struct itimerspec *cur_value) const
{
  NS_LOG_FUNCTION (this << Current () << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}

void
UnixSocketFd::ClearSocket (void)
{
  if (m_socket)
    {
      Callback<void, Ptr<Socket> > nil = MakeNullCallback<void, Ptr<Socket> > ();

      Callback<void, Ptr<Socket>, const Address &> nil2 = MakeNullCallback<void, Ptr<Socket>, const Address &> ();
      Callback<bool, Ptr<Socket>, const Address &> nil3 = MakeNullCallback<bool, Ptr<Socket>, const Address &> ();

      m_socket->SetAcceptCallback (nil3, nil2);

      m_socket->SetConnectCallback (nil, nil);
      m_socket->SetCloseCallbacks  (nil, nil);

      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket->SetSendCallback (MakeNullCallback<void,Ptr<Socket>,uint32_t> ());
    }

  m_socket = 0;
}
void
UnixSocketFd::ChangeSocket (Ptr<Socket> socket)
{
  ClearSocket ();
  m_socket = socket;
  m_socket->SetAttributeFailSafe ("SndBufSize", UintegerValue (4096));
  m_socket->SetAttributeFailSafe ("RcvBufSize", UintegerValue (4096));
  m_socket->SetRecvCallback (MakeCallback (&UnixSocketFd::RecvSocketData, this));
  m_socket->SetSendCallback (MakeCallback (&UnixSocketFd::SendSocketData, this));
}
int
UnixSocketFd::Ns3AddressToDeviceIndependantPhysicalLayerAddress (const Address& nsaddr, const Packet& pac,
                                                                 struct sockaddr_ll *addr, socklen_t *addrlen) const
{
  if (PacketSocketAddress::IsMatchingType (nsaddr))
    {
      PacketSocketAddress ll_addr = PacketSocketAddress::ConvertFrom (nsaddr);
      if (*addrlen < sizeof (struct sockaddr_ll))
        {
          return -1;
        }
      memset (addr, 0, sizeof (struct sockaddr_ll));
      addr->sll_family = AF_PACKET;
      addr->sll_protocol =  htons (ll_addr.GetProtocol ());
      addr->sll_ifindex = ll_addr.GetSingleDevice () + 1;
      addr->sll_hatype = 0;
      ll_addr.GetPhysicalAddress ().CopyAllTo (&(addr->sll_pkttype), 8);
      *addrlen = sizeof(struct sockaddr_ll);

      PacketSocketTag pst;
      DeviceNameTag dnt;
      bool found;

      found = pac.PeekPacketTag (dnt);
      if  (found)
        {
          if (dnt.GetDeviceName () == "NetDevice")
            {
              addr->sll_hatype = ARPHRD_PPP;
            }
          else if (dnt.GetDeviceName () == "LoopbackNetDevice")
            {
              addr->sll_hatype = ARPHRD_LOOPBACK;
            }
          else if (dnt.GetDeviceName () == "CsmaNetDevice")
            {
              addr->sll_hatype = ARPHRD_ETHER;
            }
          else if (dnt.GetDeviceName () == "PointToPointNetDevice")
            {
              addr->sll_hatype = ARPHRD_PPP;
            }
          else if (dnt.GetDeviceName () == "WifiNetDevice")
            {
              addr->sll_hatype = ARPHRD_IEEE80211;
            }
        }
      found = pac.PeekPacketTag (pst);
      if (found)
        {
          addr->sll_pkttype = pst.GetPacketType ();
        }
    }
  else
    {
      NS_ASSERT (false);
    }
  return 0;
}
int
UnixSocketFd::Ftruncate (off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (this << current << length);

  current->err = EINVAL;
  return -1;
}
int
UnixSocketFd::Fsync (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

void
UnixSocketFd::AddPeekedData (const uint8_t *buf, uint32_t count, Address from)
{
  Ptr<Packet> p =  Create<Packet> (buf, count);
  m_peekedAddress = from;
  if (!m_peekedData)
    {
      m_peekedData = p;
    }
  else
    {
      m_peekedData->AddAtEnd (p);
    }
}

void
UnixSocketFd::AddPeekedData (Ptr<Packet> p)
{
  if (!m_peekedData)
    {
      m_peekedData = p;
    }
  else
    {
      m_peekedData->AddAtEnd (p);
    }
}
bool
UnixSocketFd::isPeekedData (void)
{
  return ((0 != m_peekedData) && (m_peekedData->GetSize () > 0));
}
Address
UnixSocketFd::GetPeekedFrom (void)
{
  return m_peekedAddress;
}

} // namespace ns3
