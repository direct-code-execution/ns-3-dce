/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
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
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 */
#include "local-socket-fd.h"
#include "local-stream-socket-fd.h"
#include "local-socket-fd-factory.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include "unix-fd.h"
#include <exception>
#include <poll.h>
#include <unistd.h>
#include "wait-queue.h"
#include "ns3/simulator.h"
#include "file-usage.h"

NS_LOG_COMPONENT_DEFINE ("LocalStreamSocketFd");

namespace ns3 {
TypeId
LocalStreamSocketFd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LocalStreamSocketFd").SetParent<LocalSocketFd> ();

  return tid;
}
TypeId
LocalStreamSocketFd::GetInstanceTypeId (void) const
{
  return LocalStreamSocketFd::GetTypeId ();
}
LocalStreamSocketFd::LocalStreamSocketFd (Ptr<LocalSocketFdFactory> f)
  : m_state (CREATED),
    m_backLog (0)
{
  NS_LOG_FUNCTION (this);
  m_factory = f;
}

LocalStreamSocketFd::LocalStreamSocketFd (LocalStreamSocketFd *peer, std::string bindPath)
  : m_state (CONNECTED),
    m_backLog (0),
    m_peer (peer)
{
  NS_LOG_FUNCTION (this);
  m_bindPath = bindPath;
}

LocalStreamSocketFd::~LocalStreamSocketFd ()
{
  NS_LOG_FUNCTION (this);

  ClearAll (false);
}

void
LocalStreamSocketFd::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ClearAll (false);
  LocalSocketFd::DoDispose ();
}

int
LocalStreamSocketFd::Close (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this);
  NS_ASSERT (current != 0);

  NS_LOG_DEBUG ("m_state: " << m_state);

  switch (m_state)
    {
    case CREATED:
      {
        ClearAll (false);
        return 0;
      }

    case BINDED:
    case LISTENING:
    case ACCEPTING:
      {
        NS_ASSERT (0 != m_factory);
        m_factory->UnRegisterBinder (m_bindPath);
        m_bindPath = "";
        ClearAll (ACCEPTING == m_state);

        return 0;
      }

    case CONNECTING:
      {
        ClearAll (true);
        return 0;
      }

    case CONNECTED:
      {
        NS_ASSERT (m_peer != 0);
        m_peer->PeerClosed ();

        ClearAll (true);

        return 0;
      }

    case REMOTECLOSED:
      {
        ClearAll (false);
        return 0;
      }

    case CLOSED:
      {
        return 0;
      }

    default:
      break;
    }

  current->err = EBADF;
  return -1;
}


