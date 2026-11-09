/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "event-fd.h"
#include "process.h"
#include "utils.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/mman.h>

NS_LOG_COMPONENT_DEFINE ("DceEventFd");

namespace ns3 {

// The counter may not reach the maximum value (see eventfd(2)).
static const uint64_t EVENTFD_MAX = 0xfffffffffffffffeULL;

EventFd::EventFd (unsigned int initval, int flags)
  : m_counter (initval),
    m_semaphore (flags & EFD_SEMAPHORE)
{
  m_statusFlags = O_RDWR | ((flags & EFD_NONBLOCK) ? O_NONBLOCK : 0);
  m_fdFlags = (flags & EFD_CLOEXEC) ? FD_CLOEXEC : 0;
}

EventFd::~EventFd ()
{
}

short
EventFd::Events (void) const
{
  short events = 0;
  if (m_counter > 0)
    {
      events |= POLLIN;
    }
  if (m_counter < EVENTFD_MAX)
    {
      events |= POLLOUT;
    }
  return events;
}

int
EventFd::Close (void)
{
  return 0;
}

ssize_t
EventFd::Read (void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << count);
  NS_ASSERT (current != 0);
  if (count < sizeof (uint64_t))
    {
      current->err = EINVAL;
      return -1;
    }
  WaitQueueEntryTimeout *wq = 0;
  while (true)
    {
      if (m_counter > 0)
        {
          uint64_t value = m_semaphore ? 1 : m_counter;
          m_counter -= value;
          memcpy (buf, &value, sizeof (value));
          short po = POLLOUT;
          WakeWaiters (&po);
          RETURNFREE (sizeof (value));
        }
      if (m_statusFlags & O_NONBLOCK)
        {
          current->err = EAGAIN;
          RETURNFREE (-1);
        }
      if (!wq)
        {
          wq = new WaitQueueEntryTimeout (POLLIN | POLLHUP, Time (0));
        }
      AddWaitQueue (wq, true);
      PollTable::Result res = wq->Wait ();
      RemoveWaitQueue (wq, true);
      if (res == PollTable::INTERRUPTED)
        {
          UtilsDoSignal ();
          current->err = EINTR;
          RETURNFREE (-1);
        }
    }
}

ssize_t
EventFd::Write (const void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << count);
  NS_ASSERT (current != 0);
  uint64_t value;
  if (count < sizeof (value))
    {
      current->err = EINVAL;
      return -1;
    }
  memcpy (&value, buf, sizeof (value));
  if (value == 0xffffffffffffffffULL)
    {
      current->err = EINVAL;
      return -1;
    }
  WaitQueueEntryTimeout *wq = 0;
  while (true)
    {
      if (value <= EVENTFD_MAX - m_counter)
        {
          m_counter += value;
          short pi = POLLIN;
          WakeWaiters (&pi);
          RETURNFREE (sizeof (value));
        }
      if (m_statusFlags & O_NONBLOCK)
        {
          current->err = EAGAIN;
          RETURNFREE (-1);
        }
      if (!wq)
        {
          wq = new WaitQueueEntryTimeout (POLLOUT | POLLHUP, Time (0));
        }
      AddWaitQueue (wq, true);
      PollTable::Result res = wq->Wait ();
      RemoveWaitQueue (wq, true);
      if (res == PollTable::INTERRUPTED)
        {
          UtilsDoSignal ();
          current->err = EINTR;
          RETURNFREE (-1);
        }
    }
}

int
EventFd::Poll (PollTable* ptable)
{
  if (ptable)
    {
      ptable->PollWait (this);
    }
  return Events ();
}

bool
EventFd::HangupReceived (void) const
{
  return false;
}

#define EVENTFD_NOT_SUPPORTED(rettype, err, name, args)   \
  rettype EventFd::name args                                \
  {                                                         \
    Current ()->err = err;                                  \
    return (rettype) -1;                                    \
  }

ssize_t
EventFd::Recvmsg (struct msghdr *msg, int flags)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
ssize_t
EventFd::Sendmsg (const struct msghdr *msg, int flags)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
bool
EventFd::Isatty (void) const
{
  return false;
}
int
EventFd::Setsockopt (int level, int optname, const void *optval, socklen_t optlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Getsockopt (int level, int optname, void *optval, socklen_t *optlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Ioctl (unsigned long request, char *argp)
{
  Current ()->err = EINVAL;
  return -1;
}
int
EventFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Listen (int backlog)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Shutdown (int how)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EventFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
void *
EventFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  Current ()->err = ENODEV;
  return MAP_FAILED;
}
off64_t
EventFd::Lseek (off64_t offset, int whence)
{
  Current ()->err = ESPIPE;
  return -1;
}
int
EventFd::Fxstat (int ver, struct ::stat *buf)
{
  memset (buf, 0, sizeof (*buf));
  buf->st_mode = S_IFIFO | 0600;
  return 0;
}
int
EventFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  memset (buf, 0, sizeof (*buf));
  buf->st_mode = S_IFIFO | 0600;
  return 0;
}
int
EventFd::Settime (int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{
  Current ()->err = EINVAL;
  return -1;
}
int
EventFd::Gettime (struct itimerspec *cur_value) const
{
  Current ()->err = EINVAL;
  return -1;
}
int
EventFd::Ftruncate (off_t length)
{
  Current ()->err = EINVAL;
  return -1;
}
int
EventFd::Fsync (void)
{
  Current ()->err = EINVAL;
  return -1;
}

} // namespace ns3
