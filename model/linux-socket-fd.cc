#include "linux-socket-fd.h"
#include "linux-socket-fd-factory.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h> // for MMAP_FAILED

NS_LOG_COMPONENT_DEFINE("LinuxSocketFd");

namespace ns3 {

LinuxSocketFd::LinuxSocketFd (Ptr<LinuxSocketFdFactory> factory, struct SimSocket *socket)
  : m_factory (factory),
    m_socket (socket),
    m_statusFlags (0)
{}

LinuxSocketFd::~LinuxSocketFd ()
{}

int 
LinuxSocketFd::Close (void)
{
  return m_factory->Close (m_socket);
}
ssize_t 
LinuxSocketFd::Write (const void *buf, size_t count)
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
LinuxSocketFd::Read(void *buf, size_t count)
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
LinuxSocketFd::Recvmsg(struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking?MSG_DONTWAIT:0;
  return m_factory->Recvmsg (m_socket, msg, flags);
}
ssize_t 
LinuxSocketFd::Sendmsg(const struct msghdr *msg, int flags)
{
  bool nonBlocking = (m_statusFlags & O_NONBLOCK) == O_NONBLOCK;
  flags |= nonBlocking?MSG_DONTWAIT:0;
  return m_factory->Sendmsg (m_socket, msg, flags);
}
bool 
LinuxSocketFd::Isatty (void) const
{
  return false;
}
int 
LinuxSocketFd::Setsockopt (int level, int optname,
			   const void *optval, socklen_t optlen)
{
  return m_factory->Setsockopt (m_socket, level, optname, optval, optlen);
}
int 
LinuxSocketFd::Getsockopt (int level, int optname,
			   void *optval, socklen_t *optlen)
{
  return m_factory->Getsockopt (m_socket, level, optname, optval, optlen);
}
int 
LinuxSocketFd::Getsockname(struct sockaddr *name, socklen_t *namelen)
{
  return m_factory->Getsockname (m_socket, name, namelen);
}
int 
LinuxSocketFd::Getpeername(struct sockaddr *name, socklen_t *namelen)
{
  return m_factory->Getpeername (m_socket, name, namelen);
}
int 
LinuxSocketFd::Ioctl (int request, char *argp)
{
  return m_factory->Ioctl (m_socket, request, argp);
}
int 
LinuxSocketFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  return m_factory->Bind (m_socket, my_addr, addrlen);
}
int 
LinuxSocketFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  return m_factory->Connect (m_socket, my_addr, addrlen);
}
int 
LinuxSocketFd::Listen (int backlog)
{
  return m_factory->Listen (m_socket, backlog);
}
int 
LinuxSocketFd::Shutdown(int how)
{
  return m_factory->Shutdown (m_socket, how);
}
int 
LinuxSocketFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  return m_factory->Accept (m_socket, my_addr, addrlen);
}
void *
LinuxSocketFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  GET_CURRENT (start << length << prot << flags << offset);
  current->err = ENODEV;
  return MAP_FAILED;
}
off64_t 
LinuxSocketFd::Lseek (off64_t offset, int whence)
{
  GET_CURRENT(offset << whence);
  current->err = ESPIPE;
  return -1;  
}
int 
LinuxSocketFd::Fxstat (int ver, struct ::stat *buf)
{
  GET_CURRENT(ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int 
LinuxSocketFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  GET_CURRENT(ver << buf);
  buf->st_mode = S_IFSOCK;
  buf->st_dev = -1;
  buf->st_blksize = 0;
  return 0;
}
int 
LinuxSocketFd::Fcntl (int cmd, unsigned long arg)
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
LinuxSocketFd::Settime (int flags,
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
LinuxSocketFd::Gettime (struct itimerspec *cur_value) const
{
  NS_LOG_FUNCTION (this << Current () << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}

bool 
LinuxSocketFd::CanRecv (void) const
{
  return m_factory->CanRecv (m_socket);
}
bool 
LinuxSocketFd::CanSend (void) const
{
  return m_factory->CanSend (m_socket);
}
bool
LinuxSocketFd::HangupReceived (void) const
{
  // XXX: TEMPOFUR TO BE IMPLEMENTED OR NOT :)
  return false;
}
} // namespace ns3