ssize_t
LocalStreamSocketFd::Write (const void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << buf << count);
  NS_ASSERT (current != 0);

  NS_LOG_DEBUG ("Write enter state " << m_state);

  if (m_state != CONNECTED)
    {
      if (m_state == REMOTECLOSED)
        {
          return 0;
        }
      current->err = ENOTCONN;
      return -1;
    }
  if (m_shutWrite)
    {
      current->err = EPIPE;
      return -1;
    }

  NS_ASSERT (m_peer != 0);

  size_t tot = 0;

  WaitQueueEntryTimeout *wq = 0;
  do
    {
      ssize_t lg = m_peer->DoRecvPacket ((uint8_t*) buf + tot, count - tot);
      NS_LOG_FUNCTION (this <<  " lg:" << lg);
      if (0 == lg)
        {
          if (m_statusFlags & O_NONBLOCK)
            {
              // Socket do not want to wait
              Current ()->err = EAGAIN;
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
              UtilsDoSignal ();
              if (tot != 0)
                {
                  // this is a short write
                  RETURNFREE (tot);
                }
              else
                {
                  current->err = EINTR;
                  RETURNFREE (-1);
                }
              break;
            case PollTable::TIMEOUT:
              if (tot != 0)
                {
                  // this is a short write
                  RETURNFREE (tot);
                }
              else
                {
                  current->err = EAGAIN;
                  RETURNFREE (-1);
                }
              break;
            }
        }
      else if (lg > 0)
        {
          tot += lg;
        }
      else
        {
          if (-2 == lg)
            {
              if (0 == tot)
                {
                  current->err = EPIPE;
                  RETURNFREE (-1);
                }
              break;
            }
          current->err = ENOMEM;
          RETURNFREE (-1);
        }
    }
  while ((tot < count) && (m_state == CONNECTED));

  if ((0 == tot) && (m_state != CONNECTED))
    {
      current->err = EPIPE;
      RETURNFREE (-1);
    }

  RETURNFREE (tot);
}
ssize_t
LocalStreamSocketFd::Read (void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << buf << count);
  NS_ASSERT (current != 0);

  NS_LOG_DEBUG ("Read enter state: " << m_state);

  if ((m_state != CONNECTED) && (m_state != REMOTECLOSED))
    {
      current->err = EINVAL;
      return -1;
    }

  size_t filled = 0;
  WaitQueueEntryTimeout *wq = 0;

  while ((filled < count) && ((m_state == CONNECTED) || ((REMOTECLOSED == m_state) && (m_readBufferSize > 0))))
    {
      size_t lg = ReadData ((uint8_t*) buf + filled, count - filled, false);
      if (0 == lg)
        {
          if (m_statusFlags & O_NONBLOCK)
            {
              if (0 == filled)
                {
                  // Socket do not want to wait
                  Current ()->err = EAGAIN;
                  RETURNFREE (-1);
                }
              return filled;
            }
          if ((0 != m_peer) && (m_peer->IsShutWrite ()))
            {
              RETURNFREE (0);
            }
          // Nothing should wait

          if (!wq)
            {
              wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, GetRecvTimeout ());
            }
          AddWaitQueue (wq, true);
          WaitPoint::Result res = wq->Wait ();
          RemoveWaitQueue (wq, true);

          switch (res)
            {
            case WaitPoint::OK:
              break;
            case WaitPoint::INTERRUPTED:
              UtilsDoSignal ();
              if (filled != 0)
                {
                  // this is a short read
                  RETURNFREE (filled);
                }
              else
                {
                  current->err = EINTR;
                  RETURNFREE (-1);
                }
              break;
            case WaitPoint::TIMEOUT:
              if (filled != 0)
                {
                  // this is a short read
                  RETURNFREE (filled);
                }
              else
                {
                  current->err = EAGAIN;
                  RETURNFREE (-1);
                }
              break;
            }
        }
      else if (lg > 0)
        {
          filled += lg;
          if  (0 != m_peer)
            {
              short po = POLLOUT;
              m_peer->WakeWaiters (&po);
            }
        }
      else
        {
          current->err = ENOMEM;
          RETURNFREE (-1);
        }
    }

  RETURNFREE (filled);
}
ssize_t
LocalStreamSocketFd::Recvmsg (struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << msg << flags);
  NS_ASSERT (current != 0);

  ssize_t retval = 0;
  for (uint32_t i = 0; i < msg->msg_iovlen; ++i)
    {
      uint8_t *buf = (uint8_t *) msg->msg_iov[i].iov_base;
      ssize_t len = msg->msg_iov[i].iov_len;

      size_t ret = Read (buf, len);

      if ((msg->msg_namelen > 0) && (0 != msg->msg_name))
        {
          memset (msg->msg_name, 0, msg->msg_namelen);
        }

      msg->msg_namelen = 0;

      if (ret == 0)
        {
          break;
        }
      if (ret > 0)
        {
          retval += ret;
        }
      else
        {
          return -1;
        }
    }

  return retval;
}
ssize_t
LocalStreamSocketFd::Sendmsg (const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << msg << flags);
  NS_ASSERT (current != 0);

  if (msg->msg_name != 0 || msg->msg_namelen != 0)
    {
      current->err = EISCONN;
      return -1;
    }

  ssize_t retval = 0;
  for (uint32_t i = 0; i < msg->msg_iovlen; ++i)
    {
      uint8_t *buf = (uint8_t *) msg->msg_iov[i].iov_base;
      ssize_t len = msg->msg_iov[i].iov_len;

      size_t ret = Write (buf, len);

      if (ret == 0)
        {
          break;
        }
      if (ret > 0)
        {
          retval += ret;
        }
      else
        {
          return -1;
        }
    }

  return retval;
}
bool
LocalStreamSocketFd::Isatty (void) const
{
  return false;
}
int
LocalStreamSocketFd::Setsockopt (int level, int optname, const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  if (level != SOL_SOCKET)
    {
      current->err = EINVAL;
      return -1;
    }

  switch (optname)
    {

    case SO_LINGER:
      {
        if ((0 == optval) || (0 == optlen) || (optlen < sizeof(struct linger)))
          {
            current->err = EINVAL;
            return -1;
          }
        if (((struct linger *) optval)->l_onoff != 0)
          {
            m_linger = (((struct linger *) optval)->l_linger);
          }
        else
          {
            m_linger = 0;
          }
      }
      break;

    case SO_PASSCRED:
      {
        NS_LOG_DEBUG ("LocalStreamSocketFd SO_PASSCRED NOT IMPLEMENTED");
        current->err = EINVAL;
        return -1;
      }

    case SO_RCVBUF:
    case SO_SNDBUF:
      {
        NS_LOG_DEBUG ("LocalStreamSocketFd SO_RCVBUF and SO_SNDBUF ignored.");
        return 0;
      }
    case SO_RCVLOWAT:
      {
        NS_LOG_DEBUG ("LocalStreamSocketFd SO_RCVLOWAT ignored.");
        return 0;
      }
    case SO_SNDLOWAT:
      {
        NS_LOG_DEBUG ("LocalStreamSocketFd SO_SNDLOWAT ignored.");
        return 0;
      }

    case SO_RCVTIMEO:
      {
        if ((0 == optval) || (0 == optlen) || (optlen < sizeof(struct timeval)))
          {
            current->err = EINVAL;
            return -1;
          }

        m_recvTimeout = UtilsTimevalToTime ((struct timeval *) optval);

        return 0;
      }

    case SO_SNDTIMEO:
      {
        if ((0 == optval) || (0 == optlen) || (optlen < sizeof(struct timeval)))
          {
            current->err = EINVAL;
            return -1;
          }

        m_sendTimeout = UtilsTimevalToTime ((struct timeval *) optval);

        return 0;
      }

    default:
      break;
    }
  current->err = EINVAL;
  return -1;
}
int
LocalStreamSocketFd::Getsockopt (int level, int optname, void *optval, socklen_t *optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  if (level != SOL_SOCKET)
    {
      current->err = EINVAL;
      return -1;
    }

  switch (optname)
    {

    case SO_LINGER:
      {
        if ((0 == optval) || (0 == optlen) || ((*optlen) < sizeof(struct linger)))
          {
            current->err = EINVAL;
            return -1;
          }
        ((struct linger *) optval)->l_linger = m_linger;

        return 0;
      }
      break;

    case SO_PASSCRED:
      {
        NS_LOG_DEBUG ("LocalStreamSocketFd SO_PASSCRED NOT IMPLEMENTED");
        current->err = EINVAL;
        return -1;
      }

    case SO_RCVBUF:
    case SO_SNDBUF:
      {
        if ((0 == optval) || (0 == optlen) || ((*optlen) < sizeof(int)))
          {
            current->err = EINVAL;
            return -1;
          }

        int *ival = (int*)optval;

        *ival = LOCAL_SOCKET_MAX_BUFFER;

        (*optlen) = sizeof(int);

        return 0;
      }
    case SO_SNDLOWAT:
    case SO_RCVLOWAT:
      {
        if ((0 == optval) || (0 == optlen) || ((*optlen) < sizeof(int)))
          {
            current->err = EINVAL;
            return -1;
          }

        int *ival = (int*)optval;

        *ival = 1;

        (*optlen) = sizeof(int);

        return 0;
      }

    case SO_ACCEPTCONN:
      {
        if ((0 == optval) || (0 == optlen) || ((*optlen) < sizeof(int)))
          {
            current->err = EINVAL;
            return -1;
          }

        int *ival = (int*)optval;

        *ival = ((m_state == LISTENING)||(m_state == ACCEPTING));

        (*optlen) = sizeof(int);

        return 0;
      }

    case SO_RCVTIMEO:
      {
        if ((0 == optval) || (0 == optlen) || ((*optlen) < sizeof(struct timeval)))
          {
            current->err = EINVAL;
            return -1;
          }

        struct timeval *tv = (struct timeval*)optval;

        *tv = UtilsTimeToTimeval (m_recvTimeout);

        return 0;
      }

    case SO_SNDTIMEO:
      {
        if ((0 == optval) || (0 == optlen) || ((*optlen) < sizeof(struct timeval)))
          {
            current->err = EINVAL;
            return -1;
          }

        struct timeval *tv = (struct timeval*)optval;

        *tv = UtilsTimeToTimeval (m_sendTimeout);

        return 0;
      }

    case SO_TYPE:
      {
        if ((0 == optval) || (0 == optlen) || ((*optlen) < sizeof(int)))
          {
            current->err = EINVAL;
            return -1;
          }

        int *ival = (int*)optval;

        *ival = SOCK_STREAM;

        (*optlen) = sizeof(int);

        return 0;
      }

    default:
      {
        current->err = EINVAL;
        return -1;
      }
    }

  return -1;
}
int
LocalStreamSocketFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this);

  if ((0 == name) || (0 == namelen))
    {
      Current ()->err = EINVAL;
      return -1;
    }
  struct sockaddr_un address;

  memset (&address, 0, sizeof(sockaddr_un));
  address.sun_family = AF_UNIX;
  if ((m_bindPath.length () > 0)&&(m_state != CLOSED))
    {
      std::string root = UtilsGetRealFilePath ("/");
      std::string virtualPath = m_bindPath.substr (root.length () - 1);

      memcpy (&address.sun_path, virtualPath.c_str (), std::min (108, (int)virtualPath.length ()));
    }

  socklen_t len = std::min ((int) *namelen, (int) sizeof(struct sockaddr_un));

  memcpy (name, &address, len);

  *namelen = len;

  return 0;
}
int
LocalStreamSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this);

  if ((0 == name) || (0 == namelen))
    {
      Current ()->err = EINVAL;
      return -1;
    }
  if ((m_state != CONNECTED) && (m_state != REMOTECLOSED))
    {
      Current ()->err = ENOTCONN;
      return -1;
    }
  struct sockaddr_un address;

  memset (&address, 0, sizeof(sockaddr_un));
  address.sun_family = AF_UNIX;
  if (m_connectPath.length () > 0)
    {
      std::string root = UtilsGetRealFilePath ("/");
      std::string virtualPath = m_connectPath.substr (root.length () - 1);

      memcpy (&address.sun_path, virtualPath.c_str (), std::min (108, (int)virtualPath.length ()));
    }

  socklen_t len = std::min ((int) *namelen, (int) sizeof(struct sockaddr_un));

  memcpy (name, &address, len);

  *namelen = len;

  return 0;
}
int
LocalStreamSocketFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  NS_LOG_FUNCTION (this);

  if (my_addr->sa_family != AF_UNIX)
    {
      Current ()->err = EINVAL;
      return -1;
    }

  if (m_state != CREATED)
    {
      return -1;
    }

  std::string realPath = UtilsGetRealFilePath (std::string (((struct sockaddr_un*) my_addr)->sun_path));

  struct sockaddr_un realAddr;

  memset (&realAddr, 0, sizeof(realAddr));

  realAddr.sun_family = my_addr->sa_family;

  memcpy (&(realAddr.sun_path), realPath.c_str (), std::min (108, (int) realPath.length ()));

  int realFd = ::socket (AF_UNIX, SOCK_STREAM, 0);

  UtilsEnsureAllDirectoriesExist (realPath);

  int ret = ::bind (realFd, (struct sockaddr *) &realAddr, sizeof(realAddr));

  if (0 == ret)
    {
      m_state = BINDED;
      m_bindPath = realPath;
      m_factory->RegisterBinder (m_bindPath, this);
    }
  else
    {
      Current ()->err = errno;
    }
  close (realFd);

  return ret;
}
int
LocalStreamSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen);
  NS_ASSERT (current != 0);

  // first seek bind one
  if (0 == m_factory)
    {
      Current ()->err = EINVAL;
      return -1;
    }
  std::string realPath = UtilsGetRealFilePath (std::string (((struct sockaddr_un*) my_addr)->sun_path));
  LocalSocketFd* l1 = m_factory->FindBinder (realPath, this->GetTypeId ());
  LocalStreamSocketFd *listener =  (0 == l1) ? 0 : dynamic_cast<LocalStreamSocketFd*> (l1);

  if (0 != listener)
    {
      // There is a good listener ...
      if (listener->IsAccepting () || listener->IsListening ())
        {
          if ((!listener->IsAccepting ()) && (m_statusFlags & O_NONBLOCK))
            {
              // Socket do not want to wait
              Current ()->err = EAGAIN;
              return -1;
            }
          m_state = CONNECTING;

          WaitQueueEntryTimeout *wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, GetRecvTimeout ());
          AddWaitQueue (wq, true);
          listener->ConnectionCreated (this, wq);
          WaitPoint::Result res = WaitPoint::OK;
          if (m_state == CONNECTING)
            {
              res = wq->Wait ();
            }
          RemoveWaitQueue (wq, true);
          delete (wq);
          wq = 0;

          if (WaitPoint::INTERRUPTED == res)
            {
              UtilsDoSignal ();
            }
          if ((m_state == CONNECTED) || (m_state == REMOTECLOSED))
            {
              m_connectPath = realPath;

              return 0;
            }
          else
            {
              listener->RemoveFromQueue (this);
              current->err = EAGAIN;
            }
        }
    }
  else
    {
      current->err = ENOENT;
    }

  return -1;
}
int
LocalStreamSocketFd::Listen (int backlog)
{
  if (m_state != BINDED)
    {
      Current ()->err = EINVAL;
      return -1;
    }
  else
    {
      m_backLog = backlog;
      m_state = LISTENING;
      m_factory->RegisterBinder (m_bindPath, this);
      return 0;
    }
}
int
LocalStreamSocketFd::Shutdown (int how)
{
  NS_LOG_FUNCTION (this << how);
  if ((m_state != CONNECTED) && (m_state != REMOTECLOSED))
    {
      Current ()->err = ENOTCONN;
      return -1;
    }
  switch (how)
    {
    case SHUT_RD:
      {
        m_shutRead = true;
      }
      break;

    case SHUT_WR:
      {
        m_shutWrite = true;
      }
      break;

    case SHUT_RDWR:
      {
        m_shutWrite = true;
        m_shutRead = true;
      }
      break;
    }
  return 0;
}
int
LocalStreamSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen);
  NS_ASSERT (current != 0);

  if (m_state != LISTENING)
    {
      Current ()->err = EINVAL;
      return -1;
    }

  WaitQueueEntryTimeout *wq = 0;

  NS_LOG_DEBUG ("Accept: incoming queue len:" << m_cnxQueue.size ());

  while (m_cnxQueue.empty ())
    {
      if (m_statusFlags & O_NONBLOCK)
        {
          // Socket do not want to wait
          Current ()->err = EAGAIN;
          RETURNFREE (-1);
        }
      m_state = ACCEPTING;
      if (!wq)
        {
          wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, GetRecvTimeout ());
        }
      AddWaitQueue (wq, true);
      WaitPoint::Result res = wq->Wait ();
      RemoveWaitQueue (wq, true);

      switch (res)
        {
        case WaitPoint::OK:
          break;
        case WaitPoint::INTERRUPTED:
          {
            UtilsDoSignal ();
            current->err = EINTR;
            RETURNFREE (-1);
          }
        case WaitPoint::TIMEOUT:
          {
            current->err = EAGAIN;
            RETURNFREE (-1);
          }
        }
    }

  m_state = LISTENING;

  while (!m_cnxQueue.empty ())
    {
      LocalStreamSocketFd* first = m_cnxQueue.front ();
      m_cnxQueue.pop_front ();
      // create an fd for the socket.
      int fd = UtilsAllocateFd ();
      if (fd == -1)
        {
          first->Close ();
          current->err = EMFILE;
          RETURNFREE (-1);
        }

      if (first->InternalConnect ())
        {
          LocalStreamSocketFd *socket = new LocalStreamSocketFd (first, m_bindPath);
          socket->IncFdCount ();
          current->process->openFiles[fd] = new FileUsage (fd, socket);

          first->SetPeer (socket);

          if ((0 != my_addr) && (0 != addrlen) && ((*addrlen) >= sizeof (sa_family_t)))
            {
              my_addr->sa_family = AF_UNIX;
              *addrlen = sizeof (sa_family_t);
            }

          RETURNFREE (fd);
        }

    }

  RETURNFREE (-1);
}

