#include "unix-stream-socket-fd.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/packet.h"
#include <errno.h>
#include <algorithm>
#include "ns3/socket-factory.h"
#include <poll.h>
#include <fcntl.h>
#include "file-usage.h"

NS_LOG_COMPONENT_DEFINE ("UnixStreamSocketFd");

namespace ns3 {

UnixStreamSocketFd::UnixStreamSocketFd (Ptr<Socket> sock, bool connected)
  : UnixSocketFd (sock),
    m_backlog (0),
    m_peerAddress (0),
    m_shutWrite (0)
{
  NS_LOG_FUNCTION (this << sock);
  m_socket->SetAcceptCallback (MakeCallback (&UnixStreamSocketFd::ConnectionRequest, this),
                               MakeCallback (&UnixStreamSocketFd::ConnectionCreated, this));
  m_socket->SetConnectCallback (MakeCallback (&UnixStreamSocketFd::ConnectionSuccess, this),
                                MakeCallback (&UnixStreamSocketFd::ConnectionError, this));
  m_socket->SetCloseCallbacks  (MakeCallback (&UnixStreamSocketFd::CloseSuccess, this),
                                MakeCallback (&UnixStreamSocketFd::CloseError, this));
  m_state = (connected) ? CONNECTED : CREATED;
}

UnixStreamSocketFd::~UnixStreamSocketFd (void)
{
  SetPeerAddress (0);
  ClearSocket ();
}

ssize_t
UnixStreamSocketFd::DoRecvmsg (struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << msg << flags << m_state);
  NS_ASSERT (current != 0);

  if (!isPeekedData ())
    {
      if (!WaitRecvDoSignal (flags & MSG_DONTWAIT))
        {
          // current->err set by callee
          return -1;
        }
      switch (m_state)
        {
        default:
        case CREATED:
          {
            current->err = EINVAL;
            return -1;
          }

        case CONNECTED:
          break;

        case CLOSING:
          return 0;

        case REMOTECLOSED:
        case CLOSED:
          {
            if (m_socket->GetRxAvailable () <= 0)
              {
                return 0;
              }
          }
          break;
        }
    }

  uint32_t totalAvailable = 0;
  uint32_t count = msg->msg_iov[0].iov_len;
  uint8_t *buf = (uint8_t *)msg->msg_iov[0].iov_base;
  size_t toCopy = 0;
  ssize_t ret = 0;
  Ptr<Packet> packet = 0;

  for (uint32_t i = 0; i < msg->msg_iovlen; i++)
    {
      totalAvailable += msg->msg_iov[i].iov_len;
    }

  if (isPeekedData ())
    {
      m_peekedData->CopyData (buf, count);
      toCopy = m_peekedData->GetSize ();

      if (toCopy > totalAvailable)
        {
          toCopy = totalAvailable;
        }
      Ns3AddressToPosixAddress (GetPeekedFrom (), (struct sockaddr*)msg->msg_name, &msg->msg_namelen);
    }
  else
    {
      Address from;
      packet = m_socket->RecvFrom (totalAvailable, flags & ~MSG_DONTWAIT & ~MSG_PEEK, from);
      if (packet == 0)
        {
          current->err = ErrnoToSimuErrno ();
          return -1;
        }
      NS_ASSERT (packet->GetSize () <= totalAvailable);
      packet->CopyData (buf, count);
      toCopy = packet->GetSize ();
      Ns3AddressToPosixAddress (from, (struct sockaddr*)msg->msg_name, &msg->msg_namelen);
      if (flags & MSG_PEEK)
        {
          AddPeekedData (buf, toCopy, from);
        }
    }
  for (uint32_t i = 0; i < msg->msg_iovlen && toCopy > 0; i++)
    {
      uint32_t len = std::min (toCopy, msg->msg_iov[i].iov_len);
      memcpy (msg->msg_iov[i].iov_base, buf, len);
      toCopy -= len;
      buf += len;
      ret += len;
    }

  if (!(flags & MSG_PEEK) && isPeekedData ())
    {
      m_peekedData->RemoveAtStart (ret);
      if (m_peekedData->GetSize () <= 0)
        {
          m_peekedData = 0;
        }
    }
  return ret;
}
/**
 * Callback used to call the socket send using the main thread.
 */
