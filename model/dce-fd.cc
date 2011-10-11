#define _LARGEFILE64_SOURCE 1
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include "unix-fd.h"
#include "unix-file-fd.h"
#include "socket-fd-factory.h"
#include "waiter.h"
#include "dce-fcntl.h"
#include "dce-unistd.h"
#include "dce-poll.h"
#include "dce-stdio.h"
#include "sys/dce-socket.h"
#include "sys/dce-ioctl.h"
#include "sys/dce-stat.h"
#include "sys/dce-mman.h"
#include "sys/dce-select.h"
#include "ns3/log.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include <fcntl.h>
#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "ns3/node.h"
#include "local-socket-fd-factory.h"
#include "ns3-socket-fd-factory.h"
#include "file-usage.h"
#include "dce-stdlib.h"

NS_LOG_COMPONENT_DEFINE ("SimuFd");


#ifdef _K_SS_MAXSIZE
#define SOCK_MAX_ADDRESS_SIZE _K_SS_MAXSIZE
#else
#define SOCK_MAX_ADDRESS_SIZE 128
#endif

#define DEFINE_FORWARDER_PATH(name, pathname, ...)                      \
  {                                                                     \
    Thread *current = Current ();                                       \
    NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << pathname);      \
    NS_ASSERT (Current () != 0);                                                \
                                                                        \
    if (std::string (pathname) == std::string (""))                      \
      {                                                                 \
        current->err = ENOENT;                                          \
        return -1;                                                      \
      }                                                                 \
    std::string fullpath = UtilsGetRealFilePath (pathname);     \
    int status = ::name (fullpath.c_str (), ## __VA_ARGS__);             \
    if (status == -1)                                                   \
      {                                                                 \
        current->err = errno;                                           \
        return -1;                                                      \
      }                                                                 \
    return status;                                                      \
  }


using namespace ns3;

int dce_open (const char *path, int flags, mode_t mode)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << path << flags);
  NS_ASSERT (current != 0);

  if (std::string (path) == "")
    {
      current->err = ENOENT;
      return -1;
    }

  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }
  UnixFd *unixFd = 0;

  if ( ( std::string (path) == "/dev/random" ) || ( std::string (path) == "/dev/urandom" )
       || ( std::string (path) == "/dev/srandom" ) )
    {
      unixFd = new UnixRandomFd (path);
    }
  else
    {
      std::string fullpath = UtilsGetRealFilePath (path);

      int realFd = ::open (fullpath.c_str (), flags, mode);
      if (realFd == -1)
        {
          current->err = errno;
          return -1;
        }

      if ( ( ( 2 == fd) || ( 1 == fd ) ) && ( Current ()->process->minimizeFiles ) )
        {
          unixFd = new UnixFileFdLight (fullpath);
          close (realFd);
        }
      else
        {
          unixFd = new UnixFileFd (realFd);
        }
    }
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);
  return fd;
}

int dce_creat (const char *path, mode_t mode)
{
  return dce_open (path, O_CREAT|O_WRONLY|O_TRUNC, mode);
}

int dce_unlink_real (const char *pathname)
{
  DEFINE_FORWARDER_PATH (unlink, pathname);
}

int dce_unlink (const char *pathname)
{
  NS_LOG_FUNCTION ( pathname );
  int ret = dce_unlink_real (pathname);

  if (0 == ret)
    {
      std::string fullpath = UtilsGetRealFilePath (pathname);
      Ptr<SocketFdFactory> factory = Current ()->process->manager->GetObject<LocalSocketFdFactory> ();

      if ( 0 != factory )
        {
          factory->UnlinkNotify (fullpath);
        }
      factory = Current ()->process->manager->GetObject<SocketFdFactory> ();
      if ( 0 != factory )
        {
          factory->UnlinkNotify (fullpath);
        }
    }

  return ret;
}
int dce_mkdir (const char *pathname, mode_t mode)
{
  mode_t m =  (mode & ~(Current ()->process->uMask ));
  DEFINE_FORWARDER_PATH (mkdir, pathname, m );
}
int dce_rmdir (const char *pathname)
{
  DEFINE_FORWARDER_PATH (rmdir, pathname);
}
int dce_close (int fd)
{
  int retval = 0;
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  FileUsage *fu = current->process->openFiles[fd];

  if (!fu)
    {
      current->err = EBADF;
      return -1;
    }
  if ( fu->GetFile () && (1 == fu->GetFile ()->GetFdCount () )  )
    {
      // If only one process point to file we can really close it
      // else we be closed while the last process close it
      retval = fu->GetFile ()->Close ();
    }
  if ( fu->CanForget () )
    {
      // If no thread of this process is using it we can free the corresponding fd entry
      // else we be freed by last thread renoncing of using it
      current->process->openFiles[fd] = 0;
      delete fu;
      fu = 0;
    }

  return retval;
}

