#include "unix-file-fd.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "dce-node-context.h"
#include "poll.h"

NS_LOG_COMPONENT_DEFINE ("UnixFileFd");

namespace ns3 {

UnixFileFdBase::UnixFileFdBase (int realFd)
  : m_realFd (realFd)
{
}
UnixFileFdBase::~UnixFileFdBase ()
{
  NS_LOG_FUNCTION (this << m_realFd);
  m_realFd = -1;
}
int
UnixFileFdBase::PeekRealFd (void) const
{
  return m_realFd;
}
ssize_t
UnixFileFdBase::Write (const void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << buf << count);
  NS_ASSERT (current != 0);
  ssize_t result = ::write (m_realFd, buf, count);
  if (result == -1)
    {
      current->err = errno;
    }
  return result;
}
ssize_t
UnixFileFdBase::Read (void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << buf << count);
  NS_ASSERT (current != 0);
  ssize_t result = ::read (m_realFd, buf, count);
  if (result == -1)
    {
      current->err = errno;
    }
  return result;
}

ssize_t
UnixFileFdBase::Recvmsg (struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
ssize_t
UnixFileFdBase::Sendmsg (const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
bool
UnixFileFdBase::Isatty (void) const
{
  return ::isatty (m_realFd);
}
int
UnixFileFdBase::Setsockopt (int level, int optname,
                            const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
int
UnixFileFdBase::Getsockopt (int level, int optname,
                            void *optval, socklen_t *optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
int
UnixFileFdBase::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << name << *namelen);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
int
UnixFileFdBase::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << name << *namelen);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}

int
UnixFileFdBase::Ioctl (unsigned long request, char *argp)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = EINVAL;
  return -1;
}
int
UnixFileFdBase::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
int
UnixFileFdBase::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
int
UnixFileFdBase::Listen (int backlog)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
int
UnixFileFdBase::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << my_addr << addrlen);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
int
UnixFileFdBase::Shutdown (int how)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << how);
  NS_ASSERT (current != 0);
  current->err = ENOTSOCK;
  return -1;
}
void *
UnixFileFdBase::Mmap (void *start, size_t length, int prot, int flags,
                      off64_t offset)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);

  void *retval = ::mmap (start, length, prot, flags, m_realFd, offset);
  if (retval == 0)
    {
      current->err = errno;
    }
  return retval;
}
off64_t
UnixFileFdBase::Lseek (off64_t offset, int whence)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << offset << whence);
  NS_ASSERT (current != 0);
  off64_t retval = ::lseek64 (m_realFd, offset, whence);
  if (retval == -1)
    {
      current->err = errno;
    }
  return retval;
}
int
UnixFileFdBase::Fxstat (int ver, struct ::stat *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << buf);
  NS_ASSERT (current != 0);
  int retval = ::__fxstat (ver, m_realFd, buf);
  if (retval == -1)
    {
      current->err = errno;
    }
  return retval;
}
int
UnixFileFdBase::Fxstat64 (int ver, struct ::stat64 *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << buf);
  NS_ASSERT (current != 0);
  int retval = ::__fxstat64 (ver, m_realFd, buf);
  if (retval == -1)
    {
      current->err = errno;
    }
  return retval;
}
int
UnixFileFdBase::Fcntl (int cmd, unsigned long arg)
{
  NS_LOG_FUNCTION (this << Current () << cmd << arg);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int retval = ::fcntl (m_realFd, cmd, arg);
  if (retval == -1)
    {
      current->err = errno;
    }
  return retval;
}
int
UnixFileFdBase::Settime (int flags,
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
UnixFileFdBase::Gettime (struct itimerspec *cur_value) const
{
  NS_LOG_FUNCTION (this << Current () << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  current->err = EINVAL;
  return -1;
}


bool
UnixFileFdBase::CanRecv (void) const
{
  NS_LOG_FUNCTION (this << " fd:" << m_realFd);
  // Must do a real select
  fd_set readFd;
  struct timeval timeOut;
  int ret = 0;

  timeOut.tv_sec = 0;
  timeOut.tv_usec = 0;
  FD_ZERO (&readFd);
  FD_SET (m_realFd, &readFd);

  ret = select (1 + m_realFd, &readFd, NULL, NULL, &timeOut);

  return (ret == 1) && (FD_ISSET (m_realFd, &readFd));
}
bool
UnixFileFdBase::CanSend (void) const
{
  NS_LOG_FUNCTION (this << " fd:" << m_realFd);
  // Must do a real select
  fd_set writeFd;
  struct timeval timeOut;
  int ret = 0;

  timeOut.tv_sec = 0;
  timeOut.tv_usec = 0;
  FD_ZERO (&writeFd);
  FD_SET (m_realFd, &writeFd);

  ret = select (1 + m_realFd, &writeFd, NULL, NULL, &timeOut);

  return (ret == 1) && (FD_ISSET (m_realFd, &writeFd));
}
bool
UnixFileFdBase::HangupReceived (void) const
{
  return false;
}

int
UnixFileFdBase::Ftruncate (off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (this << current << length);

  int retval = ::ftruncate (m_realFd, length);
  if (retval == -1)
    {
      current->err = errno;
    }
  return retval;
}
int
UnixFileFdBase::GetRealFd (void) const
{
  return m_realFd;
}

UnixFileFd::UnixFileFd (int realFd)
  : UnixFileFdBase (realFd)
{
}
UnixFileFd::~UnixFileFd ()
{
  if (PeekRealFd () != -1)
    {
      ::close (PeekRealFd ());
    }
}
int
UnixFileFd::Close (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  int result = ::close (PeekRealFd ());
  if (result == -1)
    {
      current->err = errno;
    }
  // caller is responsible for removing this fd from process
  // list of fds and deleting this class instance.
  return result;
}

int
UnixFileFdBase::Poll (PollTable* ptable)
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

UnixFileFdLight::UnixFileFdLight (std::string path) : m_path (path),
                                                      UnixFileFdBase (-1)
{

}

UnixFileFdLight::~UnixFileFdLight ()
{
  m_path = "";
}

ssize_t
UnixFileFdLight::Write (const void *buf, size_t count)
{
  FILE *f = fopen (m_path.c_str (), "a");

  if (!f)
    {
      Current ()->err = errno;
      return -1;
    }

  ssize_t res = fwrite (buf, count, 1, f);

  fclose (f);

  return res;
}

int
UnixFileFdLight::Close (void)
{
  return 0;
}

bool
UnixFileFdLight::CanSend (void) const
{
  return true;
}

TermUnixFileFd::TermUnixFileFd (int realFd)
  : UnixFileFdBase (realFd)
{
}
TermUnixFileFd::~TermUnixFileFd ()
{
}
int
TermUnixFileFd::Close (void)
{
  return 0;
}

UnixRandomFd::UnixRandomFd (std::string devPath) : m_devPath (devPath),
                                                   UnixFileFdBase (-1)
{
}

UnixRandomFd::~UnixRandomFd ()
{

}

ssize_t
UnixRandomFd::Read (void *buf, size_t count)
{
  Ptr<DceNodeContext> nodeContext = DceNodeContext::GetNodeContext ();
  NS_ASSERT (0 != nodeContext);

  return nodeContext->RandomRead (buf, count);
}

bool
UnixRandomFd::CanRecv (void) const
{
  return true;
}

int
UnixRandomFd::Close (void)
{
  return 0;
}
bool
UnixRandomFd::CanSend (void) const
{
  return true;
}
ssize_t
UnixRandomFd::Write (const void *buf, size_t count)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  current->err = EBADF;

  return -1;
}
int
UnixRandomFd::Fxstat (int ver, struct ::stat *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << buf);
  int tmpFd = open (m_devPath.c_str (), O_RDONLY, 0);

  if (tmpFd < 0)
    {
      current->err = errno;
      return -1;
    }

  NS_ASSERT (current != 0);
  int retval = ::__fxstat (ver, tmpFd, buf);
  if (retval == -1)
    {
      current->err = errno;
    }
  close (tmpFd);
  return retval;
}
int
UnixRandomFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << buf);
  int tmpFd = open (m_devPath.c_str (), O_RDONLY, 0);

  if (tmpFd < 0)
    {
      current->err = errno;
      return -1;
    }

  NS_ASSERT (current != 0);
  int retval = ::__fxstat64 (ver, tmpFd, buf);
  if (retval == -1)
    {
      current->err = errno;
    }
  close (tmpFd);
  return retval;
}
int
UnixFileFdBase::Fsync (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current);
  NS_ASSERT (current != 0);
  int retval = ::fsync (m_realFd);
  if (retval == -1)
    {
      current->err = errno;
    }
  return retval;
}
} // namespace ns3
