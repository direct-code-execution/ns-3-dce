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
#include "local-socket-fd-factory.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h> // for MMAP_FAILED
#include <sys/un.h>
#include "waiter.h"
#include "unix-fd.h"
#include <exception>

NS_LOG_COMPONENT_DEFINE("LocalSocketFd");

namespace ns3
{

LocalSocketFd::LocalSocketFd (Ptr<LocalSocketFdFactory> f) :
  m_factory (f), m_bindPath (""), m_connectPath (""), m_state (CREATED), m_backLog (0),
  m_readBufferSize (0), m_linger (0), m_shutRead(false), m_shutWrite(false),  m_statusFlags (0)
{
}

LocalSocketFd::LocalSocketFd (LocalSocketFd *peer, std::string bindPath) :
  m_factory (0), m_bindPath (bindPath), m_connectPath (""), m_state (CONNECTED), m_backLog (0),
  m_readBufferSize (0), m_peer (peer), m_linger (0), m_shutRead(false), m_shutWrite(false),
  m_statusFlags (0)
{
}

LocalSocketFd::~LocalSocketFd ()
{
  ClearAll (false);
}

int
LocalSocketFd::Close (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this);
  NS_ASSERT (current != 0);

  NS_LOG_DEBUG("m_state: " << m_state);

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
        NS_ASSERT( 0 != m_factory);
        m_factory->UnRegisterBinder (m_bindPath);

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
LocalSocketFd::Write (const void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << buf << count);
  NS_ASSERT (current != 0);

  NS_LOG_DEBUG("Write enter state " << m_state);

  if (m_state != CONNECTED)
    {
      if (m_state == REMOTECLOSED)
        {
          return 0;
        }
      current->err = ENOTCONN;
      return -1;
    }
  if ( m_shutWrite )
    {
      current->err = EPIPE;
      return -1;
    }

  NS_ASSERT(m_peer != 0);

  size_t tot = 0;
  Waiter waiter;
  waiter.SetTimeout (GetSendTimeout ());

  do
    {
      ssize_t lg = m_peer->DoRecvPacket ((uint8_t*) buf + tot, count - tot);
      NS_LOG_FUNCTION (this <<  " lg:" << lg);
      if (0 == lg)
        {
          if (  m_statusFlags & O_NONBLOCK  )
            {
              // Socket do not want to wait
              Current ()->err = EAGAIN;
              return -1;
            }
          SetSendWaiter (&waiter);
          Waiter::Result result = waiter.Wait ();
          SetSendWaiter (0);
          switch (result)
            {
            case Waiter::OK:
              break;
            case Waiter::INTERRUPTED:
              UtilsDoSignal ();
              if (tot != 0)
                {
                  // this is a short write
                  return tot;
                }
              else
                {
                  current->err = EINTR;
                  return -1;
                }
              break;
            case Waiter::TIMEOUT:
              if (tot != 0)
                {
                  // this is a short write
                  return tot;
                }
              else
                {
                  current->err = EAGAIN;
                  return -1;
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
              if ( 0 == tot )
                {
                  current->err = EPIPE;
                  return -1;
                }
              break;
            }
          current->err = ENOMEM;
          return -1;
        }
    }
  while ((tot < count) && (m_state == CONNECTED));

  if ((0 == tot) && (m_state != CONNECTED) )
    {
      current->err = EPIPE;
      return -1;
    }

  return tot;
}
ssize_t
LocalSocketFd::Read (void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << buf << count);
  NS_ASSERT (current != 0);

  NS_LOG_DEBUG("Read enter state: "<< m_state);

  if ((m_state != CONNECTED) && (m_state != REMOTECLOSED))
    {
      current->err = EINVAL;
      return -1;
    }

  size_t filled = 0;
  Waiter waiter;
  waiter.SetTimeout (GetRecvTimeout ());

  while ((filled < count) && ((m_state == CONNECTED) || ((REMOTECLOSED == m_state) && (m_readBufferSize > 0))))
    {
      size_t lg = ReadData ((uint8_t*) buf + filled, count - filled);
      if (0 == lg)
        {
          if (  m_statusFlags & O_NONBLOCK  )
            {
              // Socket do not want to wait
              Current ()->err = EAGAIN;
              return -1;
            }
          if ( ( 0 != m_peer) && ( m_peer->IsShutWrite()) ) return 0;
          // Nothing should wait
          SetRecvWaiter (&waiter);
          Waiter::Result result = waiter.Wait ();
          SetRecvWaiter (0);
          switch (result)
            {
            case Waiter::OK:
              break;
            case Waiter::INTERRUPTED:
              UtilsDoSignal ();
              if (filled != 0)
                {
                  // this is a short read
                  return filled;
                }
              else
                {
                  current->err = EINTR;
                  return -1;
                }
              break;
            case Waiter::TIMEOUT:
              if (filled != 0)
                {
                  // this is a short read
                  return filled;
                }
              else
                {
                  current->err = EAGAIN;
                  return -1;
                }
              break;
            }
        }
      else if (lg > 0)
        {
          filled += lg;
        }
      else
        {
          current->err = ENOMEM;
          return -1;
        }
    }