void
UnixStreamSocketFd::MainSend (int *r, Ptr<Packet> p)
{
  *r = m_socket->Send (p, 0);
}
ssize_t
UnixStreamSocketFd::DoSendmsg (const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << msg << flags);
  NS_ASSERT (current != 0);

  if (msg->msg_name != 0 || msg->msg_namelen != 0)
    {
      current->err = EISCONN;
      return -1;
    }
  WaitQueueEntryTimeout *wq = 0;

  ssize_t retval = 0;
  for (uint32_t i = 0; i < msg->msg_iovlen; ++i)
    {
      uint8_t *buf = (uint8_t *)msg->msg_iov[i].iov_base;
      ssize_t len = msg->msg_iov[i].iov_len;
      while (len > 0)
        {
          while (m_socket->GetTxAvailable () == 0)
            {
              if (flags & MSG_DONTWAIT)
                {
                  if (retval != 0)
                    {
                      // this is a short write
                      RETURNFREE (retval);
                    }
                  else
                    {
                      current->err =  EAGAIN;
                      RETURNFREE (-1);
                    }
                }

              if (CONNECTED != m_state)
                {
                  if (retval != 0)
                    {
                      // this is a short write
                      RETURNFREE (retval);
                    }
                  else
                    {
                      current->err = ENOTCONN;
                      RETURNFREE (-1);
                    }
                }

              if (m_shutWrite)
                {
                  current->err = EPIPE;
                  RETURNFREE (-1);
                }

              if (!wq)
                {
                  wq = new WaitQueueEntryTimeout (POLLOUT | POLLHUP, GetSendTimeout ());
                }
              AddWaitQueue (wq, true);
              PollTable::Result res = wq->Wait ();
              RemoveWaitQueue (wq, true);

              switch (res)
                {
                case PollTable::OK:
                  break;
                case PollTable::INTERRUPTED:
                  if (retval != 0)
                    {
                      // this is a short write
                      RETURNFREE (retval);
                    }
                  else
                    {
                      UtilsDoSignal ();
                      current->err = EINTR;
                      RETURNFREE (-1);
                    }
                  break;
                case PollTable::TIMEOUT:
                  if (retval != 0)
                    {
                      // this is a short write
                      RETURNFREE (retval);
                    }
                  else
                    {
                      current->err = EAGAIN;
                      RETURNFREE (-1);
                    }
                  break;
                }
            }
          ssize_t availLen = std::min (len, (ssize_t)m_socket->GetTxAvailable ());
          Ptr<Packet> packet = Create<Packet> (buf, availLen);
          TaskManager *manager = TaskManager::Current ();
          int result = -1;

          manager->ExecOnMain (MakeEvent (&UnixStreamSocketFd::MainSend, this, &result, packet));

          if (result == -1)
            {
              if (retval != 0)
                {
                  // this is a short write
                  RETURNFREE (retval);
                }
              else
                {
                  current->err = ErrnoToSimuErrno ();
                  RETURNFREE (-1);
                }
            }
          NS_ASSERT (result == availLen);
          len -= result;
          buf += result;
          retval += result;
        }
    }
  RETURNFREE (retval);
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
  m_state = LISTENING;
  m_backlog = backlog;
  return 0;
}
int
UnixStreamSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen << GetRecvTimeout ());
  NS_ASSERT (current != 0);

  WaitQueueEntryTimeout *wq = 0;

  while (m_connectionQueue.empty ())
    {
      if (m_statusFlags & O_NONBLOCK)
        {
          current->err = EWOULDBLOCK;
          return -1;
        }

      if (!wq)
        {
          wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, GetRecvTimeout ());
        }
      AddWaitQueue (wq, true);
      NS_LOG_DEBUG ("Accept: waiting ...");
      PollTable::Result res = wq->Wait ();
      NS_LOG_DEBUG ("Accept: wait result:" << res);
      RemoveWaitQueue (wq, true);

      switch (res)
        {
        case PollTable::OK:
          break;
        case PollTable::INTERRUPTED:
          {
            UtilsDoSignal ();
            current->err = EINTR;
            RETURNFREE (-1);
          }
        case PollTable::TIMEOUT:
          {
            current->err = EAGAIN;
            RETURNFREE (-1);
          }
        }
    }

  // create an fd for the socket.
  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      RETURNFREE (-1);
    }

  Ptr<Socket> sock = m_connectionQueue.front ().first;
  Address ad = m_connectionQueue.front ().second;
  m_connectionQueue.pop_front ();
  UnixStreamSocketFd *socket = new UnixStreamSocketFd (sock, true);
  Ns3AddressToPosixAddress (ad, my_addr, addrlen);
  socket->SetPeerAddress (new Address (ad));
  socket->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, socket);

  RETURNFREE (fd);
}
bool
UnixStreamSocketFd::CanRecv (void) const
{
  bool ret = 0;
  uint32_t rx = 0;

  if (0 == m_socket)
    {
      ret = 0;
    }
  else
    {
      switch (m_state)
        {
        case CREATED:
          ret = 1;
          break;
        case LISTENING:
          ret = (0 == m_connectionQueue.empty ());
          break;
        case CONNECTING:
          ret = 0;
          break;
        case CONNECTED:
          ret = (m_socket->GetRxAvailable () > 0);
          break;

        case CLOSING:
        case REMOTECLOSED:
        case CLOSED:
          ret = 1;
          break;

        default:
          ret = 0;
          break;
        }
      rx = m_socket->GetRxAvailable ();
    }
  NS_LOG_FUNCTION (m_socket << m_state << rx << m_connectionQueue.empty () << " ret " << ret);

  return ret;
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

  int pi = POLLIN;
  WakeWaiters (&pi);
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
      m_shutWrite = 1;
      return 0;
    }
  else if (how == SHUT_RDWR)
    {
      retval = m_socket->ShutdownRecv ();
      m_shutWrite = 1;
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
  m_state = CLOSED;
  return 0;
}
void
UnixStreamSocketFd::ConnectionSuccess (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);
  if (CONNECTING == m_state)
    {
      m_state = CONNECTED;
      Address ad;
      if (0 == sock->GetSockName (ad))
        {
          SetPeerAddress (new Address (ad));
        }
    }
  int pi = POLLIN;
  WakeWaiters (&pi);
}
void
UnixStreamSocketFd::ConnectionError (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this);
  if (CONNECTING == m_state)
    {
      m_state = CREATED;
    }
  int pi = POLLHUP;
  WakeWaiters (&pi);
}
void
UnixStreamSocketFd::CloseSuccess (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this << m_state);
  if (CLOSING == m_state)
    {
      m_state = CLOSED;
    }
  else
    {
      m_state = REMOTECLOSED;
      int pi = POLLHUP;
      WakeWaiters (&pi);
    }
  SetPeerAddress (0);
}
void
UnixStreamSocketFd::CloseError (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION (this << m_state);
}

