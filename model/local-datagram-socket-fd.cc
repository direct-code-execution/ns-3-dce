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
#include "local-datagram-socket-fd.h"
#include "local-socket-fd-factory.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h> // for MMAP_FAILED
#include <sys/un.h>
#include "unix-fd.h"
#include <exception>
#include <poll.h>
#include "wait-queue.h"
#include <unistd.h>

NS_LOG_COMPONENT_DEFINE ("DceLocalDatagramSocketFd");

namespace ns3 {
TypeId
LocalDatagramSocketFd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LocalDatagramSocketFd").SetParent<LocalSocketFd> ();

  return tid;
}
TypeId
LocalDatagramSocketFd::GetInstanceTypeId (void) const
{
  return LocalDatagramSocketFd::GetTypeId ();
}
LocalDatagramSocketFd::LocalDatagramSocketFd (Ptr<LocalSocketFdFactory> f)
  : m_state (CREATED),
    m_peer (0)
{
  m_factory = f;
}
LocalDatagramSocketFd::~LocalDatagramSocketFd ()
{
  ClearAll (false);
}

void
LocalDatagramSocketFd::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ClearAll (false);
}

int
LocalDatagramSocketFd::Close (void)
{
  NS_LOG_FUNCTION (this);

  switch (m_state)
    {
    case CONNECTED:
    case BINDED:
      ClearAll (true);
      break;

    default:
      ClearAll (false);
      break;
    }

  return 0;
}
ssize_t
LocalDatagramSocketFd::Write (const void *buf, size_t count)
{
  NS_LOG_FUNCTION (this << "state:" << m_state);

  if (CONNECTED != m_state)
    {

      Current ()->err =  (REMOTECLOSED == m_state) ? ECONNREFUSED : ENOTCONN;

      return -1;
    }

  return Write2Peer (buf, count, m_peer);
}

ssize_t
LocalDatagramSocketFd::Read (void *buf, size_t count, bool noWait, bool peek)
{
  NS_LOG_FUNCTION (this << "state:" << m_state);
  WaitQueueEntryTimeout *wq = 0;

  while (BINDED == m_state)
    {
      // Is There some already received data ?
      if (m_readBuffer.size () > 0)
        {
          ssize_t ret = ReadData ((uint8_t*) buf, count, peek);

          if ((ret > 0)&&(0 != m_peer))
            {
              // WakeUP peer because we made room
              short po = POLLOUT;
              m_peer->WakeWaiters (&po);
            }
          RETURNFREE (ret);
        }

      // Should Wait data ?
      if (noWait || m_shutRead)
        {
          RETURNFREE (0);
        }
      if (m_statusFlags & O_NONBLOCK)
        {
          // Socket do not want to wait
          Current ()->err = EAGAIN;
          RETURNFREE (-1);
        }
      // Before wait verify not closed by another thread !
      if (BINDED != m_state)
        {
          Current ()->err = ENOTCONN;
          RETURNFREE (-1);
        }
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
          if (0 == m_readBuffer.size ())
            {
              Current ()->err = EINTR;
              RETURNFREE (-1);
            }
          break;

        case WaitPoint::TIMEOUT:
          if (0 == m_readBuffer.size ())
            {
              Current ()->err = EAGAIN;
              RETURNFREE (-1);
            }
          break;
        }
    }

  Current ()->err = ENOTCONN;
  RETURNFREE (-1);
}