  return filled;
}
ssize_t
LocalSocketFd::Recvmsg (struct msghdr *msg, int flags)
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
LocalSocketFd::Sendmsg (const struct msghdr *msg, int flags)
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
LocalSocketFd::Isatty (void) const
{
  return false;
}
int
LocalSocketFd::Setsockopt (int level, int optname, const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << level << optname << optval << optlen);NS_ASSERT (current != 0);

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
        NS_LOG_DEBUG("LocalSocketFd SO_PASSCRED NOT IMPLEMENTED");
        current->err = EINVAL;
        return -1;
      }

    case SO_RCVBUF:
    case SO_SNDBUF:
      {
        NS_LOG_DEBUG("LocalSocketFd SO_RCVBUF and SO_SNDBUF ignored.");
        return 0;
      }
    case SO_RCVLOWAT:
      {
        NS_LOG_DEBUG("LocalSocketFd SO_RCVLOWAT ignored.");
        return 0;
      }
    case SO_SNDLOWAT:
      {
        NS_LOG_DEBUG("LocalSocketFd SO_SNDLOWAT ignored.");
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

    default: break;
    }
  current->err = EINVAL;
  return -1;
}
int
LocalSocketFd::Getsockopt (int level, int optname, void *optval, socklen_t *optlen)
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
        NS_LOG_DEBUG("LocalSocketFd SO_PASSCRED NOT IMPLEMENTED");
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
        if ((0 == optval) || (0 == optlen) || ( (*optlen) < sizeof(struct timeval)))
          {
            current->err = EINVAL;
            return -1;
          }

        struct timeval *tv = (struct timeval*)optval;

        *tv = UtilsTimeToTimeval(m_recvTimeout);

        return 0;
      }

    case SO_SNDTIMEO:
      {
        if ((0 == optval) || (0 == optlen) || ( (*optlen) < sizeof(struct timeval)))
          {
            current->err = EINVAL;
            return -1;
          }

        struct timeval *tv = (struct timeval*)optval;

        *tv = UtilsTimeToTimeval(m_sendTimeout);

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
LocalSocketFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this);

  if ( (0 == name) || (0 == namelen) )
    {
      Current() -> err = EINVAL;
      return -1;
    }
  struct sockaddr_un address;

  memset (&address , 0, sizeof(sockaddr_un));
  address.sun_family = AF_UNIX;
  if ((m_bindPath.length() > 0)&&(m_state != CLOSED))
    {
      std::string root = UtilsGetRealFilePath ("/");
      std::string virtualPath = m_bindPath.substr(root.length()-1);

      memcpy(&address.sun_path, virtualPath.c_str(), std::min(108, (int)virtualPath.length()));
    }

  socklen_t len = std::min( (int) *namelen, (int) sizeof(struct sockaddr_un) );

  memcpy(name, &address, len);

  *namelen = len;

  return 0;
}
int
LocalSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this);

  if ( (0 == name) || (0 == namelen) )
    {
      Current() -> err = EINVAL;
      return -1;
    }
  if ( (m_state != CONNECTED) && (m_state != REMOTECLOSED) )
    {
      Current() -> err = ENOTCONN;
      return -1;
    }
  struct sockaddr_un address;

  memset (&address , 0, sizeof(sockaddr_un));
  address.sun_family = AF_UNIX;
  if (m_connectPath.length() > 0)
    {
      std::string root = UtilsGetRealFilePath ("/");
      std::string virtualPath = m_connectPath.substr(root.length()-1);

      memcpy(&address.sun_path, virtualPath.c_str(), std::min(108, (int)virtualPath.length()));
    }

  socklen_t len = std::min( (int) *namelen, (int) sizeof(struct sockaddr_un) );

  memcpy(name, &address, len);

  *namelen = len;

  return 0;
}
int
LocalSocketFd::Ioctl (int request, char *argp)
{
  Current ()->err = EINVAL;
  return -1;
}
int
LocalSocketFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  NS_LOG_FUNCTION (this);

  if (my_addr->sa_family != AF_UNIX)
    {
      Current ()->err = EINVAL;
      return -1;
    }

  if (m_state != CREATED)
    return -1;

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
LocalSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen);NS_ASSERT (current != 0);

  // first seek bind one
  if (0 == m_factory)
    {
      Current ()->err = EINVAL;
      return -1;
    }
  std::string realPath = UtilsGetRealFilePath (std::string (((struct sockaddr_un*) my_addr)->sun_path));

  Ptr<LocalSocketFd> listener = m_factory->FindBinder (realPath);

  if (0 != listener)
    {
      // There is a good listener ...
      if (listener->IsAccepting () || listener->IsListening ())
        {
          if ( ( ! listener->IsAccepting () ) && (  m_statusFlags & O_NONBLOCK  ) )
            {
              // Socket do not want to wait
              Current ()->err = EAGAIN;
              return -1;
            }
          m_state = CONNECTING;

          Waiter waiter;
          SetRecvWaiter (&waiter);
          waiter.SetTimeout (GetRecvTimeout ());
          listener->ConnectionCreated (this);
          Waiter::Result result = waiter.Wait ();
          SetRecvWaiter (0);

          if (Waiter::INTERRUPTED == result)
            {
              UtilsDoSignal ();
            }

          if (( m_state == CONNECTED )||( m_state == REMOTECLOSED ) )
            {
              m_connectPath = realPath;

              return 0;
            }
          else
            {
              NS_LOG_DEBUG("Local Socket Connect Timeout should remove from queue .... state:"<<m_state);
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
LocalSocketFd::Listen (int backlog)
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
LocalSocketFd::Shutdown (int how)
{
  NS_LOG_FUNCTION (this << how);
  if ( (m_state != CONNECTED) && (m_state != REMOTECLOSED ) )
    {
      Current() -> err = ENOTCONN;
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
LocalSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen);
  NS_ASSERT (current != 0);

  if (m_state != LISTENING)
    {
      Current ()->err = EINVAL;
      return -1;
    }

  Waiter waiter;
  waiter.SetTimeout (GetRecvTimeout ());

  NS_LOG_DEBUG("Accept: incoming queue len:" << m_cnxQueue.size());

  while (m_cnxQueue.empty ())
    {
      if (  m_statusFlags & O_NONBLOCK  )
        {
          // Socket do not want to wait
          Current ()->err = EAGAIN;
          return -1;
        }
      m_state = ACCEPTING;

      SetRecvWaiter (&waiter);
      NS_LOG_DEBUG("Accept: waiting ...");
      Waiter::Result result = waiter.Wait ();
      NS_LOG_DEBUG("Accept: wait result:" << result);
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

  m_state = LISTENING;

  while (!m_cnxQueue.empty ())
    {
      Ptr<LocalSocketFd> first = m_cnxQueue.front ();
      m_cnxQueue.pop_front ();
      if (first->InternalConnect ())
        {
          // create an fd for the socket.
          int fd = UtilsAllocateFd ();
          if (fd == -1)
            {
              current->err = EMFILE;
              return -1;
            }
          LocalSocketFd *socket = new LocalSocketFd (PeekPointer(first), m_bindPath);

          current->process->openFiles.push_back (std::make_pair (fd, socket));

          first->SetPeer (socket);

          return fd;
        }
    }

  return -1;
}
void *
LocalSocketFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  GET_CURRENT (start << length << prot << flags << offset);
  current->err = ENODEV;
  return MAP_FAILED;
}
off64_t
LocalSocketFd::Lseek (off64_t offset, int whence)
{
  GET_CURRENT(offset << whence);
  current->err = ESPIPE;
  return -1;
}
int
LocalSocketFd::Fxstat (int ver, struct ::stat *buf)
{
  GET_CURRENT(ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
LocalSocketFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  GET_CURRENT(ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
LocalSocketFd::Fcntl (int cmd, unsigned long arg)
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
LocalSocketFd::Settime (int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{
  NS_LOG_FUNCTION (this << Current () << flags << new_value << old_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}
int
LocalSocketFd::Gettime (struct itimerspec *cur_value) const
{
  NS_LOG_FUNCTION (this << Current () << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}

bool
LocalSocketFd::CanRecv (void) const
{
  switch(m_state)
  {
    case CONNECTED:
    case REMOTECLOSED:
      {
        return (m_readBufferSize > 0);
      }

    case LISTENING:
    case ACCEPTING:
      {
        return m_cnxQueue.size() > 0;
      }

    default: return 0; // XXX: must be verified with tests with poll and select

  }
}
bool
LocalSocketFd::CanSend (void) const
{
  return ( ( CONNECTED == m_state ) && (0 != m_peer) && ( m_peer->m_readBufferSize < LOCAL_SOCKET_MAX_BUFFER ) )
         || ( CONNECTED != m_state );
}
bool
LocalSocketFd::HangupReceived (void) const
{
  NS_LOG_FUNCTION( this << " state:" << m_state);
  return ( REMOTECLOSED == m_state );
}
bool
LocalSocketFd::InternalConnect (void)
{
  if (CONNECTING != m_state)
    return false;

  m_state = CONNECTED;

  WakeupRecv ();

  return true;
}

Time
LocalSocketFd::GetRecvTimeout (void)
{
  return m_recvTimeout;
}
Time
LocalSocketFd::GetSendTimeout (void)
{
  return m_sendTimeout;
}

bool
LocalSocketFd::IsAccepting (void)
{
  return ACCEPTING == m_state;
}

bool
LocalSocketFd::IsListening (void)
{
  return LISTENING == m_state;
}

void
LocalSocketFd::ConnectionCreated (Ptr<LocalSocketFd> sock)
{
  NS_LOG_FUNCTION (sock);
  //XXX    NS_ASSERT (((int)m_connectionQueue.size ()) < m_backlog);
  m_cnxQueue.push_back (sock);
  WakeupRecv ();
}

void
LocalSocketFd::RemoveFromQueue (Ptr<LocalSocketFd> sock)
{
  NS_LOG_FUNCTION (sock);
  //XXX    NS_ASSERT (((int)m_connectionQueue.size ()) < m_backlog);
  m_cnxQueue.remove (sock);
}

void
LocalSocketFd::SetPeer (LocalSocketFd *sock)
{
  m_peer = sock;
}

// Return :
//   the size readed ,
//  or 0 if no more space available,
//  or -1 if fatal error occurs
ssize_t
LocalSocketFd::DoRecvPacket (uint8_t* buf, size_t len)
{
  NS_LOG_FUNCTION (this << len << " shutRead:" << m_shutRead);

  if (m_shutRead) return -2;
  if (m_readBufferSize >= LOCAL_SOCKET_MAX_BUFFER)
    {
      return 0;
    }

  size_t maxi = LOCAL_SOCKET_MAX_BUFFER - m_readBufferSize;

  int l = std::min (maxi, len);

  struct Buffer myBuf;

  myBuf.data = (uint8_t*) malloc (l);
  if (0 == myBuf.data)
    {
      Thread *current = Current ();
      NS_ASSERT (current != 0);
      current -> err = ENOMEM;
      return -1;
    }
  myBuf.readOffset = 0;
  myBuf.size = l;

  memcpy (myBuf.data, buf, l);

  m_readBuffer.push_back (myBuf);

  m_readBufferSize += l;

  NS_LOG_DEBUG("DoRecvPacket before WakeUpRecv");

  WakeupRecv ();

  return l;
}

size_t
LocalSocketFd::ReadData (uint8_t* buf, size_t len)
{
  size_t fill = 0;
  size_t rest = len;

  while ((fill < len) && (m_readBuffer.size () > 0) && (rest > 0))
    {
      struct Buffer myBuf = m_readBuffer.front ();

      size_t avail = std::min (rest, myBuf.size - myBuf.readOffset);

      if (avail > 0)
        {
          memcpy (buf + fill, myBuf.data + myBuf.readOffset, avail);
          fill += avail;
          m_readBufferSize -= avail;
          myBuf.readOffset += avail;
          rest -= avail;
          if (myBuf.readOffset >= myBuf.size)
            {
              free (myBuf.data);
              myBuf.data = 0;
              m_readBuffer.pop_front ();
            }
        }
      else
        {
          break;
        }
    }

  if ((fill > 0) && (0 != m_peer))
    {
      //m_peer->WakeupRecv();
      NS_LOG_DEBUG("ReadData: wake up my peer sender :)");
      m_peer->WakeupSend (); //XXX a checker :)
    }

  return fill;
}

void
LocalSocketFd::PeerClosed ()
{
  NS_LOG_FUNCTION (this);
  m_peer = 0;
  if (m_state == CONNECTED)
    {
      m_state = REMOTECLOSED;
      WakeupRecv ();
      WakeupSend ();
    }
}

void
LocalSocketFd::ClearAll (bool andWakeUp)
{
  m_state = CLOSED;
  m_peer = 0;
  m_factory = 0;
  m_bindPath = m_connectPath = "";
  m_cnxQueue.clear ();

  for (std::list<Buffer>::iterator i = m_readBuffer.begin (); i != m_readBuffer.end (); ++i)
    {
      if (0 != i->data) free (i->data);
      i->data = 0;
      i->size = 0;
      i->readOffset = 0;
    }
  m_readBuffer.clear ();
  m_readBufferSize = 0;

  if (andWakeUp)
    {
      WakeupRecv ();
      WakeupSend ();
    }
}

bool
LocalSocketFd::IsShutWrite(void) const
{
  return m_shutWrite;
}


} // namespace ns3
