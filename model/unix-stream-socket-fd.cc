#include "unix-stream-socket-fd.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "waiter.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include <errno.h>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("UnixStreamSocketFd");

namespace ns3 {

UnixStreamSocketFd::UnixStreamSocketFd (Ptr<Socket> sock)
  : UnixSocketFd (sock),
    m_backlog (0),
    m_connected (0),
    m_connecting (0)
{
  NS_LOG_FUNCTION (this << sock);
  m_socket->SetAcceptCallback (MakeCallback (&UnixStreamSocketFd::ConnectionRequest, this),
                               MakeCallback (&UnixStreamSocketFd::ConnectionCreated, this));
  m_socket->SetConnectCallback (MakeCallback (&UnixStreamSocketFd::ConnectionSuccess, this),
                                MakeCallback (&UnixStreamSocketFd::ConnectionError, this));
  m_socket->SetCloseCallbacks  (MakeCallback (&UnixStreamSocketFd::CloseSuccess, this),
                                MakeCallback (&UnixStreamSocketFd::CloseError, this));
}

UnixStreamSocketFd::~UnixStreamSocketFd (void)
{
  NS_LOG_FUNCTION (this << m_socket);
}

ssize_t 
UnixStreamSocketFd::DoRecvmsg(struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << msg << flags << m_connected);
  NS_ASSERT (current != 0);

  if (!WaitRecvDoSignal (flags & MSG_DONTWAIT))
    {
      // current->err set by callee
      return -1;
    }
  if (!m_connected) return 0;

