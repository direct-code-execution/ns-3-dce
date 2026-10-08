/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "lkl-socket-fd.h"
#include "lkl-socket-fd-factory.h"
#include "lkl-kernel.h"
#include "process.h"
#include "utils.h"
#include "file-usage.h"
#include "wait-queue.h"
#include "ns3/log.h"
#include <lkl/asm/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>

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
LklSocketFd::Call (long no, long a0, long a1, long a2, long a3, long a4, long a5) const
{
  long params[6] = { a0, a1, a2, a3, a4, a5 };
  long ret = m_factory->GetKernel ()->Syscall (no, params);
  if (ret < 0)
    {
      Current ()->err = -ret;
      return -1;
    }
  return ret;
}

int
LklSocketFd::Close (void)
{
  NS_LOG_FUNCTION (this);
  m_factory->Unregister (this);
  return Call (__lkl__NR_close, m_fd);
}

ssize_t
LklSocketFd::Write (const void *buf, size_t count)
{
  return Call (__lkl__NR_write, m_fd, (long)buf, count);
}

ssize_t
LklSocketFd::Read (void *buf, size_t count)
{
  return Call (__lkl__NR_read, m_fd, (long)buf, count);
}

ssize_t
LklSocketFd::Recvmsg (struct msghdr *msg, int flags)
{
  return Call (__lkl__NR_recvmsg, m_fd, (long)msg, flags);
}

ssize_t
LklSocketFd::Sendmsg (const struct msghdr *msg, int flags)
{
  return Call (__lkl__NR_sendmsg, m_fd, (long)msg, flags);
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
  return Call (__lkl__NR_connect, m_fd, (long)my_addr, addrlen);
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
  long kernelFd = Call (__lkl__NR_accept4, m_fd, (long)my_addr, (long)addrlen, 0);
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
