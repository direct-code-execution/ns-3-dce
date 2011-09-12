#include "unix-socket-fd.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "waiter.h"
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
#include <fcntl.h>
#include <errno.h>
#include <linux/icmp.h> // need ICMP_FILTER
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <net/ethernet.h>
#include <linux/if_arp.h>

NS_LOG_COMPONENT_DEFINE ("UnixSocketFd");

namespace ns3 {

UnixSocketFd::UnixSocketFd (Ptr<Socket> socket)
  : m_socket (socket),
    m_sendTimeout (Seconds (0.0)),
    m_recvTimeout (Seconds (0.0)),
    m_statusFlags (0)
{
  m_socket->SetAttributeFailSafe ("SndBufSize", UintegerValue (126976));
  m_socket->SetAttributeFailSafe ("RcvBufSize", UintegerValue (126976));
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
  NS_ASSERT (Current ()!= 0);
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
UnixSocketFd::Fcntl (int cmd, unsigned long arg)
{
  switch (cmd) 
    {
    case F_GETFL: //XXX this command should also consider the flags O_APPEND and O_ASYNC
      return m_statusFlags;
      break;
    case F_SETFL:
      m_statusFlags = arg;
      return 0;
      break;
    default:
      //XXX commands missing
      NS_FATAL_ERROR ("fcntl not implemented on socket");
      return -1;
    }
}
int 
UnixSocketFd::ErrnoToSimuErrno (void) const
{
  switch (m_socket->GetErrno ()) {
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
    NS_LOG_ERROR("Unknown Errno:" <<  m_socket->GetErrno ());
    NS_ASSERT (false);
    return 0; // quiet compiler
    break;
  }
}
void 
UnixSocketFd::RecvSocketData (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << m_socket << socket);
  WakeupRecv ();
}
void 
UnixSocketFd::SendSocketData (Ptr<Socket> socket, uint32_t available)
{
  NS_LOG_FUNCTION (this << m_socket << socket);
  WakeupSend ();  
}

int 
UnixSocketFd::Close (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  int result = m_socket->Close ();
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
  WakeupSend ();
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
UnixSocketFd::Recvmsg(struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking?MSG_DONTWAIT:0;
  return DoRecvmsg (msg, flags);
}
ssize_t
UnixSocketFd::Sendmsg(const struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking?MSG_DONTWAIT:0;
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

  switch (level) {
  case SOL_RAW:
    switch (optname) {
    case ICMP_FILTER: {
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
    }
    break;
  case SOL_SOCKET:
    switch (optname) {
    case SO_RCVTIMEO: {
      if (optlen != sizeof (struct timeval))
	{
	  current->err = EINVAL;
	  return -1;
	}
      struct timeval *tv = (struct timeval *)optval;
      m_recvTimeout = UtilsTimevalToTime (*tv);
    } break;
    case SO_SNDTIMEO: {
      if (optlen != sizeof (struct timeval))
	{
	  current->err = EINVAL;
	  return -1;
	}
      struct timeval *tv = (struct timeval *)optval;
      m_sendTimeout = UtilsTimevalToTime (*tv);
    } break;
    case SO_SNDBUF: {
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
    }
    break;
  case SOL_IP:
    switch (optname) {
    case IP_RECVERR: {
      if (optlen != sizeof (int))
	{
	  current->err = EINVAL;
	  return -1;
	}
      int *v = (int*)optval;
      m_recverr = *v;
    } break;
    case IP_RECVTTL: {
      if (optlen != sizeof (int))
	{
	  current->err = EINVAL;
	  return -1;
	}
      int *v = (int*)optval;
      m_recvttl = *v;
    } break;
    case IP_TTL: {
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
    }
    break;
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

  switch (level) {
  case SOL_RAW:
    switch (optname) {
    case ICMP_FILTER: {
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
    }
    break;
  case SOL_SOCKET:
    switch (optname) {
    case SO_RCVTIMEO: {
      if (*optlen < sizeof (struct timeval))
	{
	  current->err = EINVAL;
	  return -1;
	}
      struct timeval *tv = (struct timeval *)optval;
      *tv = UtilsTimeToTimeval (m_recvTimeout);
      *optlen = sizeof (struct timeval);
    } break;
    case SO_SNDTIMEO: {
      if (*optlen < sizeof (struct timeval))
	{
	  current->err = EINVAL;
	  return -1;
	}
      struct timeval *tv = (struct timeval *)optval;
      *tv = UtilsTimeToTimeval (m_sendTimeout);
      *optlen = sizeof (struct timeval);
    } break;
    case SO_SNDBUF: {
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
    }
    break;
  case SOL_IP:
    switch (optname) {
    case IP_RECVERR: {
      if (*optlen < sizeof (int))
	{
	  current->err = EINVAL;
	  return -1;
	}
      int *v = (int*)optval;
      *v = m_recverr;
      *optlen = sizeof (int);
    } break;
    case IP_RECVTTL: {
      if (*optlen < sizeof (int))
	{
	  current->err = EINVAL;
	  return -1;
	}
      int *v = (int*)optval;
      *v = m_recvttl;
      *optlen = sizeof (int);
    } break;
    case IP_TTL: {
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
    }
    break;
  }
  return 0;
}
int 
UnixSocketFd::Getsockname(struct sockaddr *name, socklen_t *namelen)
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
UnixSocketFd::Getpeername(struct sockaddr *name, socklen_t *namelen)
{
  //XXX
  return -1;
}
int 
UnixSocketFd::Ioctl (int request, char *argp)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EINVAL;  
  return -1;
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
  NS_ASSERT (false);
  return Address ();
}
int
UnixSocketFd::Ns3AddressToPosixAddress(const Address& nsaddr, 
				       struct sockaddr *addr, socklen_t *addrlen) const
{
  if (addr == 0 || addrlen == 0)
    {
      return 0;
    }
  if (InetSocketAddress::IsMatchingType(nsaddr))
    {
      InetSocketAddress ns_inetaddr = InetSocketAddress::ConvertFrom(nsaddr);
      if (*addrlen < sizeof (struct sockaddr_in))
	{
	  return -1;
	}
      struct sockaddr_in *inet_addr = (struct sockaddr_in *)addr;
      inet_addr->sin_family = AF_INET;
      inet_addr->sin_port = htons (ns_inetaddr.GetPort ());
      inet_addr->sin_addr.s_addr = htonl (ns_inetaddr.GetIpv4 ().Get ());
      *addrlen = sizeof(struct sockaddr_in);
    }
  else
    {
      NS_ASSERT (false);
    }
  return 0;
}
#ifdef NEW_PACKET_SOCKET
int 
UnixSocketFd::Ns3AddressToDeviceIndependantPhysicalLayerAddress (const Address& nsaddr, const Packet& pac,
                                                                 struct sockaddr_ll *addr, socklen_t *addrlen) const
{
  if (PacketSocketAddress::IsMatchingType(nsaddr))
      {
        PacketSocketAddress ll_addr = PacketSocketAddress::ConvertFrom(nsaddr);
        if (*addrlen < sizeof (struct sockaddr_ll))
          {
            return -1;
          }
        memset (addr, 0, sizeof (struct sockaddr_ll));
        addr->sll_family = AF_PACKET;
        addr->sll_protocol =  htons( ll_addr.GetProtocol() );
        addr->sll_ifindex = ll_addr.GetSingleDevice() + 1;
        addr->sll_hatype = 0;
        ll_addr.GetPhysicalAddress().CopyAllTo(&(addr->sll_pkttype), 8);
        *addrlen = sizeof(struct sockaddr_ll);

        PacketSocketTag pst;
        DeviceNameTag dnt;
        bool found;

        found = pac.PeekPacketTag (dnt);
        if  (found)
          {
            if ( dnt.GetDeviceName () == "NetDevice" )
              {
                addr->sll_hatype = ARPHRD_PPP;
              }
            else if ( dnt.GetDeviceName () == "LoopbackNetDevice" )
                {
                  addr->sll_hatype = ARPHRD_LOOPBACK;
                }
            else if ( dnt.GetDeviceName () == "CsmaNetDevice" )
                {
                  addr->sll_hatype = ARPHRD_ETHER;
                }
            else if ( dnt.GetDeviceName () == "PointToPointNetDevice" )
                {
                  addr->sll_hatype = ARPHRD_PPP;
                }
            else if ( dnt.GetDeviceName () == "WifiNetDevice" )
                {
                  addr->sll_hatype = ARPHRD_IEEE80211;
                }
          }
        found = pac.PeekPacketTag (pst);
        if (found)
          {
            addr->sll_pkttype = pst.GetPacketType();
          }
      }
    else
      {
        NS_ASSERT (false);
      }
  return 0;
}
#endif

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
int 
UnixSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  Address ad = PosixAddressToNs3Address (my_addr, addrlen);
  int result = m_socket->Connect (ad);
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
      Waiter waiter;
      SetRecvWaiter (&waiter);
      waiter.SetTimeout (m_recvTimeout);
      bool ok = waiter.WaitDoSignal ();
      SetRecvWaiter (0);
      NS_ASSERT_MSG (ok?CanRecv ():true, this << " " << current);
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
  if ( m_socket )
    {
      Callback<void, Ptr< Socket > > nil = MakeNullCallback<void, Ptr<Socket> > ();

      Callback<void, Ptr<Socket>, const Address &> nil2 = MakeNullCallback<void, Ptr<Socket>, const Address &> ();
      Callback<bool, Ptr<Socket>, const Address &> nil3 = MakeNullCallback<bool, Ptr<Socket>, const Address &> ();

      m_socket->SetAcceptCallback ( nil3, nil2) ;

      m_socket->SetConnectCallback (nil, nil);
      m_socket->SetCloseCallbacks  ( nil, nil);

      m_socket->SetRecvCallback ( nil);
      m_socket->SetSendCallback ( MakeNullCallback<void,Ptr<Socket>,uint32_t > ());
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


} // namespace ns3
