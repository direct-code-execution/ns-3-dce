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

NS_LOG_COMPONENT_DEFINE ("SimuFd");


#ifdef _K_SS_MAXSIZE
#define SOCK_MAX_ADDRESS_SIZE _K_SS_MAXSIZE
#else
#define SOCK_MAX_ADDRESS_SIZE 128
#endif

#define DEFINE_FORWARDER_PATH(name, pathname, ...)			\
  {									\
    Thread *current = Current ();					\
    NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << pathname);	\
    NS_ASSERT (Current () != 0);						\
    									\
    if (std::string (pathname) == std::string(""))			\
      {									\
        current->err = ENOENT;						\
	return -1;							\
      }									\
    std::string fullpath = UtilsGetRealFilePath (pathname);	\
    int status = ::name (fullpath.c_str (), ##__VA_ARGS__);		\
    if (status == -1)							\
      {									\
	current->err = errno;						\
	return -1;							\
      }									\
    return status;							\
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

  std::string fullpath = UtilsGetRealFilePath (path);
  int realFd = ::open (fullpath.c_str (), flags, mode);
  if (realFd == -1)
    {
      current->err = errno;
      return -1;
    }
  UnixFd *unixFd = new UnixFileFd (realFd);
  current->process->openFiles.push_back (std::make_pair(fd,unixFd));
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
  int ret = dce_unlink_real (pathname);

  if (0 == ret)
    {
      Ptr<LocalSocketFdFactory> factory = Current ()->process->manager->GetObject<LocalSocketFdFactory> ();

      if ( 0 != factory )
        {
          factory->UnRegisterBinder (UtilsGetRealFilePath (pathname) );
        }
    }
  return ret;
}
int dce_mkdir(const char *pathname, mode_t mode)
{
  DEFINE_FORWARDER_PATH (mkdir, pathname, mode);
}
int dce_rmdir(const char *pathname)
{
  DEFINE_FORWARDER_PATH (rmdir, pathname);
}
int dce_close (int fd)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = 0;
  if (unixFd->GetReferenceCount () == 1)
    {
      // Note to the attentive reader: the logical and clean way to handle this call
      // to Close would be to move it to the UnixFd destructor and make the Close method
      // private so that it is invoked automatically upon the last call to Unref below.
      // However, we do not do this and for good reasons: one must never invoke virtual
      // methods from a destructor unless one is prepared to suffer considerably.
      retval = unixFd->Close ();
    }
  unixFd->Unref ();
  current->process->openFiles[index].second = 0;
  current->process->openFiles[index].first = -1;
  current->process->openFiles.erase(current->process->openFiles.begin() + index);
  return retval;
}
int dce_isatty(int fd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Isatty ();
  return retval;
}
ssize_t dce_send(int fd, const void *buf, size_t len, int flags)
{
  NS_LOG_FUNCTION (fd << buf << len << flags);
  return dce_sendto (fd, buf, len, flags, 0, 0);
}
ssize_t dce_sendto(int fd, const void *buf, size_t len, int flags,
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
ssize_t dce_sendmsg(int fd, const struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << msg << flags);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  ssize_t retval = unixFd->Sendmsg (msg, flags);
  return retval;
}
int dce_ioctl (int fd, int request, char *argp)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << request << argp);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Ioctl (request, argp);
  return retval;
}
int dce_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fds << nfds << timeout);
  NS_ASSERT (current != 0);
  for (uint32_t i = 0; i < nfds; ++i)
    {
      // initialize all outgoing events.
      fds[i].revents = 0;
    }
  for (uint32_t i = 0; i < nfds; ++i)
    {
      int index = UtilsSearchOpenFd (fds[i].fd);
      if (index == -1)
	{
	  current->err = EBADF;
	  return -1;
	}
    }
  bool mustWait = false;
  Waiter waiter;
  for (uint32_t i = 0; i < nfds; ++i)
    {
      int index = UtilsSearchOpenFd (fds[i].fd);
      NS_ASSERT (index != -1);
      UnixFd *unixFd = current->process->openFiles[index].second;
      if (fds[i].events & POLLIN && !unixFd->CanRecv ())
	{
	  unixFd->SetRecvWaiter (&waiter);
	  mustWait = true;
	}
      if (fds[i].events & POLLOUT && !unixFd->CanSend ())
	{
	  unixFd->SetSendWaiter (&waiter);
	  mustWait = true;
	}
    }
  if (mustWait)
    {
      waiter.SetTimeout (MilliSeconds (timeout));
      if (!waiter.WaitDoSignal ())
	{
	  // current->err set by call above.
	  for (uint32_t i = 0; i < nfds; ++i)
	    {
	      // make sure we remove all waiters we setup earlier.
	      int index = UtilsSearchOpenFd (fds[i].fd);
	      NS_ASSERT (index != -1);
	      UnixFd *unixFd = current->process->openFiles[index].second;
	      if (fds[i].events & POLLOUT)
		{
		  unixFd->SetSendWaiter (0);
		}
	      if (fds[i].events & POLLIN)
		{
		  unixFd->SetRecvWaiter (0);
		}
	    }
	  return -1;
	}
    }
  int retval = 0;
  for (uint32_t i = 0; i < nfds; ++i)
    {
      int index = UtilsSearchOpenFd (fds[i].fd);
      NS_ASSERT (index != -1);
      UnixFd *unixFd = current->process->openFiles[index].second;
      if (fds[i].events & POLLOUT && unixFd->CanSend ())
	{
	  unixFd->SetSendWaiter (0);
	  fds[i].revents |= POLLOUT;
	  retval++;
	}
      if (fds[i].events & POLLIN && unixFd->CanRecv ())
	{
	  unixFd->SetRecvWaiter (0);
	  fds[i].revents |= POLLIN;
	  retval++;
	}
    }
  return retval;
}
int dce_select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nfds << timeout);
  NS_ASSERT (current != 0);

  if (nfds == -1)
    {
      current->err = EINVAL;
      return -1;
    }
  if (readfds == 0 && writefds == 0 && exceptfds == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  if (timeout)
    {
      if(timeout->tv_sec < 0 || timeout->tv_usec < 0)
        {
          current->err = EINVAL;
          return -1;
        }
    }
  for (int fd = 0; fd < nfds; fd++)
    {
      if ((readfds != 0 && FD_ISSET(fd, readfds))
          ||(writefds != 0 &&  FD_ISSET (fd, writefds))
          ||(exceptfds != 0 && FD_ISSET (fd, exceptfds)))
        {
          int index = UtilsSearchOpenFd (fd);
          if (index == -1)
            {
              current->err = EBADF;
              return -1;
            }
        }
    }

  bool mustWait = true;
  Waiter waiter;
  Time timeoutLeft = Seconds (0.0);

  if (timeout && (timeout->tv_sec == 0) && (timeout->tv_usec == 0))
    {
      mustWait = false;
    }

  if (mustWait)
    {
      // First Pass: seek if there is at least one fd ready
      bool someAreReady = false;

      for (int fd = 0; fd < nfds; fd++)
        {
          if (readfds != 0 && FD_ISSET(fd, readfds))
            {
              int index = UtilsSearchOpenFd (fd);
              NS_ASSERT (index != -1);
              UnixFd *unixFd = current->process->openFiles[index].second;
              if (unixFd->CanRecv ())
                {
                  someAreReady = true;
                  break;
                }
            }
          if (writefds != 0 && FD_ISSET (fd, writefds))
            {
              int index = UtilsSearchOpenFd (fd);
              NS_ASSERT (index != -1);
              UnixFd *unixFd = current->process->openFiles[index].second;
              if (unixFd->CanSend ())
                {
                  someAreReady = true;
                  break;
                }
            }
        }

      if ( !someAreReady )
        { // We should wait so set the waiters ...
          for (int fd = 0; fd < nfds; fd++)
            {
              if (readfds != 0 && FD_ISSET(fd, readfds))
                {
                  int index = UtilsSearchOpenFd (fd);
                  NS_ASSERT (index != -1);
                  UnixFd *unixFd = current->process->openFiles[index].second;
                  if (!unixFd->CanRecv ())
                    {
                      unixFd->SetRecvWaiter (&waiter);
                    }
                }
              // not else because an fd can be in read and write at same time
              if (writefds != 0 && FD_ISSET (fd, writefds))
                {
                  int index = UtilsSearchOpenFd (fd);
                  NS_ASSERT (index != -1);
                  UnixFd *unixFd = current->process->openFiles[index].second;
                  if (!unixFd->CanSend ())
                    {
                      unixFd->SetSendWaiter (&waiter);
                    }
                }
            }

          waiter.SetTimeout (UtilsTimevalToTime (timeout));
          Waiter::Result result = waiter.Wait ();

          for (int fd = 0; fd < nfds; fd++)
            {
              // cleanup all waiters setup previously
              if (readfds != 0 && FD_ISSET(fd, readfds))
                {
                  int index = UtilsSearchOpenFd (fd);
                  if (index != -1) // no ASSERT because Meanwhile the fd should disappear
                    {
                      UnixFd *unixFd = current->process->openFiles[index].second;
                      unixFd->SetRecvWaiter (0);
                    }
                  else
                    {
                      continue;
                    }
                }
              if (writefds != 0 && FD_ISSET (fd, writefds))
                {
                  int index = UtilsSearchOpenFd (fd);
                  if (index != -1) // no ASSERT because Meanwhile the fd should disappear
                    {
                      UnixFd *unixFd = current->process->openFiles[index].second;
                      unixFd->SetSendWaiter (0);
                    }
                }
            }
          if (Waiter::INTERRUPTED == result)
            {
              UtilsDoSignal ();
              current->err = EINTR;
              return -1;
            }
          timeoutLeft = waiter.GetTimeoutLeft ();
        }
    }

  int retval = 0;
  fd_set rd, wt;
  FD_ZERO(&rd);
  FD_ZERO(&wt);

   for (int fd = 0; fd < nfds; fd++)
    {    
      int index = UtilsSearchOpenFd (fd);
      if (index == -1)
        continue;
      UnixFd *unixFd = current->process->openFiles[index].second;
      if (readfds != 0 && FD_ISSET(fd, readfds))
        {
          if (unixFd->CanRecv ())
            {
              FD_SET(fd, &rd);
              retval++;
            }
        }
      if (writefds != 0 && FD_ISSET(fd, writefds))
        {
          if (unixFd->CanSend ())
            {
              FD_SET(fd, &wt);
              retval ++;
            }
        }
    }

  if (readfds != 0)
    {
      memcpy(readfds, &rd, sizeof(rd));
    }
  if (writefds != 0)
    {
      memcpy(writefds, &wt, sizeof(wt));
    }
  if (exceptfds != 0)
    {
      FD_ZERO(exceptfds);
    }
  if (timeout != 0)
    {
      *timeout = UtilsTimeToTimeval (timeoutLeft);
    }
  return retval;
}