int dce_isatty (int fd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[fd]->GetFileInc ();
  int retval =  unixFd->Isatty ();
  FdDecUsage (fd);

  return retval;
}
ssize_t dce_send (int fd, const void *buf, size_t len, int flags)
{
  NS_LOG_FUNCTION (fd << buf << len << flags);
  return dce_sendto (fd, buf, len, flags, 0, 0);
}
ssize_t dce_sendto (int fd, const void *buf, size_t len, int flags,
                    const struct sockaddr *to, socklen_t tolen)
{
  NS_LOG_FUNCTION (Current () << fd << buf << len << flags << to << tolen);
  NS_ASSERT (Current () != 0);
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = len;
  iov.iov_base = (void *)buf;
  msg.msg_name = (void *)to;
  msg.msg_namelen = tolen;
  ssize_t retval = dce_sendmsg (fd, &msg, flags);
  return retval;
}
ssize_t dce_sendmsg (int fd, const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << msg << flags);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  ssize_t retval = unixFd->Sendmsg (msg, flags);
  FdDecUsage (fd);

  return retval;
}
int dce_ioctl (int fd, int request, char *argp)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << request << argp);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Ioctl (request, argp);
  FdDecUsage (fd);

  return retval;
}
ssize_t dce_write (int fd, const void *buf, size_t count)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << buf << count);
  Thread *current = Current ();
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      NS_LOG_DEBUG ("write error");
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Write (buf, count);
  NS_LOG_DEBUG ("write " << retval << "bytes");
  FdDecUsage (fd);

  return retval;
}
ssize_t dce_writev (int fd, const struct iovec *iov, int iovcnt)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << iov << iovcnt);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      NS_LOG_DEBUG ("write error");
      current->err = EBADF;
      return -1;
    }

  size_t count = 0;
  for (int i = 0; i < iovcnt; ++i)
    count += iov[i].iov_len;

  if (count == 0) 
    {
      current->err = EINVAL;
      return -1;
    }

  char buf[count], *bufp = buf;
  for (int i = 0; i < iovcnt; ++i)
    {
      memcpy (bufp, iov[i].iov_base, iov[i].iov_len);
      bufp += iov[i].iov_len;
    }

  UnixFd *unixFd = current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Write (buf, count);
  FdDecUsage (fd);

  return retval;
}

