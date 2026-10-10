/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "lkl-socket-fd.h"
#include "lkl-socket-fd-factory.h"
#include "lkl-kernel.h"
#include "task-manager.h"
#include "process.h"
#include "utils.h"
#include "file-usage.h"
#include "wait-queue.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <lkl/asm/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <cstring>
#include <vector>

NS_LOG_COMPONENT_DEFINE ("DceLklSocketFd");

namespace ns3 {

LklSocketFd::LklSocketFd (Ptr<LklSocketFdFactory> factory, int fd)
  : m_factory (factory),
    m_fd (fd)
{
  NS_LOG_FUNCTION (this << fd);
  m_factory->Register (this);
}

LklSocketFd::~LklSocketFd ()
{
  // Not closed: the files of a process DCE stops are released, not closed.
  if (m_fd >= 0)
    {
      m_factory->Unregister (this);
      // Not when the simulation is destroyed, outside of the node.
      Ptr<LklKernel> kernel = m_factory->GetKernel ();
      if (kernel && kernel->GetTaskManager ()
          && kernel->GetTaskManager () == TaskManager::Current ())
        {
          RawCall (__lkl__NR_close, m_fd);
        }
    }
}

int
LklSocketFd::GetKernelFd (void) const
{
  return m_fd;
}

void
LklSocketFd::NotifyEvents (short events)
{
  WakeWaiters (&events);
}

long
LklSocketFd::RawCall (long no, long a0, long a1, long a2, long a3, long a4, long a5) const
{
  long params[6] = { a0, a1, a2, a3, a4, a5 };
  return m_factory->GetKernel ()->Syscall (no, params);
}

long
LklSocketFd::Result (long ret) const
{
  if (ret < 0)
    {
      Current ()->err = -ret;
      return -1;
    }
  return ret;
}

long
LklSocketFd::Call (long no, long a0, long a1, long a2, long a3, long a4, long a5) const
{
  return Result (RawCall (no, a0, a1, a2, a3, a4, a5));
}

Time
LklSocketFd::GetTimeout (int option) const
{
  struct timeval tv;
  socklen_t len = sizeof (tv);
  if (RawCall (__lkl__NR_getsockopt, m_fd, SOL_SOCKET, option, (long)&tv, (long)&len) < 0)
    {
      return Seconds (0);
    }
  return Seconds (tv.tv_sec) + MicroSeconds (tv.tv_usec);
}

int
LklSocketFd::WaitEvents (short events, Time timeout)
{
  // Watched first: the probe below then sees the events that came before,
  // and the watcher wakes this task for the later ones.
  m_factory->Watch (this);
  struct pollfd pfd;
  pfd.fd = m_fd;
  pfd.events = events;
  pfd.revents = 0;
  struct
  {
    long long tv_sec;
    long long tv_nsec;
  } zero = { 0, 0 };
  // Ready although the call would block (e.g. DCCP reports a socket with a
  // full queue writable): wait for the next event, or retry in 1 ms.
  bool ready = RawCall (__lkl__NR_ppoll, (long)&pfd, 1, (long)&zero, 0, 8) > 0;
  bool retry = ready && (timeout.IsZero () || timeout > MilliSeconds (1));
  WaitQueueEntryTimeout *wq = new WaitQueueEntryTimeout (events | POLLERR | POLLHUP,
                                                         retry ? MilliSeconds (1) : timeout);
  AddWaitQueue (wq, true);
  PollTable::Result res = wq->Wait ();
  RemoveWaitQueue (wq, true);
  delete wq;
  return retry && res == PollTable::TIMEOUT ? PollTable::OK : res;
}

// DCE deletes the tasks of the processes it stops, at any time, while the
// kernel keeps references to a task sleeping in it (e.g. the wait queue
// entries on its stack): so tasks never block in the kernel, but in DCE.
long
LklSocketFd::BlockingCall (short events, int timeoutOption,
                           long no, long a0, long a1, long a2, long a3, long a4, long a5)
{
  long flags = RawCall (__lkl__NR_fcntl, m_fd, F_GETFL);
  if (flags < 0 || (flags & O_NONBLOCK))
    {
      return RawCall (no, a0, a1, a2, a3, a4, a5);
    }
  Time end = Simulator::Now () + GetTimeout (timeoutOption);
  bool timeout = end != Simulator::Now ();
  while (true)
    {
      RawCall (__lkl__NR_fcntl, m_fd, F_SETFL, flags | O_NONBLOCK);
      long ret = RawCall (no, a0, a1, a2, a3, a4, a5);
      RawCall (__lkl__NR_fcntl, m_fd, F_SETFL, flags);
      if (ret != -EAGAIN)
        {
          return ret;
        }
      if (timeout && Simulator::Now () >= end)
        {
          return -EAGAIN;
        }
      switch (WaitEvents (events, timeout ? end - Simulator::Now () : Seconds (0)))
        {
        case PollTable::INTERRUPTED:
          return -EINTR;
        case PollTable::TIMEOUT:
          return -EAGAIN;
        default:
          break;
        }
    }
}

int
LklSocketFd::Close (void)
{
  NS_LOG_FUNCTION (this);
  m_factory->Unregister (this);
  int fd = m_fd;
  m_fd = -1;
  return Call (__lkl__NR_close, fd);
}

ssize_t
LklSocketFd::Write (const void *buf, size_t count)
{
  // A blocking write returns once all the data is queued.
  size_t sent = 0;
  while (true)
    {
      long ret = BlockingCall (POLLOUT, SO_SNDTIMEO, __lkl__NR_write, m_fd,
                               (long)((const uint8_t *)buf + sent), count - sent);
      if (ret < 0)
        {
          return sent > 0 ? (ssize_t)sent : Result (ret);
        }
      sent += ret;
      if (sent >= count || ret == 0)
        {
          return sent;
        }
    }
}

ssize_t
LklSocketFd::Read (void *buf, size_t count)
{
  return Result (BlockingCall (POLLIN, SO_RCVTIMEO, __lkl__NR_read, m_fd, (long)buf, count));
}

ssize_t
LklSocketFd::Recvmsg (struct msghdr *msg, int flags)
{
  if (flags & MSG_DONTWAIT)
    {
      return Call (__lkl__NR_recvmsg, m_fd, (long)msg, flags);
    }
  return Result (BlockingCall (POLLIN, SO_RCVTIMEO, __lkl__NR_recvmsg, m_fd, (long)msg, flags));
}

ssize_t
LklSocketFd::Sendmsg (const struct msghdr *msg, int flags)
{
  if (flags & MSG_DONTWAIT)
    {
      return Call (__lkl__NR_sendmsg, m_fd, (long)msg, flags);
    }
  // A blocking send returns once all the data is queued: send the rest of
  // a partial send.
  std::vector<struct iovec> iov (msg->msg_iov, msg->msg_iov + msg->msg_iovlen);
  struct msghdr rest = *msg;
  rest.msg_iov = iov.empty () ? 0 : &iov[0];
  size_t total = 0;
  for (size_t i = 0; i < iov.size (); i++)
    {
      total += iov[i].iov_len;
    }
  size_t sent = 0;
  while (true)
    {
      long ret = BlockingCall (POLLOUT, SO_SNDTIMEO, __lkl__NR_sendmsg, m_fd, (long)&rest, flags);
      if (ret < 0)
        {
          return sent > 0 ? (ssize_t)sent : Result (ret);
        }
      sent += ret;
      if (sent >= total || ret == 0)
        {
          return sent;
        }
      // Skip what was sent.
      size_t skip = ret;
      while (rest.msg_iovlen > 0 && skip >= rest.msg_iov->iov_len)
        {
          skip -= rest.msg_iov->iov_len;
          rest.msg_iov++;
          rest.msg_iovlen--;
        }
      if (rest.msg_iovlen > 0)
        {
          rest.msg_iov->iov_base = (uint8_t *)rest.msg_iov->iov_base + skip;
          rest.msg_iov->iov_len -= skip;
        }
    }
}

bool
LklSocketFd::Isatty (void) const
{
  return false;
}

int
LklSocketFd::Setsockopt (int level, int optname, const void *optval, socklen_t optlen)
{
  return Call (__lkl__NR_setsockopt, m_fd, level, optname, (long)optval, optlen);
}

int
LklSocketFd::Getsockopt (int level, int optname, void *optval, socklen_t *optlen)
{
  return Call (__lkl__NR_getsockopt, m_fd, level, optname, (long)optval, (long)optlen);
}

int
LklSocketFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  return Call (__lkl__NR_getsockname, m_fd, (long)name, (long)namelen);
}