ssize_t dce_write (int fd, const void *buf, size_t count)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << buf << count);
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      NS_LOG_DEBUG ("write error");
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Write (buf, count);
  NS_LOG_DEBUG ("write " << retval << "bytes");
  return retval;
}

ssize_t dce_writev (int fd, const struct iovec *iov, int iovcnt)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << iov << iovcnt);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
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

  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Write (buf, count);

  return retval;
}

ssize_t dce_read (int fd, void *buf, size_t count)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << buf << count);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Read (buf, count);
  return retval;
}
int dce_socket (int domain, int type, int protocol)
{
  Thread *current = Current ();
  DceManager *manager = current->process->manager;
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << domain << type << protocol);
  NS_ASSERT (current != 0);
  NS_ASSERT (manager != 0);

  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }

  Ptr<SocketFdFactory>  factory = 0 ;

  if (domain != AF_UNIX)
    {
      factory = manager->GetObject<SocketFdFactory> ();
  }
  else
    {
      factory = manager->GetObject<LocalSocketFdFactory> ();
      if (0 == factory)
        {
          factory = CreateObject<LocalSocketFdFactory> ();
          manager->AggregateObject(factory);
        }
    }
  UnixFd *socket = factory->CreateSocket (domain, type, protocol);

  current->process->openFiles.push_back (std::make_pair (fd, socket));

  return fd;
}
int dce_bind (int fd, const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << my_addr << addrlen);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Bind (my_addr, addrlen);
  return retval;
}
int dce_connect (int fd, const struct sockaddr *my_addr, socklen_t addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << my_addr << addrlen);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Connect (my_addr, addrlen);
  return retval;
}
int dce_listen (int sockfd, int backlog)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sockfd << backlog);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (sockfd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Listen (backlog);
  return retval;
}
int dce_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << sockfd << addr << addrlen);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (sockfd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Accept (addr, addrlen);
  return retval;  
}
int dce_shutdown(int s, int how)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << s << how);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (s);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Shutdown (how);
  return retval;
}
ssize_t dce_recv (int fd, void *buf, size_t count, int flags)
{
  NS_LOG_FUNCTION (fd << buf << count << flags);
  return dce_recvfrom (fd, buf, count, flags, 0, 0);
}
ssize_t dce_recvfrom(int fd, void *buf, size_t len, int flags,
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
ssize_t dce_recvmsg(int fd, struct msghdr *msg, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << msg << flags);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  ssize_t retval = unixFd->Recvmsg (msg, flags);
  return retval;
}
int dce_setsockopt(int fd, int level, int optname,
		    const void *optval, socklen_t optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << level << optname << optval << optlen);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Setsockopt (level, optname, optval, optlen);
  return retval;
}
int dce_getsockopt(int fd, int level, int optname,
		    void *optval, socklen_t *optlen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd << level << optname << optval << optlen);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Getsockopt (level, optname, optval, optlen);
  return retval;
}
int dce_getsockname(int fd, struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << name << namelen);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Getsockname (name, namelen);
  return retval;
}
int dce_getpeername(int fd, struct sockaddr *name, socklen_t *namelen)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << name << namelen);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Getpeername (name, namelen);
  return retval;
}
int dce_dup(int oldfd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << oldfd);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (oldfd);
  if (index == -1)
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
  UnixFd *unixFd = current->process->openFiles[index].second;
  unixFd->Ref ();
  current->process->openFiles.push_back (std::make_pair (fd, unixFd));
  return fd;
}
int dce_dup2(int oldfd, int newfd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << oldfd << newfd);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (oldfd);
  if (index == -1 || 
      newfd > MAX_FDS)
    {
      current->err = EBADF;
      return -1;
    }
  if (UtilsSearchOpenFd (newfd) != -1)
    {
      dce_close (newfd);
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  unixFd->Ref ();
  current->process->openFiles.push_back (std::make_pair (newfd, unixFd));
  return newfd;
}
void *dce_mmap64 (void *start, size_t length, int prot, int flags,
		   int fd, off64_t offset)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << start << length << prot << flags << fd << offset);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return MAP_FAILED;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  void * retval = unixFd->Mmap (start, length, prot, flags, offset);
  return retval;  
}
int dce_munmap(void *start, size_t length)
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
off_t dce_lseek(int fildes, off_t offset, int whence)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fildes << offset << whence);
  return dce_lseek64 (fildes, offset, whence);
}
off64_t dce_lseek64(int fildes, off64_t offset, int whence)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fildes << offset << whence);
  NS_ASSERT (current != 0);
  int index = UtilsSearchOpenFd (fildes);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[index].second;
  off64_t retval = unixFd->Lseek (offset, whence);
  NS_LOG_DEBUG (retval);
  return retval;    
}
int dce_fcntl(int fd, int cmd, unsigned long arg)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << cmd << arg);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  int index = UtilsSearchOpenFd (fd);
  if (index == -1)
    {
      current->err = EBADF;
      return -1;
    }
  // XXX: we should handle specially some fcntl commands.
  // For example, FD_DUP, etc.
  UnixFd *unixFd = current->process->openFiles[index].second;
  int retval = unixFd->Fcntl (cmd, arg);
  return retval;    
}
