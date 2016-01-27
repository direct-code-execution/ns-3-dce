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
#include "pipe-fd.h"

NS_LOG_COMPONENT_DEFINE ("DceFd");


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
int dce_open64 (const char *path, int flags, ...)
{
  va_list vl;
  va_start (vl, flags);
  // hope this trick actually works...
  int status = dce_open (path, flags, vl);
  va_end (vl);

  return status;
}

int dce_open (const char *path, int flags, ...)
{
  va_list vl;
  va_start (vl, flags);

  mode_t mode = 0;
  if (flags & O_CREAT)
    {
      mode = va_arg (vl, mode_t);
    }
  va_end (vl);
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

  if ((std::string (path) == "/dev/random") || (std::string (path) == "/dev/urandom")
      || (std::string (path) == "/dev/srandom"))
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

      if (((2 == fd) || (1 == fd)) && (Current ()->process->minimizeFiles))
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
  return dce_open (path, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int dce_unlink_real (const char *pathname)
{
  DEFINE_FORWARDER_PATH (unlink, pathname);
}

void unlink_notify (std::string fullpath)
{
  NS_LOG_FUNCTION ("UNLINK FULL PATH " << fullpath);

  Ptr<SocketFdFactory> factory = Current ()->process->manager->GetObject<LocalSocketFdFactory> ();

  if (0 != factory)
    {
      factory->UnlinkNotify (fullpath);
    }
  factory = Current ()->process->manager->GetObject<SocketFdFactory> ();
  if (0 != factory)
    {
      factory->UnlinkNotify (fullpath);
    }
}

int dce_unlink (const char *pathname)
{
  NS_LOG_FUNCTION (pathname);
  int ret = dce_unlink_real (pathname);

  if (0 == ret)
    {
      std::string fullpath = UtilsGetRealFilePath (pathname);

      unlink_notify (fullpath);
    }

  return ret;
}
int dce_mkdir (const char *pathname, mode_t mode)
{
  mode_t m =  (mode & ~(Current ()->process->uMask));
  DEFINE_FORWARDER_PATH (mkdir, pathname, m);
}
int dce_rmdir (const char *pathname)
{
  DEFINE_FORWARDER_PATH (rmdir, pathname);
}
int dce_access (const char *pathname, int mode)
{
  DEFINE_FORWARDER_PATH (access, pathname, mode);
}
int dce_close (int fd)
{
  int retval = 0;
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  std::map<int,FileUsage *>::iterator it =  current->process->openFiles.find (fd);

  if (it == current->process->openFiles.end ())
    {
      current->err = EBADF;
      return -1;
    }

  FileUsage *fu = current->process->openFiles[fd];

  if (fu->GetFile () && (1 == fu->GetFile ()->GetFdCount ()))
    {
      // If only one process point to file we can really close it
      // else we be closed while the last process close it
      retval = fu->GetFile ()->Close ();
    }
  if (fu->CanForget ())
    {
      // If no thread of this process is using it we can free the corresponding fd entry
      // else we be freed by last thread renoncing of using it
      current->process->openFiles.erase (fd);
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

  OPENED_FD_METHOD (int, Isatty ());
}
char* dce_ttyname (int fd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (char*, Ttyname ());
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

  OPENED_FD_METHOD (ssize_t, Sendmsg (msg, flags))
}

int dce_ioctl (int fd, long unsigned int request, ...)
{
  va_list vl;
  va_start (vl, request);
  char *argp = va_arg (vl, char*);
  va_end (vl);

  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << request << argp);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Ioctl (request, argp))
}
ssize_t dce_write (int fd, const void *buf, size_t count)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << buf << count);
  Thread *current = Current ();
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Write (buf, count))
}

ssize_t dce_writev (int fd, const struct iovec *iov, int iovcnt)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << iov << iovcnt);
  NS_ASSERT (current != 0);

  if (!CheckFdExists (current->process, fd, true))
    {
      NS_LOG_DEBUG ("write error");
      current->err = EBADF;
      return -1;
    }

  size_t count = 0;
  for (int i = 0; i < iovcnt; ++i)
    {
      count += iov[i].iov_len;
    }

  if (count == 0)
    {
      current->err = EINVAL;
      return -1;
    }

  void *buf = malloc (count);
  char *bufp = (char *)buf;
  for (int i = 0; i < iovcnt; ++i)
    {
      memcpy (bufp, iov[i].iov_base, iov[i].iov_len);
      bufp = (char *)bufp + iov[i].iov_len;
    }

  UnixFd *unixFd = current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Write (buf, count);
  FdDecUsage (fd);
  free (buf);

  return retval;
}

