#include "unix-timer-fd.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "task-manager.h"
#include <errno.h>
#include <sys/mman.h>
#include <poll.h>

NS_LOG_COMPONENT_DEFINE ("DceUnixTimerFd");

namespace ns3 {

UnixTimerFd::UnixTimerFd (int clockid, int flags)
  : m_period (Seconds (0.0)),
    m_skipped (0),
    m_timer (),
    m_waiter (0)
{
}

int
UnixTimerFd::Close (void)
{
  m_timer.Cancel ();
  if (m_waiter != 0)
    {
      m_waiter->process->manager->Wakeup (m_waiter);
    }
  return 0;
}

ssize_t
UnixTimerFd::Write (const void *buf, size_t count)
{
  NS_LOG_FUNCTION (this << buf << count);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}
ssize_t
UnixTimerFd::Read (void *buf, size_t count)
{
  Thread *current = Current ();
  if (count < 8)
    {
      current->err = EINVAL;
      return -1;
    }
  uint64_t *counter = (uint64_t *)buf;
  if (m_skipped > 0)
    {
      *counter = m_skipped;
      m_skipped = 0;
      return 0;
    }
  m_waiter = current;
  current->process->manager->Wait ();
  m_waiter = 0;
  *counter = m_skipped;
  m_skipped = 0;

  return 0;
}
ssize_t
UnixTimerFd::Recvmsg (struct msghdr *msg, int flags)
{
  NS_LOG_FUNCTION (this << msg << flags);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
ssize_t
UnixTimerFd::Sendmsg (const struct msghdr *msg, int flags)
{
  NS_LOG_FUNCTION (this << msg << flags);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
bool
UnixTimerFd::Isatty (void) const
{
  return false;
}
int
UnixTimerFd::Setsockopt (int level, int optname,
                         const void *optval, socklen_t optlen)
{
  NS_LOG_FUNCTION (this << level << optname << optval << optlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Getsockopt (int level, int optname,
                         void *optval, socklen_t *optlen)
{
  NS_LOG_FUNCTION (this << level << optname << optval << optlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this << name << namelen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  NS_LOG_FUNCTION (this << name << namelen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Ioctl (unsigned long request, char *argp)
{
  //XXX
  return -1;
}
int
UnixTimerFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  NS_LOG_FUNCTION (this << my_addr << addrlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  NS_LOG_FUNCTION (this << my_addr << addrlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Listen (int backlog)
{
  NS_LOG_FUNCTION (this << backlog);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Shutdown (int how)
{
  NS_LOG_FUNCTION (this << how);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
int
UnixTimerFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  NS_LOG_FUNCTION (this << my_addr << addrlen);
  Thread *current = Current ();
  current->err = ENOTSOCK;
  return -1;
}
void *
UnixTimerFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  NS_LOG_FUNCTION (this << start << length << prot << flags << offset);
  Thread *current = Current ();
  current->err = EINVAL;
  return MAP_FAILED;
}
off64_t
UnixTimerFd::Lseek (off64_t offset, int whence)
{
  NS_LOG_FUNCTION (this << offset << whence);
  Thread *current = Current ();
  current->err = ESPIPE;
  return -1;
}
int
UnixTimerFd::Fxstat (int ver, struct ::stat *buf)
{
  NS_LOG_FUNCTION (this << buf);
  //XXX: I do not know what I should write here.
  // but this call is expected to succeed by the kernel.
  return 0;
}
int
UnixTimerFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  NS_LOG_FUNCTION (this << buf);
  //XXX: I do not know what I should write here.
  // but this call is expected to succeed by the kernel.
  return 0;
}
int
UnixTimerFd::Fcntl (int cmd, unsigned long arg)
{
  // XXX: this really needs to be fixed
  return 0;
}
void
UnixTimerFd::TimerExpired (void)
{
  if (!m_period.IsZero ())
    {
      m_timer = Simulator::Schedule (m_period,
                                     &UnixTimerFd::TimerExpired,
                                     this);
    }
  m_skipped++;
  if (m_waiter != 0)
    {
      m_waiter->process->manager->Wakeup (m_waiter);
    }
}
void
UnixTimerFd::SettimeMain (Time t)
{
  m_timer =  Simulator::Schedule (t,
                                  &UnixTimerFd::TimerExpired,
                                  this);
}
int
UnixTimerFd::Settime (int flags,
                      const struct itimerspec *new_value,
                      struct itimerspec *old_value)
{
  NS_ASSERT (flags == 0);
  if (old_value != 0)
    {
      Gettime (old_value);
    }
  m_skipped = 0;
  m_timer.Cancel ();
  m_period = UtilsTimespecToTime (new_value->it_interval);
  Time initial = UtilsTimespecToTime (new_value->it_value);
  if (!initial.IsZero ())
    {
      TaskManager::Current ()->ExecOnMain (MakeEvent (&UnixTimerFd::SettimeMain,
                                                      this, initial));
    }
  return 0;
}
int
UnixTimerFd::Gettime (struct itimerspec *cur_value) const
{
  if (!m_timer.IsRunning ())
    {
      cur_value->it_value.tv_sec = 0;
      cur_value->it_value.tv_nsec = 0;
    }
  else
    {
      Time left = Simulator::GetDelayLeft (m_timer);
      cur_value->it_value = UtilsTimeToTimespec (left);
    }
  cur_value->it_interval = UtilsTimeToTimespec (m_period);
  return 0;
}

bool
UnixTimerFd::CanRecv (void) const
{
  // XXX ?
  return false;
}
bool
UnixTimerFd::CanSend (void) const
{
  return false;
}
bool
UnixTimerFd::HangupReceived (void) const
{
  return false;
}
int
UnixTimerFd::Poll (PollTable* ptable)
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

int
UnixTimerFd::Ftruncate (off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (this << current << length);

  current->err = EINVAL;
  return -1;
}

int
UnixTimerFd::Fsync (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}

} // namespace ns3
