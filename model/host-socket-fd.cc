/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "host-socket-fd.h"
#include "task-manager.h"
#include "process.h"
#include "utils.h"
#include "ns3/log.h"
#include "ns3/global-value.h"
#include "ns3/string.h"
#include "ns3/simulator.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/un.h>

NS_LOG_COMPONENT_DEFINE ("DceHostSocketFd");

namespace ns3 {

static GlobalValue g_hostUnixSocketPaths = GlobalValue (
    "DceHostUnixSocketPaths",
    "Colon separated list of AF_UNIX socket path prefixes. A DCE application "
    "connecting a stream socket to a matching path reaches the host service "
    "listening there (for example /tmp/.X11-unix/ for the X server) instead "
    "of a socket of its simulated node. Requires ns3::RealtimeSimulatorImpl.",
    StringValue (""), MakeStringChecker ());

bool
UtilsIsHostUnixSocketPath (const std::string &path)
{
  StringValue value;
  g_hostUnixSocketPaths.GetValue (value);
  std::string prefixes = value.Get ();
  size_t start = 0;
  while (start < prefixes.size ())
    {
      size_t end = prefixes.find (':', start);
      if (end == std::string::npos)
        {
          end = prefixes.size ();
        }
      std::string prefix = prefixes.substr (start, end - start);
      if (prefix != "" && path.compare (0, prefix.size (), prefix) == 0)
        {
          return true;
        }
      start = end + 1;
    }
  return false;
}

HostSocketFd *
HostSocketFd::ConnectHost (const struct sockaddr *addr, socklen_t addrlen)
{
  int fd = ::socket (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
  if (fd == -1)
    {
      return 0;
    }
  if (::connect (fd, addr, addrlen) == -1)
    {
      // connect() on a non-blocking AF_UNIX socket completes immediately
      // when the server is listening, so anything else is a real failure.
      int err = errno;
      ::close (fd);
      errno = err;
      return 0;
    }
  NS_LOG_INFO ("connected host socket fd=" << fd);
  return new HostSocketFd (fd);
}

HostSocketFd::HostSocketFd (int realFd)
  : UnixFileFd (realFd),
    m_closed (false)
{
  // The application sees a blocking socket until it asks otherwise; the
  // host socket itself always stays non-blocking.
  m_statusFlags = O_RDWR;
}

HostSocketFd::~HostSocketFd ()
{
  m_check.Cancel ();
}

int
HostSocketFd::SetErrno (int result)
{
  if (result == -1 && Current () != 0)
    {
      Current ()->err = errno;
    }
  return result;
}

short
HostSocketFd::HostEvents (void) const
{
  if (m_closed)
    {
      return POLLHUP;
    }
  struct pollfd p;
  p.fd = PeekRealFd ();
  p.events = POLLIN | POLLOUT;
  p.revents = 0;
  if (::poll (&p, 1, 0) <= 0)
    {
      return 0;
    }
  return p.revents;
}

bool
HostSocketFd::WaitFor (short events)
{
  events |= POLLHUP | POLLERR;
  while (!(HostEvents () & events))
    {
      // No way to be notified by the host without a thread: re-check every
      // millisecond of (real-time) simulation.
      TaskManager::Current ()->Sleep (MilliSeconds (1));
      if (m_closed)
        {
          return false;
        }
      Thread *current = Current ();
      if (current != 0
          && (!sigisemptyset (&current->pendingSignals)
              || !sigisemptyset (&current->process->pendingSignals)))
        {
          return false;
        }
    }
  return true;
}

void
HostSocketFd::CheckReadiness (void)
{
  if (m_closed)
    {
      return;
    }
  short events = HostEvents ();
  if (events != 0)
    {
      // Wakes up the pollers whose event mask matches (a socket is almost
      // always writable, so this is a no-op for pollers waiting for input).
      WakeWaiters (&events);
    }
  // The pollers cannot be observed from here, so keep checking until the
  // socket is closed: one event per millisecond.
  m_check = Simulator::Schedule (MilliSeconds (1), &HostSocketFd::CheckReadiness, this);
}

int
HostSocketFd::Poll (PollTable *ptable)
{
  short events = HostEvents ();
  if (ptable != 0)
    {
      ptable->PollWait (this);
      if (!m_check.IsPending ())
        {
          m_check = Simulator::Schedule (MilliSeconds (1), &HostSocketFd::CheckReadiness, this);
        }
    }
  return events;
}

bool
HostSocketFd::CanRecv (void) const
{
  return HostEvents () & (POLLIN | POLLHUP | POLLERR);
}

bool
HostSocketFd::CanSend (void) const
{
  return HostEvents () & (POLLOUT | POLLHUP | POLLERR);
}

bool
HostSocketFd::HangupReceived (void) const
{
  return HostEvents () & POLLHUP;
}

int
HostSocketFd::Close (void)
{
  m_closed = true;
  m_check.Cancel ();
  return UnixFileFd::Close ();
}

ssize_t
HostSocketFd::Read (void *buf, size_t count)
{
  struct iovec iov;
  iov.iov_base = buf;
  iov.iov_len = count;
  struct msghdr msg;
  memset (&msg, 0, sizeof (msg));
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  return Recvmsg (&msg, 0);
}

ssize_t
HostSocketFd::Write (const void *buf, size_t count)
{
  struct iovec iov;
  iov.iov_base = (void *)buf;
  iov.iov_len = count;
  struct msghdr msg;
  memset (&msg, 0, sizeof (msg));
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  return Sendmsg (&msg, 0);
}

ssize_t
HostSocketFd::Recvmsg (struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) || (flags & MSG_DONTWAIT);
  while (true)
    {
      ssize_t r = ::recvmsg (PeekRealFd (), msg, flags | MSG_DONTWAIT);
      if (r >= 0)
        {
          NS_LOG_LOGIC ("recvmsg fd=" << PeekRealFd () << " -> " << r);
          return r;
        }
      if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
          NS_LOG_INFO ("recvmsg fd=" << PeekRealFd () << " failed: " << strerror (errno));
          current->err = errno;
          return -1;
        }
      if (nonBlocking)
        {
          current->err = EAGAIN;
          return -1;
        }
      if (!WaitFor (POLLIN))
        {
          NS_LOG_INFO ("recvmsg fd=" << PeekRealFd () << " interrupted");
          current->err = EINTR;
          return -1;
        }
    }
}

