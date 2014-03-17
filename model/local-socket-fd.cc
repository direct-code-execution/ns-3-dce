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
#include <sys/mman.h> // for MAP_FAILED
#include <sys/un.h>
#include "unix-fd.h"
#include <exception>
#include "poll.h"

NS_LOG_COMPONENT_DEFINE ("LocalSocketFd");

namespace ns3 {
TypeId
LocalSocketFd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LocalSocketFd").SetParent<UnixFd> ();

  return tid;
}
TypeId
LocalSocketFd::GetInstanceTypeId (void) const
{
  return LocalSocketFd::GetTypeId ();
}
LocalSocketFd::LocalSocketFd () : m_readBuffer (0),
                                  m_readBufferSize (0),
                                  m_sendTimeout (0),
                                  m_recvTimeout (0),
                                  m_factory (0),
                                  m_linger (0),
                                  m_bindPath (""),
                                  m_connectPath (""),
                                  m_shutRead (false),
                                  m_shutWrite (false)
{
}
LocalSocketFd::~LocalSocketFd ()
{

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
        NS_LOG_DEBUG ("LocalSocketFd SO_PASSCRED NOT IMPLEMENTED");
        current->err = EINVAL;
        return -1;
      }

    case SO_RCVBUF:
    case SO_SNDBUF:
      {
        NS_LOG_DEBUG ("LocalSocketFd SO_RCVBUF and SO_SNDBUF ignored.");
        return 0;
      }
    case SO_RCVLOWAT:
      {
        NS_LOG_DEBUG ("LocalSocketFd SO_RCVLOWAT ignored.");
        return 0;
      }
    case SO_SNDLOWAT:
      {
        NS_LOG_DEBUG ("LocalSocketFd SO_SNDLOWAT ignored.");
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
LocalSocketFd::Ioctl (unsigned long request, char *argp)
{
  Current ()->err = EINVAL;
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
  GET_CURRENT (offset << whence);
  current->err = ESPIPE;
  return -1;
}
int
LocalSocketFd::Fxstat (int ver, struct ::stat *buf)
{
  GET_CURRENT (ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
LocalSocketFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  GET_CURRENT (ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
LocalSocketFd::Fcntl (int cmd, unsigned long arg)
{
  return UnixFd::Fcntl (cmd, arg);
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

// Return :
//   the size readed ,
//  or 0 if no more space available,
//  or -1 if fatal error occurs
ssize_t
LocalSocketFd::DoRecvPacket (uint8_t* buf, size_t len)
{
  NS_LOG_FUNCTION (this << len << " shutRead:" << m_shutRead << "Closed:" << IsClosed ());

  if ((m_shutRead)|| IsClosed ())
    {
      return -2;
    }
  if (m_readBufferSize >= LOCAL_SOCKET_MAX_BUFFER)
    {
      return 0;
    }

  size_t maxi = LOCAL_SOCKET_MAX_BUFFER - m_readBufferSize;

  int l = std::min (maxi, len);

  struct Buffer *myBuf = new struct Buffer;

  myBuf->data = (uint8_t*) malloc (l);
  if (0 == myBuf->data)
    {
      Thread *current = Current ();
      NS_ASSERT (current != 0);
      current->err = ENOMEM;
      return -1;
    }
  myBuf->readOffset = 0;
  myBuf->size = l;

  memcpy (myBuf->data, buf, l);

  m_readBuffer.push_back (myBuf);

  m_readBufferSize += l;

  NS_LOG_DEBUG ("DoRecvPacket before WakeUpRecv");

  short pi = POLLIN;
  WakeWaiters (&pi); // WakeUp reader or poller for read or select for read

  return l;
}

size_t
LocalSocketFd::ReadData (uint8_t* buf, size_t len, bool peek)
{
  size_t fill = 0;
  size_t rest = len;

  if (peek)
    {
      for (std::list<struct Buffer*>::iterator i = m_readBuffer.begin (); i != m_readBuffer.end (); ++i)
        {
          if ((fill >= len) || (rest <= 0))
            {
              break;
            }

          struct Buffer* myBuf = *i;

          size_t avail = std::min (rest, myBuf->size - myBuf->readOffset);
          NS_LOG_DEBUG ("ReadData avail:" << avail << " offset:" <<  myBuf->readOffset << " size:" << myBuf->size);

          if (avail > 0)
            {
              memcpy (buf + fill, myBuf->data + myBuf->readOffset, avail);
              fill += avail;
              rest -= avail;
            }
        }
    }
  else
    {
      while ((fill < len) && (m_readBuffer.size () > 0) && (rest > 0))
        {
          struct Buffer* myBuf = m_readBuffer.front ();

          size_t avail = std::min (rest, myBuf->size - myBuf->readOffset);
          NS_LOG_DEBUG ("ReadData avail:" << avail << " offset:" <<  myBuf->readOffset << " size:" << myBuf->size);

          if (avail > 0)
            {
              memcpy (buf + fill, myBuf->data + myBuf->readOffset, avail);
              fill += avail;
              rest -= avail;

              m_readBufferSize -= avail;
              myBuf->readOffset += avail;

              if (myBuf->readOffset >= myBuf->size)
                {
                  free (myBuf->data);
                  myBuf->data = 0;
                  m_readBuffer.pop_front ();
                  delete myBuf;
                }
            }
          else
            {
              break;
            }
        }
    }
  return fill;
}

void
LocalSocketFd::ClearReadBuffer (void)
{
  NS_LOG_FUNCTION (this);
  for (std::list<struct Buffer *>::iterator i = m_readBuffer.begin (); i != m_readBuffer.end (); ++i)
    {
      struct Buffer *b = *i;
      if (0 != b->data)
        {
          free (b->data);
        }
      b->data = 0;
      b->size = 0;
      b->readOffset = 0;
      delete b;
    }
  m_readBuffer.clear ();
  m_readBufferSize = 0;
}

int
LocalSocketFd::Ftruncate (off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (this << current << length);

  current->err = EINVAL;
  return -1;
}
int
LocalSocketFd::Fsync (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

} // namespace ns3