int
LklSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  return Call (__lkl__NR_getpeername, m_fd, (long)name, (long)namelen);
}

int
LklSocketFd::Ioctl (unsigned long request, char *argp)
{
  return Call (__lkl__NR_ioctl, m_fd, request, (long)argp);
}

int
LklSocketFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  return Call (__lkl__NR_bind, m_fd, (long)my_addr, addrlen);
}

int
LklSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  long flags = RawCall (__lkl__NR_fcntl, m_fd, F_GETFL);
  if (flags < 0 || (flags & O_NONBLOCK))
    {
      return Call (__lkl__NR_connect, m_fd, (long)my_addr, addrlen);
    }
  // Without blocking in the kernel (see BlockingCall): connect, and wait in
  // DCE until connecting again tells the outcome (and, on failure, makes the
  // socket unconnected again, like a blocking connect).
  Time end = Simulator::Now () + GetTimeout (SO_SNDTIMEO);
  bool timeout = end != Simulator::Now ();
  bool started = false;
  while (true)
    {
      RawCall (__lkl__NR_fcntl, m_fd, F_SETFL, flags | O_NONBLOCK);
      long ret = RawCall (__lkl__NR_connect, m_fd, (long)my_addr, addrlen);
      RawCall (__lkl__NR_fcntl, m_fd, F_SETFL, flags);
      if (started && (ret == -EISCONN || ret == -EEXIST))
        {
          // Connected, for protocols that do not report it by connecting
          // again (SCTP).
          int error = 0;
          socklen_t len = sizeof (error);
          ret = RawCall (__lkl__NR_getsockopt, m_fd, SOL_SOCKET, SO_ERROR, (long)&error, (long)&len);
          return Result (ret < 0 ? ret : -error);
        }
      if (ret != -EINPROGRESS && ret != -EALREADY)
        {
          return Result (ret);
        }
      started = true;
      if (timeout && Simulator::Now () >= end)
        {
          return Result (-EINPROGRESS);
        }
      switch (WaitEvents (POLLOUT, timeout ? end - Simulator::Now () : Seconds (0)))
        {
        case PollTable::INTERRUPTED:
          return Result (-EINTR);
        case PollTable::TIMEOUT:
          return Result (-EINPROGRESS);
        default:
          break;
        }
    }
}

