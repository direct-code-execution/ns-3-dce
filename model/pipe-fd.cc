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
#include "pipe-fd.h"
#include "ns3/log.h"
#include "utils.h"
#include "process.h"
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

NS_LOG_COMPONENT_DEFINE ("PipeFd");

#define PIPE_CAPACITY 65536

namespace ns3 {
PipeFd::PipeFd () : m_peer (0),
                    m_readSide (1),
                    m_statusFlags (0),
                    m_buf (PIPE_CAPACITY)
{
}

PipeFd::PipeFd (PipeFd *peer) : m_peer (peer),
                                m_readSide (0),
                                m_statusFlags (0),
                                m_buf (0)
{
  m_peer->m_peer = this;
}

int
PipeFd::Close (void)
{
  NS_LOG_FUNCTION (this);
  if (m_peer)
    {
      PipeFd *p = m_peer;
      m_peer = 0;
      p->Close ();
      short ph = POLLHUP;
      WakeWaiters (&ph);
      return 0;
    }
  Current ()->err = EBADF;
  return -1;
}

ssize_t
PipeFd::Write (const void *buf, size_t count)
{
  NS_LOG_FUNCTION (this);
  if (m_peer)
    {
      if (m_readSide)
        {
          Current ()->err = EBADF;
          return -1;
        }
      size_t r = m_peer->DoRecvPacket ((uint8_t*)buf, count);
      if (r > 0)
        {
          short po = POLLOUT;
          WakeWaiters (&po); // WakeUp
        }
      return r;
    }
  UtilsSendSignal (Current ()->process, SIGPIPE);
  UtilsDoSignal ();
  Current ()->err = EPIPE;
  return -1;
}
ssize_t
PipeFd::DoRecvPacket (uint8_t* buf, size_t len)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << (long)buf << len);
  NS_ASSERT (current != 0);

  WaitQueueEntryTimeout *wq = 0;

  while (true)
    {
      ssize_t r = m_buf.Write (buf, len);
      if (r > 0)
        {
          short pi = POLLIN;
          WakeWaiters (&pi); // WakeUp reader or poller for read or select for read

          RETURNFREE (r);
        }
      if (m_peer->m_statusFlags & O_NONBLOCK)
        {
          Current ()->err = EAGAIN;
          RETURNFREE (-1);
        }

      if (!wq)
        {
          wq = new WaitQueueEntryTimeout (POLLOUT | POLLHUP, Time (0));
        }
      AddWaitQueue (wq, true);
      PollTable::Result res = wq->Wait ();
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
          break;
        case PollTable::TIMEOUT:
          {
            current->err = EAGAIN;
            RETURNFREE (-1);
          }
          break;
        }
    }
}

ssize_t
PipeFd::Read (void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << (long)buf << count);
  NS_ASSERT (current != 0);

  if (!m_readSide)
    {
      Current ()->err = EBADF;
      return -1;
    }
  WaitQueueEntryTimeout *wq = 0;

  while (true)
    {
      ssize_t r = m_buf.Read ((uint8_t*)buf, count);
      if (r > 0)
        {
          short po = POLLOUT;
          if (m_peer)
            {
              // WakeUp
              m_peer->WakeWaiters (&po);
            }
          WakeWaiters (&po);

          RETURNFREE (r);
        }
      if (0 == m_peer)
        {
          Current ()->err = EPIPE;
          RETURNFREE (-1);
        }
      if (m_statusFlags & O_NONBLOCK)
        {
          Current ()->err = EAGAIN;
          RETURNFREE (-1);
        }
      if (!wq)
        {
          wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, Time (0));
        }
      AddWaitQueue (wq, true);
      PollTable::Result res = wq->Wait ();
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
          break;
        case PollTable::TIMEOUT:
          {
            current->err = EAGAIN;
            RETURNFREE (-1);
          }
          break;
        }
    }
}

ssize_t
PipeFd::Recvmsg (struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

ssize_t
PipeFd::Sendmsg (const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Setsockopt (int level, int optname,
                    const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Getsockopt (int level, int optname,
                    void *optval, socklen_t *optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Ioctl (unsigned long request, char *argp)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EINVAL;
  return -1;
}

int
PipeFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Listen (int backlog)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Shutdown (int how)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
PipeFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

void *
PipeFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return 0;
}

off64_t
PipeFd::Lseek (off64_t offset, int whence)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

int
PipeFd::Fxstat (int ver, struct ::stat *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

int
PipeFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

int
PipeFd::Fcntl (int cmd, unsigned long arg)
{
  switch (cmd)
    {
    case F_GETFL:
      return m_statusFlags;
      break;
    case F_SETFL:
      m_statusFlags = arg;
      return 0;
      break;
    default:
      NS_FATAL_ERROR ("fcntl not implemented on pipe");
      return -1;
    }
}

int
PipeFd::Settime (int flags,
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
PipeFd::Gettime (struct itimerspec *cur_value) const
{
  NS_LOG_FUNCTION (this << Current ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}

int
PipeFd::Ftruncate (off_t length)
{
  NS_LOG_FUNCTION (this << Current ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}

// Return true if a select on this fd should return POLLHUP
bool
PipeFd::HangupReceived (void) const
{
  return (0 == m_peer);
}
bool
PipeFd::Isatty (void) const
{
  return 0;
}

int
PipeFd::Poll (PollTable* ptable)
{
  int ret = 0;

  if (m_readSide)
    {
      if (m_buf.GetSize () > 0)
        {
          ret |= POLLIN;
        }
    }
  else
    {
      if (m_peer && (m_peer->m_buf.GetSpace () > 0))
        {
          ret |= POLLOUT;
        }
    }
  if (0 == m_peer)
    {
      ret |= POLLHUP;
    }
  if (ptable)
    {
      ptable->PollWait (this);
    }
#ifdef TODO
  if (m_readSide)
    {
      if (m_buffer.GetSize () < PIPE_CAPACITY)
        {
          ret |= POLLIN;
        }
    }
  else
    {
      if (m_peer &&  (m_peer->m_buffer.GetSize () < PIPE_CAPACITY))
        {
          ret |= POLLOUT;
        }
    }

  if (HangupReceived ())
    {
      ret |= POLLHUP;
    }

  if (ptable)
    {
      ptable->PollWait (this);
    }
#endif
  return ret;
}

int
PipeFd::Fsync (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

} // namespace ns3
