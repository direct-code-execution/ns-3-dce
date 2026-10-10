/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "dce-epoll.h"
#include "dce-unistd.h"
#include "epoll-fd.h"
#include "event-fd.h"
#include "process.h"
#include "utils.h"
#include "file-usage.h"
#include "ns3/log.h"
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("DceEpoll");

using namespace ns3;

static int
AllocateFd (UnixFd *unixFd)
{
  Thread *current = Current ();
  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      delete unixFd;
      current->err = EMFILE;
      return -1;
    }
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);
  return fd;
}

static EpollFd *
LookupEpoll (int epfd)
{
  Thread *current = Current ();
  std::map<int, FileUsage *>::iterator it = current->process->openFiles.find (epfd);
  if (it == current->process->openFiles.end () || it->second->IsClosed ())
    {
      current->err = EBADF;
      return 0;
    }
  EpollFd *epoll = dynamic_cast<EpollFd *> (it->second->GetFile ());
  if (epoll == 0)
    {
      current->err = EINVAL;
    }
  return epoll;
}

int dce_epoll_create (int size)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << size);
  NS_ASSERT (current != 0);
  if (size <= 0)
    {
      current->err = EINVAL;
      return -1;
    }
  return AllocateFd (new EpollFd (0));
}

int dce_epoll_create1 (int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << flags);
  NS_ASSERT (current != 0);
  if (flags & ~EPOLL_CLOEXEC)
    {
      current->err = EINVAL;
      return -1;
    }
  return AllocateFd (new EpollFd (flags));
}

int dce_epoll_ctl (int epfd, int op, int fd, struct epoll_event *event)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << epfd << op << fd);
  EpollFd *epoll = LookupEpoll (epfd);
  if (epoll == 0)
    {
      return -1;
    }
  return epoll->Ctl (op, fd, event);
}

int dce_epoll_wait (int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << epfd << maxevents << timeout);
  EpollFd *epoll = LookupEpoll (epfd);
  if (epoll == 0)
    {
      return -1;
    }
  return epoll->Wait (events, maxevents, timeout);
}

int dce_epoll_pwait (int epfd, struct epoll_event *events, int maxevents, int timeout,
                     const sigset_t *sigmask)
{
  // The signal mask is not applied: signals are delivered at wait points
  // anyway and interrupt the wait with EINTR.
  return dce_epoll_wait (epfd, events, maxevents, timeout);
}

int dce_eventfd (unsigned int initval, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << initval << flags);
  NS_ASSERT (current != 0);
  if (flags & ~(EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE))
    {
      current->err = EINVAL;
      return -1;
    }
  return AllocateFd (new EventFd (initval, flags));
}

int dce_eventfd_read (int fd, eventfd_t *value)
{
  return dce_read (fd, value, sizeof (eventfd_t)) == (ssize_t) sizeof (eventfd_t) ? 0 : -1;
}

int dce_eventfd_write (int fd, eventfd_t value)
{
  return dce_write (fd, &value, sizeof (eventfd_t)) == (ssize_t) sizeof (eventfd_t) ? 0 : -1;
}