int
UnixStreamSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  if (CONNECTING == m_state)
    {
      current->err = EALREADY;
      return -1;
    }

  if (CLOSED == m_state)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      Ptr<SocketFactory> factory = current->process->manager->GetObject<SocketFactory> (tid);

      Ptr<Socket> socket = factory->CreateSocket ();

      socket->SetAcceptCallback (MakeCallback (&UnixStreamSocketFd::ConnectionRequest, this),
                                 MakeCallback (&UnixStreamSocketFd::ConnectionCreated, this));
      socket->SetConnectCallback (MakeCallback (&UnixStreamSocketFd::ConnectionSuccess, this),
                                  MakeCallback (&UnixStreamSocketFd::ConnectionError, this));
      socket->SetCloseCallbacks  (MakeCallback (&UnixStreamSocketFd::CloseSuccess, this),
                                  MakeCallback (&UnixStreamSocketFd::CloseError, this));

      ChangeSocket (socket);

      m_state = CREATED;

    }

  m_state = CONNECTING;

  int sup = UnixSocketFd::Connect (my_addr, addrlen);

  if (0 == sup)
    {
      sup = -1;
      WaitQueueEntryTimeout *wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, GetRecvTimeout ());

      while (CONNECTING == m_state)
        {
          AddWaitQueue (wq, true);
          NS_LOG_DEBUG ("Connect: waiting ...");
          PollTable::Result res = wq->Wait ();
          NS_LOG_DEBUG ("Connect: wait result:" << res);
          RemoveWaitQueue (wq, true);

          switch (res)
            {
            case PollTable::OK:
              break;
            case PollTable::INTERRUPTED:
              {
                UtilsDoSignal ();
                current->err = EINTR;
                RETURNFREE (-1);
              }
            case PollTable::TIMEOUT:
              {
                current->err = EAGAIN;
                RETURNFREE (-1);
              }
            }
        }
      delete wq;
      wq = 0;
    }
  if (CONNECTED == m_state)
    {
      sup = 0;
      Address ad = PosixAddressToNs3Address (my_addr, addrlen);

      SetPeerAddress (new Address (ad));
    }
  else
    {
      sup = -1;

      if ((m_state == CLOSED)||(REMOTECLOSED == m_state))
        {
          Current ()->err = ECONNREFUSED;
        }
    }
  return sup;
}
int
UnixStreamSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << name << *namelen);
  NS_ASSERT (current != 0);
  if (0 != m_peerAddress)
    {
      if (Ns3AddressToPosixAddress (*m_peerAddress, name, namelen) == -1)
        {
          current->err = EINVAL;
          return -1;
        }
      return 0;
    }
  current->err = ENOTCONN;
  return -1;
}
int
UnixStreamSocketFd::Poll (PollTable* ptable)
{
  int ret = 0;

  if (CanRecv ())
    {
      ret |= POLLIN;
    }
  if (CanSend ())
    {
      ret |= POLLOUT;
    }
  if (HangupReceived ())
    {
      ret |= POLLHUP;
    }

  if (ptable)
    {
      ptable->PollWait (this);
    }

  return ret;
}
void
UnixStreamSocketFd::SetPeerAddress (Address *a)
{
  if (0 != m_peerAddress)
    {
      delete (m_peerAddress);
      m_peerAddress = 0;
    }
  m_peerAddress = a;
}
int
UnixStreamSocketFd::Close (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  if (CLOSING != m_state)
    {
      m_state = CLOSING;

      UnixSocketFd::Close ();

      return 0;
    }
  Current ()->err = EBADF;
  return -1;
}
} // namespace ns3
