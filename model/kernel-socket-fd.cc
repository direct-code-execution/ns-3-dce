#include "kernel-socket-fd.h"
#include "kernel-socket-fd-factory.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h> // for MMAP_FAILED
#include <poll.h>

NS_LOG_COMPONENT_DEFINE ("DceKernelSocketFd");

namespace ns3 {

KernelSocketFd::KernelSocketFd (Ptr<KernelSocketFdFactory> factory, struct DceSocket *socket)
  : m_factory (factory),
    m_socket (socket),
    m_statusFlags (0)
{
}

KernelSocketFd::~KernelSocketFd ()
{
}

int
KernelSocketFd::Close (void)
{
  return m_factory->Close (m_socket);
}
ssize_t
KernelSocketFd::Write (const void *buf, size_t count)
{
  NS_LOG_FUNCTION (this << buf << count);
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = count;
  iov.iov_base = (void*)buf;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  ssize_t retval = Sendmsg (&msg, 0);
  return retval;
}
ssize_t
KernelSocketFd::Read (void *buf, size_t count)
{
  NS_LOG_FUNCTION (this << buf << count);
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = count;
  iov.iov_base = buf;
  msg.msg_name = 0;
  msg.msg_namelen = 0;
  ssize_t retval = Recvmsg (&msg, 0);
  return retval;
}
ssize_t
KernelSocketFd::Recvmsg (struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking ? MSG_DONTWAIT : 0;
  return m_factory->Recvmsg (m_socket, msg, flags);
}
ssize_t
KernelSocketFd::Sendmsg (const struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking ? MSG_DONTWAIT : 0;
  return m_factory->Sendmsg (m_socket, msg, flags);
}
bool
KernelSocketFd::Isatty (void) const
{
  return false;
}
int
KernelSocketFd::Setsockopt (int level, int optname,
                           const void *optval, socklen_t optlen)
{
  return m_factory->Setsockopt (m_socket, level, optname, optval, optlen);
}
int
KernelSocketFd::Getsockopt (int level, int optname,
                           void *optval, socklen_t *optlen)
{
  return m_factory->Getsockopt (m_socket, level, optname, optval, optlen);
}
int
KernelSocketFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  return m_factory->Getsockname (m_socket, name, namelen);
}
int
KernelSocketFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  return m_factory->Getpeername (m_socket, name, namelen);
}
int
KernelSocketFd::Ioctl (unsigned long request, char *argp)
{
  return m_factory->Ioctl (m_socket, request, argp);
}
int
KernelSocketFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  return m_factory->Bind (m_socket, my_addr, addrlen);
}
int
KernelSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  return m_factory->Connect (m_socket, my_addr, addrlen, m_statusFlags);
}
int
KernelSocketFd::Listen (int backlog)
{
  return m_factory->Listen (m_socket, backlog);
}
int
KernelSocketFd::Shutdown (int how)
{
  return m_factory->Shutdown (m_socket, how);
}
int
KernelSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  return m_factory->Accept (m_socket, my_addr, addrlen, m_statusFlags);
}
void *
KernelSocketFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  GET_CURRENT (start << length << prot << flags << offset);
  current->err = ENODEV;
  return MAP_FAILED;
}
off64_t
KernelSocketFd::Lseek (off64_t offset, int whence)
{
  GET_CURRENT (offset << whence);
  current->err = ESPIPE;
  return -1;
}
int
KernelSocketFd::Fxstat (int ver, struct ::stat *buf)
{
  GET_CURRENT (ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
KernelSocketFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  GET_CURRENT (ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int
KernelSocketFd::Fcntl (int cmd, unsigned long arg)
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
    case F_GETFD:
      return m_fdFlags;
      break;
    case F_SETFD:
      m_fdFlags = arg;
      return 0;
      break;
    default:
      return UnixFd::Fcntl(cmd, arg );
    }
}
int
KernelSocketFd::Settime (int flags,
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
KernelSocketFd::Gettime (struct itimerspec *cur_value) const
{
  NS_LOG_FUNCTION (this << Current () << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}
bool
KernelSocketFd::HangupReceived (void) const
{
  // XXX: TO BE IMPLEMENTED OR NOT :)
  return false;
}
int
KernelSocketFd::Poll (PollTable* ptable)
{
  return m_factory->Poll (m_socket, ptable);
}
int
KernelSocketFd::Ftruncate (off_t length)
{
  NS_LOG_FUNCTION (this << Current ());
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}
int
KernelSocketFd::Fsync (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EBADF;
  return -1;
}
} // namespace ns3
