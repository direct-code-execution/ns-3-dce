#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "unix-datagram-socket-fd.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/inet-socket-address.h"
#include "cmsg.h"
#include <errno.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/errqueue.h>
#include <arpa/inet.h>

NS_LOG_COMPONENT_DEFINE ("UnixDatagramSocketFd");

namespace ns3 {

UnixDatagramSocketFd::UnixDatagramSocketFd (Ptr<Socket> sock)
  : UnixSocketFd (sock)
{
  // XXX: should I really do this here ?
  m_socket->SetAttributeFailSafe ("IcmpCallback", CallbackValue (MakeCallback (&UnixDatagramSocketFd::IcmpCallback, this)));
}

bool 
UnixDatagramSocketFd::CanRecv (void) const
{
  return m_socket != 0 && m_socket->GetRxAvailable () != 0;
}
bool 
UnixDatagramSocketFd::CanSend (void) const
{
  return m_socket != 0 && m_socket->GetTxAvailable () != 0;
}
bool
UnixDatagramSocketFd::HangupReceived (void) const
{
  return false;
}
void 
UnixDatagramSocketFd::QueueErr (sock_extended_err ee, struct sockaddr_in offender, uint8_t ttl)
{
  NS_LOG_FUNCTION (this);
  if (!IsRecvErr ())
    {
      return;
    }
  struct Error e;
  e.ee = ee;
  e.offender = offender;
  e.ttl = ttl;
  m_errQueue.push_back (e);
}

void
UnixDatagramSocketFd::IcmpCallback (Ipv4Address icmpSource, uint8_t icmpTtl, 
				    uint8_t icmpType, uint8_t icmpCode,
				    uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  // received icmp errors never interrupt a blocked recv and
  // never notify a blocked select/poll.

  sock_extended_err ee;
  struct sockaddr_in offender;

  if (icmpType == Icmpv4Header::DEST_UNREACH &&
      icmpCode == Icmpv4DestinationUnreachable::FRAG_NEEDED)
    {
      
      ee.ee_errno = EMSGSIZE;
    }
  else if (icmpType == Icmpv4Header::DEST_UNREACH &&
	   icmpCode == Icmpv4DestinationUnreachable::PORT_UNREACHABLE)
    {
      
      ee.ee_errno = EHOSTUNREACH;
    }
  else if (icmpType == Icmpv4Header::TIME_EXCEEDED &&
	   icmpCode == Icmpv4TimeExceeded::TIME_TO_LIVE)
    {
      ee.ee_errno = EHOSTUNREACH;
    }
  else
    {
      NS_ASSERT (false);
      ee.ee_errno = 0;
    }
  ee.ee_origin = SO_EE_ORIGIN_ICMP;
  ee.ee_type = icmpType;
  ee.ee_code = icmpCode;
  ee.ee_pad = 0;
  ee.ee_info = icmpInfo;
  ee.ee_data = 0;
  offender.sin_family = AF_INET;
  offender.sin_addr.s_addr = htonl (icmpSource.Get ());
  offender.sin_port = 0;
  memset (offender.sin_zero, 0, sizeof (offender.sin_zero));
  QueueErr (ee, offender, icmpTtl);
}

ssize_t 
UnixDatagramSocketFd::DoRecvmsg (struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  if (flags & MSG_ERRQUEUE)
    {
      // MSG_ERRQUEUE is valid only for DGRAM sockets.
      if (m_errQueue.empty ())
	{
	  current->err = EAGAIN;
	  return -1;
	}
      
      Cmsg cmsg = Cmsg (msg);
      if (IsRecvErr ())
	{
	  cmsg.Add (SOL_IP, IP_RECVERR, sizeof (struct Error), (const uint8_t*)&m_errQueue.front ());
	}
      if (IsRecvTtl ())
	{
	  int tmp = m_errQueue.front ().ttl;
	  cmsg.Add (SOL_IP, IP_TTL, sizeof (int), (const uint8_t*)&tmp);
	}
      cmsg.Finish ();
      m_errQueue.pop_front ();
      return 0;
    }
  else
    {
      msg->msg_controllen = 0;
    }

  if (!WaitRecvDoSignal (flags & MSG_DONTWAIT))
    {
      // current->err set by call above.
      return -1;
    }

  uint32_t count = msg->msg_iov[0].iov_len;
  uint8_t *buf = (uint8_t *)msg->msg_iov[0].iov_base;
  // we ignore the secondary items of the iovec buffers.
  // because we implement a datagram-only socket for now.
  Address from;
  Ptr<Packet> packet = m_socket->RecvFrom (count, flags, from);
  if (packet == 0)
    {
      current->err = ErrnoToSimuErrno ();
      return -1;
    }
  Ns3AddressToPosixAddress (from, (struct sockaddr*)msg->msg_name, &msg->msg_namelen);
  // XXX: we ignore MSG_TRUNC for the return value.
  NS_ASSERT (packet->GetSize () <= count);
  memcpy (buf, packet->PeekData (), packet->GetSize ());
  return packet->GetSize ();
}
ssize_t 
UnixDatagramSocketFd::DoSendmsg(const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);


  ssize_t retval = 0;
  for (uint32_t i = 0; i < msg->msg_iovlen; ++i)
    {
      uint8_t *buf = (uint8_t *)msg->msg_iov[i].iov_base;
      uint32_t len = msg->msg_iov[i].iov_len;
      Ptr<Packet> packet = Create<Packet> (buf, len);
      int result;
      if (msg->msg_name != 0 && msg->msg_namelen != 0)
	{
	  Address ad = PosixAddressToNs3Address ((const struct sockaddr *)msg->msg_name, 
						 (socklen_t)msg->msg_namelen);
	  result = m_socket->SendTo (packet, flags, ad);
	}
      else
	{
	  result = m_socket->Send (packet);
	}
      if (result == -1)
	{
	  current->err = ErrnoToSimuErrno ();
	  return -1;
	}
      retval += result;
    }
  return retval;
}
int 
UnixDatagramSocketFd::Listen (int backlog)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EOPNOTSUPP;
  return -1;
}
int 
UnixDatagramSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen);
  NS_ASSERT (current != 0);
  current->err = EOPNOTSUPP;
  return -1;
}
int 
UnixDatagramSocketFd::Shutdown (int how)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << how);
  NS_ASSERT (current != 0);
  // XXX: linux does not generate EOPNOTSUPP for this. I _think_ it 
  // generates ENOTCONN which, honestly, makes _zero_ sense.
  current->err = EOPNOTSUPP;
  return -1;
}


} // namespace ns3