ssize_t dce_read (int fd, void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << buf << count);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Read (buf, count))
}
int dce_socket (int domain, int type, int protocol)
{
  Thread *current = Current ();
  DceManager *manager = current->process->manager;
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << domain << type << protocol);
  NS_ASSERT (current != 0);
  NS_ASSERT (manager != 0);

  Ptr<SocketFdFactory>  factory = 0;

  if (domain != AF_UNIX)
    {
      factory = manager->GetObject<SocketFdFactory> ();
    }
  else
    {
      if (type != SOCK_DGRAM && type != SOCK_STREAM)
        {
          current->err = EINVAL;
          return -1;
        }
      factory = manager->GetObject<LocalSocketFdFactory> ();
    }
  UnixFd *socket = factory->CreateSocket (domain, type, protocol);
  if (!socket)
    {

      current->err = EINVAL;
      return -1;
    }

  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }
  if (!socket)
    {
      return -1;
    }
  socket->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, socket);

  return fd;
}
ssize_t dce_readv (int fd, const struct iovec *iov, int iovcnt)
{
  Thread *current = Current ();
  ssize_t ret = 0;

  if ((0 == iov)||(iovcnt < 0))
    {
      current->err = EINVAL;
      return -1;
    }
  for (int b = 0; b < iovcnt; b++)
    {
      ssize_t r = dce_read (fd, iov[b].iov_base, iov[b].iov_len);
      if (r >= 0)
        {
          ret += r;
          if (!r || (r < (ssize_t)iov[b].iov_len))
            {
              return ret;
            }
        }
      else
        {
          if (b == 0)
            {
              return -1;
            }
          else
            {
              return ret;
            }
        }
    }
  return ret;
}
int dce_socketpair (int domain, int type, int protocol, int sv[2])
{
  sv[0] = dce_socket (domain, type, protocol);
  if (sv[0] < 0)
    {
      return -1;
    }

  sv[1] = dce_socket (domain, type, protocol);
  if (sv[1] < 0)
    {
      return -1;
    }

  return 0;
}

int dce_bind (int fd, const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << my_addr << addrlen);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Bind (my_addr, addrlen))
}
int dce_connect (int fd, const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << my_addr << addrlen);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Connect (my_addr, addrlen))
}
int dce_listen (int fd, int backlog)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << backlog);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Listen (backlog))
}
int dce_accept (int fd, struct sockaddr *addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << addr << addrlen);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Accept (addr, addrlen))
}
int dce_shutdown (int fd, int how)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << how);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Shutdown (how))
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

  OPENED_FD_METHOD (ssize_t, Recvmsg (msg, flags))
}
int dce_setsockopt (int fd, int level, int optname,
                    const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Setsockopt (level, optname, optval, optlen))
}
int dce_getsockopt (int fd, int level, int optname,
                    void *optval, socklen_t *optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << level << optname << optval << optlen);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Getsockopt (level, optname, optval, optlen))
}
int dce_getsockname (int fd, struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << name << namelen);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Getsockname (name, namelen))
}
int dce_getpeername (int fd, struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << name << namelen);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Getpeername (name, namelen))
}
int dce_dup (int oldfd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << oldfd);
  NS_ASSERT (current != 0);

  if (!CheckFdExists (current->process, oldfd, true))
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

  UnixFd *unixFd = current->process->openFiles[oldfd]->GetFile ();
  unixFd->IncFdCount ();
  unixFd->Ref ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);

  return fd;
}
int dce_dup2 (int oldfd, int newfd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << oldfd << newfd);
  NS_ASSERT (current != 0);
  if (!CheckFdExists (current->process, oldfd, true) || (newfd > MAX_FDS))
    {
      current->err = EBADF;
      return -1;
    }

  if (oldfd == newfd)
    {
      return newfd;
    }
  if (CheckFdExists (current->process, newfd, true))
    {
      if (dce_close (newfd))
        {
          current->err = EBADF;
          return -1;
        }
    }
  if (CheckFdExists (current->process, newfd, false))
    {
      current->err = EBADF;
      return -1;
    }

  UnixFd *unixFd = current->process->openFiles[oldfd]->GetFile ();
  unixFd->IncFdCount ();
  unixFd->Ref ();
  current->process->openFiles[newfd] = new FileUsage (newfd, unixFd);

  return newfd;
}
void * dce_mmap (void *addr, size_t length, int prot, int flags,
                 int fd, off_t offset)
{
  return dce_mmap64 (addr, length, prot, flags, fd, offset);
}