ssize_t dce_read (int fd, void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << buf << count);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Read (buf, count);
  FdDecUsage (fd);

  return retval;
}
int dce_socket (int domain, int type, int protocol)
{
  Thread *current = Current ();
  DceManager *manager = current->process->manager;
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << domain << type << protocol);
  NS_ASSERT (current != 0);
  NS_ASSERT (manager != 0);

  Ptr<SocketFdFactory>  factory = 0;

  if ( domain == AF_INET6 )
    {
      current->err = EAFNOSUPPORT;
      return -1;
    }

  if (domain != AF_UNIX)
    {
      factory = manager->GetObject<SocketFdFactory> ();
    }
  else
    {
      if ( type != SOCK_DGRAM && type != SOCK_STREAM )
        {
          current->err = EINVAL;
          return -1;
        }
      factory = manager->GetObject<LocalSocketFdFactory> ();
    }

  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }

  UnixFd *socket = factory->CreateSocket (domain, type, protocol);
  socket->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, socket);

  return fd;
}
int dce_bind (int fd, const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << my_addr << addrlen);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Bind (my_addr, addrlen);
  FdDecUsage (fd);

  return retval;
}
int dce_connect (int fd, const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << my_addr << addrlen);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Connect (my_addr, addrlen);
  FdDecUsage (fd);

  return retval;
}
int dce_listen (int fd, int backlog)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << backlog);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Listen (backlog);
  FdDecUsage (fd);

  return retval;
}
int dce_accept (int fd, struct sockaddr *addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << addr << addrlen);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Accept (addr, addrlen);
  FdDecUsage (fd);

  return retval;
}
int dce_shutdown (int fd, int how)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << how);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Shutdown (how);
  FdDecUsage (fd);

  return retval;
}
ssize_t dce_recv (int fd, void *buf, size_t count, int flags)
{
  NS_LOG_FUNCTION (fd << buf << count << flags);
  return dce_recvfrom (fd, buf, count, flags, 0, 0);
}
ssize_t dce_recvfrom (int fd, void *buf, size_t len, int flags,
                      struct sockaddr *from, socklen_t *fromlen)
{
  NS_LOG_FUNCTION (fd << buf << len << flags << from << fromlen);
  uint8_t address[SOCK_MAX_ADDRESS_SIZE];
  struct msghdr msg;
  struct iovec iov;
  msg.msg_control = 0;
  msg.msg_controllen = 0;
  msg.msg_iovlen = 1;
  msg.msg_iov = &iov;
  iov.iov_len = len;
  iov.iov_base = buf;
  msg.msg_name = address;
  msg.msg_namelen = SOCK_MAX_ADDRESS_SIZE;
  ssize_t retval = dce_recvmsg (fd, &msg, flags);
  if (retval != -1 && from != 0)
    {
      if (*fromlen < msg.msg_namelen)
        {
          Thread *current = Current ();
          current->err = EINVAL;
          return -1;
        }
      else
        {
          *fromlen = msg.msg_namelen;
          memcpy (from, msg.msg_name, msg.msg_namelen);
        }
    }
  return retval;
}
ssize_t dce_recvmsg (int fd, struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << msg << flags);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  ssize_t retval = unixFd->Recvmsg (msg, flags);
  FdDecUsage (fd);

  return retval;
}
int dce_setsockopt (int fd, int level, int optname,
                    const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Setsockopt (level, optname, optval, optlen);
  FdDecUsage (fd);

  return retval;
}
int dce_getsockopt (int fd, int level, int optname,
                    void *optval, socklen_t *optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Getsockopt (level, optname, optval, optlen);
  FdDecUsage (fd);

  return retval;
}
int dce_getsockname (int fd, struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << name << namelen);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Getsockname (name, namelen);
  FdDecUsage (fd);

  return retval;
}
int dce_getpeername (int fd, struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << name << namelen);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Getpeername (name, namelen);
  FdDecUsage (fd);

  return retval;
}
int dce_dup (int oldfd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << oldfd);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[oldfd])||(current->process->openFiles[oldfd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }

  UnixFd *unixFd = current->process->openFiles[fd]->GetFile ();
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);

  return fd;
}
int dce_dup2 (int oldfd, int newfd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << oldfd << newfd);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[oldfd])
      ||
      (current->process->openFiles[oldfd]->IsClosed ())
      ||
      (newfd > MAX_FDS))
    {
      current->err = EBADF;
      return -1;
    }
  if (current->process->openFiles[newfd] )
    {
      dce_close (newfd);
    }
  if (current->process->openFiles[newfd] )
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[oldfd]->GetFile ();
  unixFd->IncFdCount ();
  current->process->openFiles[oldfd] = new FileUsage (newfd, unixFd);

  return newfd;
}
void *dce_mmap64 (void *start, size_t length, int prot, int flags,
                  int fd, off64_t offset)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << start << length << prot << flags << fd << offset);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return MAP_FAILED;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  void * retval = unixFd->Mmap (start, length, prot, flags, offset);
  FdDecUsage (fd);

  return retval;
}
int dce_munmap (void *start, size_t length)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << start << length);
  NS_ASSERT (current != 0);
  int retval = ::munmap (start, length);
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }
  return 0;
}
off_t dce_lseek (int fildes, off_t offset, int whence)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fildes << offset << whence);
  return dce_lseek64 (fildes, offset, whence);
}
off64_t dce_lseek64 (int fd, off64_t offset, int whence)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << offset << whence);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  off64_t retval = unixFd->Lseek (offset, whence);
  NS_LOG_DEBUG (retval);
  FdDecUsage (fd);

  return retval;
}
int dce_fcntl (int fd, int cmd, unsigned long arg)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << cmd << arg);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed ()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  // XXX: we should handle specially some fcntl commands.
  // For example, FD_DUP, etc.
  int retval = unixFd->Fcntl (cmd, arg);
  FdDecUsage (fd);

  return retval;
}