bool
LocalStreamSocketFd::CanRecv (void) const
{
  switch (m_state)
    {
    case CONNECTED:
    case REMOTECLOSED:
      {
        return (m_readBufferSize > 0);
      }

    case LISTENING:
    case ACCEPTING:
      {
        return m_cnxQueue.size () > 0;
      }

    default:
      return 0;
    }
}
bool
LocalStreamSocketFd::CanSend (void) const
{
  return ((CONNECTED == m_state) && (0 != m_peer) && (m_peer->m_readBufferSize < LOCAL_SOCKET_MAX_BUFFER))
         || (CONNECTED != m_state);
}
bool
LocalStreamSocketFd::HangupReceived (void) const
{
  NS_LOG_FUNCTION (this << " state:" << m_state);
  return (REMOTECLOSED == m_state);
}
bool
LocalStreamSocketFd::InternalConnect (void)
{
  NS_LOG_FUNCTION (this << " state:" << m_state);
  if (CONNECTING != m_state)
    {
      return false;
    }

  m_state = CONNECTED;

  short pi = POLLIN | POLLOUT;
  WakeWaiters (&pi);

  return true;
}

bool
LocalStreamSocketFd::IsAccepting (void)
{
  return ACCEPTING == m_state;
}

bool
LocalStreamSocketFd::IsListening (void)
{
  return LISTENING == m_state;
}