int
LocalDatagramSocketFd::Setsockopt (int level, int optname, const void *optval, socklen_t optlen)
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
    case SO_PASSCRED:
      {
        NS_LOG_DEBUG ("LocalDatagramSocketFd SO_PASSCRED NOT IMPLEMENTED");
        current->err = EINVAL;
        return -1;
      }

    case SO_RCVBUF:
    case SO_SNDBUF:
      {
        NS_LOG_DEBUG ("LocalDatagramSocketFd SO_RCVBUF and SO_SNDBUF ignored.");
        return 0;
      }
    case SO_RCVLOWAT:
      {
        NS_LOG_DEBUG ("LocalDatagramSocketFd SO_RCVLOWAT ignored.");
        return 0;
      }
    case SO_SNDLOWAT:
      {
        NS_LOG_DEBUG ("LocalDatagramSocketFd SO_SNDLOWAT ignored.");
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
LocalDatagramSocketFd::Getsockopt (int level, int optname, void *optval, socklen_t *optlen)
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
    case SO_PASSCRED:
      {
        NS_LOG_DEBUG ("LocalDatagramSocketFd SO_PASSCRED NOT IMPLEMENTED");
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

        *ival = SOCK_DGRAM;

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
LocalDatagramSocketFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
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
LocalDatagramSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
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
LocalDatagramSocketFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
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

  int realFd = ::socket (AF_UNIX, SOCK_DGRAM, 0);

  UtilsEnsureAllDirectoriesExist (realPath);

  int ret = ::bind (realFd, (struct sockaddr *) &realAddr, sizeof(realAddr));
  NS_LOG_FUNCTION (this << "Bind: " << ret << "errno:" << errno);
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
LocalDatagramSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
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
  switch (m_state)
    {
    default:
    case CREATED:
      break;

    case BINDED:
      {
        Current ()->err = EADDRINUSE;
        return -1;
      }

    case CONNECTED:
      {
        Current ()->err = EISCONN;
        return -1;
      }
    }

  std::string realPath = UtilsGetRealFilePath (std::string (((struct sockaddr_un*) my_addr)->sun_path));

  LocalSocketFd* l1 = m_factory->FindBinder (realPath, this->GetTypeId ());
  LocalDatagramSocketFd *listener =  dynamic_cast<LocalDatagramSocketFd*> (l1);

  if (0 != listener)
    {
      if (!listener->IsBinded ())
        {
          Current ()->err = ECONNREFUSED;
          return -1;
        }
      m_state = CONNECTED;
      m_connectPath = realPath;
      m_peer = listener;

      listener->AddConnected (this);

      return 0;
    }
  else
    {
      current->err = ECONNREFUSED;
    }

  return -1;
}

int
LocalDatagramSocketFd::Listen (int backlog)
{
  NS_LOG_FUNCTION (this);
  Current ()->err = EOPNOTSUPP;
  return -1;
}

int
LocalDatagramSocketFd::Shutdown (int how)
{
  NS_LOG_FUNCTION (this << how);
  if (m_state == CLOSED)
    {
      Current ()->err = EPIPE;
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
LocalDatagramSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  NS_LOG_FUNCTION (this);
  Current ()->err = EOPNOTSUPP;
  return -1;
}

bool
LocalDatagramSocketFd::CanRecv (void) const
{
  switch (m_state)
    {
    case CLOSED:
      return 1;
    case CREATED:
      return 1;

    case BINDED:
      return m_readBufferSize > 0;

    case REMOTECLOSED:
    case CONNECTED:
      return 0;

    }
  return 0;
}

bool
LocalDatagramSocketFd::CanSend (void) const
{
  return ((CONNECTED == m_state) && (0 != m_peer) && (m_peer->m_readBufferSize < LOCAL_SOCKET_MAX_BUFFER))
         || (CONNECTED != m_state);
}

bool
LocalDatagramSocketFd::HangupReceived (void) const
{
  NS_LOG_FUNCTION (this << " state:" << m_state);
  return (REMOTECLOSED == m_state);
}

ssize_t
LocalDatagramSocketFd::Recvmsg (struct msghdr *msg, int flags)
{
  NS_LOG_FUNCTION (this);

  if (0 == msg)
    {
      Current ()->err = EINVAL;
      return -1;
    }

  if (CONNECTED == m_state)
    {
      Current ()->err = ENOTCONN;
      return -1;
    }

  size_t retval = 0;
  for (uint32_t i = 0; i < msg->msg_iovlen; ++i)
    {
      uint8_t *buf = (uint8_t *) msg->msg_iov[i].iov_base;
      ssize_t len = msg->msg_iov[i].iov_len;
      msg->msg_namelen = 0;

      ssize_t ret = Read (buf, len, (retval > 0) || (flags & MSG_DONTWAIT), (flags & MSG_PEEK));
      NS_LOG_FUNCTION (this << "len:" << len << " read-> " << ret);
      if (ret <= 0)
        {
          NS_LOG_FUNCTION (this << "4");

          break;
        }
      if (ret > 0)
        {
          retval += ret;
        }
    }

  if (retval > 0)
    {
      if ((!(flags & MSG_PEEK) && (m_readBufferSize > 0))
          || ((flags & MSG_PEEK) && (m_readBufferSize > retval)))
        {
          msg->msg_flags |= MSG_TRUNC;
        }
      return retval;
    }

  if (flags & MSG_DONTWAIT)
    {
      Current ()->err = EAGAIN;
    }

  return -1;
}

ssize_t
LocalDatagramSocketFd::Sendmsg (const struct msghdr *msg, int flags)
{
  NS_LOG_FUNCTION (this << m_state);
  LocalDatagramSocketFd* listener = 0;
  LocalSocketFd* l1 = 0;
  if (0 == msg)
    {
      Current ()->err = EINVAL;
      return -1;
    }

  if (CONNECTED == m_state)
    {
      listener = m_peer;
    }
  else if (CREATED == m_state)
    {
      if ((msg->msg_namelen < sizeof (struct sockaddr_un)) || (0 == msg->msg_name))
        {
          Current ()->err = EINVAL;
          return -1;
        }
      struct sockaddr_un *addr = (struct sockaddr_un*) msg->msg_name;
      // Seek peer

      std::string realPath = UtilsGetRealFilePath (std::string (addr->sun_path));

      // Do a write
      l1 = m_factory->FindBinder (realPath, this->GetTypeId ());
      listener = dynamic_cast<LocalDatagramSocketFd*> (l1);
    }
  else if (REMOTECLOSED == m_state)
    {
      Current ()->err = ECONNREFUSED;
      return -1;
    }
  else
    {
      Current ()->err = ENOTCONN;
      return -1;
    }

  if (0 == listener)
    {
      Current ()->err = EACCES;
      return -1;
    }

  ssize_t retval = 0;
  WaitQueueEntryTimeout *wq = 0;

  for (uint32_t i = 0; i < msg->msg_iovlen; ++i)
    {
      uint8_t *buf = (uint8_t *) msg->msg_iov[i].iov_base;
      ssize_t len = msg->msg_iov[i].iov_len;
      int ret = 0;

      while (m_state < REMOTECLOSED)
        {
          ret = listener->DoRecvPacket (buf, len);

          switch (ret)
            {
            case 0: // NO ROOM
              {
                if (flags & MSG_DONTWAIT)
                  {
                    Current ()->err = EAGAIN;
                    RETURNFREE (-1);
                  }
                if (!wq)
                  {
                    wq = new WaitQueueEntryTimeout (POLLOUT | POLLHUP, GetSendTimeout ());
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
                      Current ()->err = EINTR;
                      RETURNFREE (-1);
                    }
                    break;
                  case WaitPoint::TIMEOUT:
                    {
                      Current ()->err = EAGAIN;
                      RETURNFREE (-1);
                    }
                    break;
                  }

                if (m_state >= REMOTECLOSED)
                  {
                    Current ()->err = ECONNREFUSED;
                    RETURNFREE (-1);
                  }
                continue;
              }

            case - 1: // NOMEM !
              RETURNFREE (-1);

            case - 2: // CLOSED !
              {
                Current ()->err = ECONNREFUSED;
                RETURNFREE (-1);
              }
            }

          if (ret > 0)
            {
              retval += ret;
              break;
            }
        }
    }
  RETURNFREE (retval);
}

bool
LocalDatagramSocketFd::IsBinded (void)
{
  return m_state == BINDED;
}

ssize_t
LocalDatagramSocketFd::Write2Peer (const void *buf, size_t count, LocalDatagramSocketFd* peer)
{
  NS_LOG_FUNCTION (this);

  // Verify peer state
  if (0 == peer || !peer->IsBinded ())
    {
      Current ()->err =  EPIPE;
      return -1;
    }

  ssize_t lg = m_peer->DoRecvPacket ((uint8_t*) buf, count);

  if (-2 == lg)
    {
      Current ()->err =  EPIPE;
      return -1;
    }

  return lg;
}

void
LocalDatagramSocketFd::UnBind (void)
{
  if ((m_bindPath.length () > 0) && (BINDED == m_state))
    {
      m_factory->UnRegisterBinder (m_bindPath);
      m_bindPath = "";
      m_state = CREATED;
    }
}

ssize_t
LocalDatagramSocketFd::Read (void *buf, size_t count)
{
  return Read (buf, count, false, false);
}
bool
LocalDatagramSocketFd::IsClosed (void) const
{
  NS_LOG_FUNCTION (this << m_state);
  return m_state >= REMOTECLOSED;
}

void
LocalDatagramSocketFd::AddConnected (LocalDatagramSocketFd* newone)
{
  NS_LOG_FUNCTION (this << m_state);
  m_myPeers.insert (newone);
}

void
LocalDatagramSocketFd::RemoveConnected (LocalDatagramSocketFd *freeOne, bool andWakeUp)
{
  NS_LOG_FUNCTION (this << m_state);
  switch (m_state)
    {
    case BINDED:
      {
        m_myPeers.erase (freeOne);
        if (andWakeUp)
          {
            int pi = POLLHUP;
            WakeWaiters (&pi);
          }
      }
      break;

    case CONNECTED:
      {
        m_peer = 0;
        m_state = REMOTECLOSED;
        if (andWakeUp)
          {
            int pi = POLLHUP;
            WakeWaiters (&pi);
          }
      }
      break;
    default:
        NS_LOG_WARN ("Unhandled case");
        break;
    }
}

void
LocalDatagramSocketFd::ClearAll (bool andWakeUp)
{
  m_state = CLOSED;
  if (0 != m_peer)
    {
      LocalDatagramSocketFd* freeOne = m_peer;

      m_peer = 0;
      freeOne->RemoveConnected (this, andWakeUp);
    }
  std::set<LocalDatagramSocketFd*> toFree = m_myPeers;

  m_myPeers.clear ();

  std::set<LocalDatagramSocketFd*>::iterator i;

  for (i = toFree.begin (); i != toFree.end (); ++i)
    {
      (*i)->RemoveConnected (this, andWakeUp);
    }
  toFree.clear ();

  if (m_bindPath.length () > 0)
    {
      m_factory->UnRegisterBinder (m_bindPath);
      m_bindPath = "";
    }

  ClearReadBuffer ();

  if (andWakeUp)
    {
      //    WakeupRecv ();
      int pi = POLLHUP;
      WakeWaiters (&pi);
    }
}
int
LocalDatagramSocketFd::Poll (PollTable* ptable)
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
} // namespace ns3