  uint32_t totalAvailable = 0;
  for (uint32_t i = 0; i < msg->msg_iovlen; i++)
    {
      totalAvailable += msg->msg_iov[i].iov_len;
    }
  Address from;
  Ptr<Packet> packet = m_socket->RecvFrom (totalAvailable, flags, from);
  if (packet == 0)
    {
      current->err = ErrnoToSimuErrno ();
      return -1;
    }
  NS_ASSERT (packet->GetSize () <= totalAvailable);
  const uint8_t *buf = packet->PeekData ();
  size_t toCopy = packet->GetSize ();
  for (uint32_t i = 0; i < msg->msg_iovlen && toCopy > 0; i++)
    {
      uint32_t len = std::min (toCopy, msg->msg_iov[i].iov_len);
      memcpy (msg->msg_iov[i].iov_base, buf, len);
      toCopy -= len;
      buf += len;
    }
  Ns3AddressToPosixAddress (from, (struct sockaddr*)msg->msg_name, &msg->msg_namelen);
  return packet->GetSize ();
}
ssize_t 
UnixStreamSocketFd::DoSendmsg(const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << msg << flags);
  NS_ASSERT (current != 0);

  if (msg->msg_name != 0 || msg->msg_namelen != 0)
    {
      current->err = EISCONN;
      return -1;
    }

  Waiter waiter;
  waiter.SetTimeout (GetSendTimeout ());
  ssize_t retval = 0;
  for (uint32_t i = 0; i < msg->msg_iovlen; ++i)
    {
      uint8_t *buf = (uint8_t *)msg->msg_iov[i].iov_base;
      ssize_t len = msg->msg_iov[i].iov_len;
      while (len > 0)
	{
	  while (m_socket->GetTxAvailable () == 0)
	    {
	      SetSendWaiter (&waiter);
	      Waiter::Result result = waiter.Wait ();
	      SetSendWaiter (0);
	      switch (result) {
	      case Waiter::OK:
		break;
	      case Waiter::INTERRUPTED:
		if (retval != 0)
		  {
		    // this is a short write
		    return retval;
		  }
		else
		  {
		    UtilsDoSignal ();
		    current->err = EINTR;
		    return -1;
		  }
		break;
	      case Waiter::TIMEOUT:
		if (retval != 0)
		  {
		    // this is a short write
		    return retval;
		  }
		else
		  {
		    current->err = EAGAIN;
		    return -1;
		  }
		break;
	      }
	    }
	  ssize_t availLen = std::min (len, (ssize_t)m_socket->GetTxAvailable ());
	  Ptr<Packet> packet = Create<Packet> (buf, availLen);
	  int result;
	  result = m_socket->Send (packet, flags);
	  if (result == -1 && retval == 0)
	    {
	      if (retval != 0)
		{
		  // this is a short write
		  return retval;
		}
	      else
		{
		  current->err = ErrnoToSimuErrno ();
		  return -1;
		}
	    }
	  NS_ASSERT (result == availLen);
	  len -= result;
	  buf += result;
	  retval += result;
	}
    }
  return retval;
}
int 
UnixStreamSocketFd::Listen (int backlog)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << backlog);
  NS_ASSERT (current != 0);

  int retval = m_socket->Listen ();
  if (retval == -1)
    {
      current->err = ErrnoToSimuErrno ();
      return -1;
    }
  m_backlog = backlog;
  return 0;
}
int 
UnixStreamSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen);
  NS_ASSERT (current != 0);

  Waiter waiter;
  SetRecvWaiter (&waiter);
  waiter.SetTimeout (GetRecvTimeout ());
  while (m_connectionQueue.empty ())
    {
      if (!waiter.Wait ())
	{
	  // current->err set by caller above.
          SetRecvWaiter (0);
          return -1;
	}
    }
  SetRecvWaiter (0);

  // create an fd for the socket.
  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }

  Ptr<Socket> sock = m_connectionQueue.front ().first;
  Address ad = m_connectionQueue.front ().second;
  m_connectionQueue.pop_front ();
  UnixStreamSocketFd *socket = new UnixStreamSocketFd (sock);

  Ns3AddressToPosixAddress (ad, my_addr, addrlen);
  current->process->openFiles.push_back (std::make_pair (fd, socket));
  return fd;
}
bool 
UnixStreamSocketFd::CanRecv (void) const
{

  return m_socket != 0 && ( ( m_connected && (m_socket->GetRxAvailable () != 0 || !m_connectionQueue.empty ())) ||
        !m_connected );
}
bool
UnixStreamSocketFd::CanSend (void) const
{
  return m_socket != 0 && m_socket->GetTxAvailable () != 0;
}
bool
UnixStreamSocketFd::HangupReceived (void) const
{
  return false;
}
bool 
UnixStreamSocketFd::ConnectionRequest (Ptr<Socket> sock, const Address & from)
{
  NS_LOG_FUNCTION (sock << from);
  return ((int)m_connectionQueue.size ()) < m_backlog;
}
void 
UnixStreamSocketFd::ConnectionCreated (Ptr<Socket> sock, const Address & from)
{
  NS_LOG_FUNCTION (sock << from);
  NS_ASSERT (((int)m_connectionQueue.size ()) < m_backlog);
  m_connectionQueue.push_back (std::make_pair (sock, from));
  WakeupRecv ();
}
int 
UnixStreamSocketFd::Shutdown (int how)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << how);
  NS_ASSERT (current != 0);
  
  int retval;
  if (how == SHUT_RD)
    {
      retval = m_socket->ShutdownRecv ();
    }
  else if (how == SHUT_WR)
    {
      retval = m_socket->ShutdownSend ();
    }
  else if (how == SHUT_RDWR)
    {
      retval = m_socket->ShutdownRecv ();
      retval = m_socket->ShutdownSend ();
    }
  else
    {
      current->err = EINVAL;
      return -1;
    }
  if (retval == -1)
    {
      current->err = ErrnoToSimuErrno ();
      return -1;
    }
  m_connected = false;
  return 0;
}
void
UnixStreamSocketFd::ConnectionSuccess (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);
  m_connected = true;
  m_connecting = false;
  WakeupRecv ();
}
void
UnixStreamSocketFd::ConnectionError(Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);
  m_connecting = false;
  WakeupRecv ();
}
void
UnixStreamSocketFd::CloseSuccess (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);
  m_connected = false;
  m_connecting = false;
  WakeupRecv ();
}
void
UnixStreamSocketFd::CloseError(Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);
  m_connecting = false;
}

int
UnixStreamSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  if (m_connecting)
    {
      current -> err = EINVAL;
      return -1;
    }

  m_connecting = true;

  int sup = UnixSocketFd::Connect( my_addr, addrlen);

  if (0 == sup) {
      sup = -1;
      Waiter waiter;

      while (m_connecting)
        {
          SetRecvWaiter (&waiter);
          waiter.SetTimeout (GetRecvTimeout ());
          Waiter::Result result = waiter.Wait ();
          NS_LOG_DEBUG("Connect: wait result:" << result);
          SetRecvWaiter (0);

          switch (result)
          {
            case Waiter::OK:

              break;
            case Waiter::INTERRUPTED:
              {
                UtilsDoSignal ();
                current->err = EINTR;
                return -1;
              }
            case Waiter::TIMEOUT:
              {
                current->err = EAGAIN;
                return -1;
              }
          }
        }
  }
  if (m_connected) sup = 0; else sup = -1;
  return sup;
}


} // namespace ns3