void
LocalStreamSocketFd::ConnectionCreated (LocalStreamSocketFd* sock, WaitQueueEntryTimeout *wq)
{
  NS_LOG_FUNCTION (sock);
  m_cnxQueue.push_back (sock);

  short pi = POLLIN;
  WakeWaiters (&pi);
}

void
LocalStreamSocketFd::RemoveFromQueue (LocalStreamSocketFd* sock)
{
  NS_LOG_FUNCTION (sock);
  m_cnxQueue.remove (sock);
}

void
LocalStreamSocketFd::SetPeer (LocalStreamSocketFd *sock)
{
  m_peer = sock;
}

void
LocalStreamSocketFd::PeerClosed ()
{
  NS_LOG_FUNCTION (this);
  m_peer = 0;
  if (m_state == CONNECTED)
    {
      m_state = REMOTECLOSED;

      short ph = POLLHUP;
      WakeWaiters (&ph);
    }
}

void
LocalStreamSocketFd::ClearAll (bool andWakeUp)
{
  m_state = CLOSED;
  m_peer = 0;
  m_factory = 0;
  m_bindPath = m_connectPath = "";
  m_cnxQueue.clear ();

  ClearReadBuffer ();

  if (andWakeUp)
    {
      short ph = POLLHUP;
      WakeWaiters (&ph);
    }
}

bool
LocalStreamSocketFd::IsShutWrite (void) const
{
  return m_shutWrite;
}

bool
LocalStreamSocketFd::IsClosed (void) const
{
  return CLOSED == m_state;
}

int
LocalStreamSocketFd::Poll (PollTable* ptable)
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
  if (IsClosed () || HangupReceived ())
    {
      ret |= POLLHUP;
    }

  if (ptable)
    {
      ptable->PollWait (this);
    }

  return ret;
}

} // namespace ns3