void * dce_mmap64 (void *start, size_t length, int prot, int flags,
                   int fd, off64_t offset)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << start << length << prot << flags << fd << offset);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD_ERR (MAP_FAILED, void *, Mmap (start, length, prot, flags, offset))
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

  OPENED_FD_METHOD (int, Lseek (offset, whence))
}
int dce_fcntl (int fd, int cmd, ... /*unsigned long arg*/)
{
  va_list vl;
  va_start (vl, cmd);
  unsigned long arg = va_arg (vl, unsigned long);
  va_end (vl);

  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << cmd << arg);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  // XXX: we should handle specially some fcntl commands.
  // For example, FD_DUP, etc.

  switch (cmd)
    {
    case F_DUPFD:
      {
        return dce_dup (fd);
      }
    }


  OPENED_FD_METHOD (int, Fcntl (cmd, arg))
}

int dce_truncate (const char *path, off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << path << length);

  int fd = dce_open (path, O_WRONLY, 0);
  if (fd == -1)
    {
      current->err = errno;
      return -1;
    }

  return dce_ftruncate (fd, length);
}

int dce_ftruncate (int fd, off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << length);
  /*
  int index = UtilsSearchOpenFd (fildes);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Ftruncate (length);
  return retval;
  */
  OPENED_FD_METHOD (int, Ftruncate (length))
}
int dce_ftruncate64 (int fd, off_t length)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << length);
  return dce_ftruncate (fd, length);
}

int dce_pipe (int pipefd[2])
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());

  if (0 == pipefd)
    {
      current->err = EFAULT;
      return -1;
    }
  int fdRead =  UtilsAllocateFd ();
  if (fdRead == -1)
    {
      current->err = EMFILE;
      return -1;
    }
  PipeFd *reader = new PipeFd ();

  if (!reader)
    {
      current->err = EMFILE;
      return -1;
    }
  current->process->openFiles[fdRead] = new FileUsage (fdRead, reader);

  int fdWrite =  UtilsAllocateFd ();
  if (fdWrite == -1)
    {
      delete current->process->openFiles[fdRead];
      current->process->openFiles.erase (fdRead);
      delete reader;
      current->err = EMFILE;
      return -1;
    }

  PipeFd *writer = new PipeFd (reader);

  if (!writer)
    {
      delete current->process->openFiles[fdRead];
      current->process->openFiles.erase (fdRead);
      delete reader;
      current->err = EMFILE;
      return -1;
    }
  current->process->openFiles[fdWrite] = new FileUsage (fdWrite, writer);

//  writer->m_peer = reader;
  reader->IncFdCount ();
  writer->IncFdCount ();

  pipefd [0] = fdRead;
  pipefd [1] = fdWrite;

  return 0;
}
ssize_t dce_pread (int fd, void *buf, size_t count, off_t offset)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());

  off_t currentOffset = dce_lseek (fd, 0, SEEK_CUR);
  if (-1 == currentOffset)
    {
      return (ssize_t) -1;
    }
  off_t res = dce_lseek (fd, offset, SEEK_SET);
  if (res != offset)
    {
      current->err = EINVAL;
      return (ssize_t) -1;
    }
  ssize_t ret = dce_read (fd, buf, count);
  if (-1 == ret)
    {
      return ret;
    }
  // Return to original offset.
  res = dce_lseek (fd, currentOffset, SEEK_SET);
  if (res != currentOffset)
    {
      return (ssize_t) -1;
    }
  return ret;
}
ssize_t dce_pwrite (int fd, const void *buf, size_t count, off_t offset)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());

  off_t currentOffset = dce_lseek (fd, 0, SEEK_CUR);
  if (-1 == currentOffset)
    {
      return (ssize_t) -1;
    }
  off_t res = dce_lseek (fd, offset, SEEK_SET);
  if (res != offset)
    {
      current->err = EINVAL;
      return (ssize_t) -1;
    }
  ssize_t ret = dce_write (fd, buf, count);
  if (-1 == ret)
    {
      return ret;
    }
  // Return to original offset.
  res = dce_lseek (fd, currentOffset, SEEK_SET);
  if (res != currentOffset)
    {
      return (ssize_t) -1;
    }
  return ret;
}
int dce_fsync (int fd)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  OPENED_FD_METHOD (int, Fsync ())
}