int
LklSocketFd::Listen (int backlog)
{
  return Call (__lkl__NR_listen, m_fd, backlog);
}

int
LklSocketFd::Shutdown (int how)
{
  return Call (__lkl__NR_shutdown, m_fd, how);
}

int
LklSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  NS_LOG_FUNCTION (this);
  long kernelFd = Result (BlockingCall (POLLIN, SO_RCVTIMEO, __lkl__NR_accept4,
                                       m_fd, (long)my_addr, (long)addrlen, 0));
  if (kernelFd < 0)
    {
      return -1;
    }
  Thread *current = Current ();
  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      Call (__lkl__NR_close, kernelFd);
      current->err = EMFILE;
      return -1;
    }
  UnixFd *unixFd = new LklSocketFd (m_factory, kernelFd);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);
  return fd;
}

void *
LklSocketFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  Current ()->err = ENODEV;
  return MAP_FAILED;
}

off64_t
LklSocketFd::Lseek (off64_t offset, int whence)
{
  Current ()->err = ESPIPE;
  return -1;
}

int
LklSocketFd::Fxstat (int ver, struct ::stat *buf)
{
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}

int
LklSocketFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}

int
LklSocketFd::Fcntl (int cmd, unsigned long arg)
{
  switch (cmd)
    {
    case F_GETFL:
    case F_SETFL:
      // The kernel keeps O_NONBLOCK and applies it to every operation.
      return Call (__lkl__NR_fcntl, m_fd, cmd, arg);
    case F_GETFD:
      return m_fdFlags;
    case F_SETFD:
      m_fdFlags = arg;
      return 0;
    default:
      return UnixFd::Fcntl (cmd, arg);
    }
}

int
LklSocketFd::Settime (int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{
  Current ()->err = EINVAL;
  return -1;
}

int
LklSocketFd::Gettime (struct itimerspec *cur_value) const
{
  Current ()->err = EINVAL;
  return -1;
}

int
LklSocketFd::Ftruncate (off_t length)
{
  Current ()->err = EINVAL;
  return -1;
}

bool
LklSocketFd::HangupReceived (void) const
{
  return false;
}

int
LklSocketFd::Poll (PollTable* ptable)
{
  short wanted = ptable ? ptable->GetEventMask () : (POLLIN | POLLOUT | POLLPRI | POLLRDHUP);
  struct pollfd pfd;
  pfd.fd = m_fd;
  pfd.events = wanted;
  pfd.revents = 0;
  struct
  {
    long long tv_sec;
    long long tv_nsec;
  } zero = { 0, 0 };
  long params[6] = { (long)&pfd, 1, (long)&zero, 0, 8, 0 };
  long ret = m_factory->GetKernel ()->Syscall (__lkl__NR_ppoll, params);
  if (ret < 0)
    {
      return POLLERR;
    }
  if (ptable)
    {
      // Get woken when the kernel reports one of these events.
      ptable->PollWait (this);
      m_factory->Watch (this);
    }
  return pfd.revents;
}

int
LklSocketFd::Fsync (void)
{
  Current ()->err = EINVAL;
  return -1;
}

} // namespace ns3