ssize_t
HostSocketFd::Sendmsg (const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) || (flags & MSG_DONTWAIT);
  while (true)
    {
      ssize_t r = ::sendmsg (PeekRealFd (), msg, flags | MSG_DONTWAIT | MSG_NOSIGNAL);
      if (r >= 0)
        {
          NS_LOG_LOGIC ("sendmsg fd=" << PeekRealFd () << " -> " << r);
          return r;
        }
      if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
          NS_LOG_INFO ("sendmsg fd=" << PeekRealFd () << " failed: " << strerror (errno));
          current->err = errno;
          return -1;
        }
      if (nonBlocking)
        {
          current->err = EAGAIN;
          return -1;
        }
      if (!WaitFor (POLLOUT))
        {
          current->err = EINTR;
          return -1;
        }
    }
}

int
HostSocketFd::Setsockopt (int level, int optname, const void *optval, socklen_t optlen)
{
  return SetErrno (::setsockopt (PeekRealFd (), level, optname, optval, optlen));
}

int
HostSocketFd::Getsockopt (int level, int optname, void *optval, socklen_t *optlen)
{
  return SetErrno (::getsockopt (PeekRealFd (), level, optname, optval, optlen));
}

int
HostSocketFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  return SetErrno (::getsockname (PeekRealFd (), name, namelen));
}

int
HostSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  return SetErrno (::getpeername (PeekRealFd (), name, namelen));
}

int
HostSocketFd::Shutdown (int how)
{
  return SetErrno (::shutdown (PeekRealFd (), how));
}

int
HostSocketFd::Ioctl (unsigned long request, char *argp)
{
  return SetErrno (::ioctl (PeekRealFd (), request, argp));
}

int
HostSocketFd::Fcntl (int cmd, unsigned long arg)
{
  // O_NONBLOCK is emulated: the host socket is always non-blocking.
  switch (cmd)
    {
    case F_GETFL:
      return m_statusFlags;
    case F_SETFL:
      m_statusFlags = arg;
      return 0;
    case F_GETFD:
      return m_fdFlags;
    case F_SETFD:
      m_fdFlags = arg;
      return 0;
    default:
      return SetErrno (::fcntl (PeekRealFd (), cmd, arg));
    }
}

} // namespace ns3
