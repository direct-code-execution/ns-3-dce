/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "epoll-fd.h"
#include "process.h"
#include "utils.h"
#include "file-usage.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <string.h>
#include <vector>

NS_LOG_COMPONENT_DEFINE ("DceEpollFd");

namespace ns3 {

// EPOLLIN/OUT/PRI/ERR/HUP/RDHUP have the values of the poll(2) flags.
static const uint32_t EPOLL_POLL_EVENTS = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLRDHUP | EPOLLERR | EPOLLHUP;

EpollFd::EpollFd (int flags)
{
  m_statusFlags = O_RDWR;
  m_fdFlags = (flags & EPOLL_CLOEXEC) ? FD_CLOEXEC : 0;
}

EpollFd::~EpollFd ()
{
}

int
EpollFd::Ctl (int op, int fd, struct epoll_event *event)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << op << fd);
  NS_ASSERT (current != 0);
  std::map<int, FileUsage *>::iterator it = current->process->openFiles.find (fd);
  if (it == current->process->openFiles.end () || it->second->IsClosed ())
    {
      current->err = EBADF;
      return -1;
    }
  if (it->second->GetFile () == this)
    {
      current->err = EINVAL;
      return -1;
    }
  std::map<int, Entry>::iterator entry = m_entries.find (fd);
  switch (op)
    {
    case EPOLL_CTL_ADD:
      if (entry != m_entries.end ())
        {
          current->err = EEXIST;
          return -1;
        }
      if (event == 0)
        {
          current->err = EFAULT;
          return -1;
        }
      {
        Entry e;
        e.events = event->events;
        e.data = event->data;
        e.disabled = false;
        m_entries[fd] = e;
      }
      return 0;
    case EPOLL_CTL_MOD:
      if (entry == m_entries.end ())
        {
          current->err = ENOENT;
          return -1;
        }
      if (event == 0)
        {
          current->err = EFAULT;
          return -1;
        }
      entry->second.events = event->events;
      entry->second.data = event->data;
      entry->second.disabled = false;
      return 0;
    case EPOLL_CTL_DEL:
      if (entry == m_entries.end ())
        {
          current->err = ENOENT;
          return -1;
        }
      m_entries.erase (entry);
      return 0;
    default:
      current->err = EINVAL;
      return -1;
    }
}

int
EpollFd::Scan (struct epoll_event *events, int maxevents, PollTable *table,
               std::map<int, FileUsage *> &referenced)
{
  Thread *current = Current ();
  int count = 0;
  std::vector<int> stale;
  for (std::map<int, Entry>::iterator i = m_entries.begin (); i != m_entries.end (); ++i)
    {
      int fd = i->first;
      Entry &e = i->second;
      std::map<int, FileUsage *>::iterator it = current->process->openFiles.find (fd);
      if (it == current->process->openFiles.end () || it->second->IsClosed ())
        {
          // closed by the application: the kernel removes it from the set
          stale.push_back (fd);
          continue;
        }
      if (e.disabled)
        {
          continue;
        }
      FileUsage *fu = it->second;
      UnixFd *unixFd;
      if (table != 0 && referenced.find (fd) == referenced.end ())
        {
          unixFd = fu->GetFileInc ();
          referenced[fd] = fu;
        }
      else
        {
          unixFd = fu->GetFile ();
        }
      short mask = (e.events & EPOLL_POLL_EVENTS) | POLLERR | POLLHUP;
      if (table != 0)
        {
          table->SetEventMask (mask);
        }
      short ready = unixFd->Poll (table) & mask;
      if (ready != 0 && count < maxevents)
        {
          events[count].events = ready;
          events[count].data = e.data;
          count++;
          if (e.events & EPOLLONESHOT)
            {
              e.disabled = true;
            }
        }
    }
  for (std::vector<int>::iterator i = stale.begin (); i != stale.end (); ++i)
    {
      m_entries.erase (*i);
    }
  return count;
}

int
EpollFd::Wait (struct epoll_event *events, int maxevents, int timeout)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (this << current << maxevents << timeout);
  NS_ASSERT (current != 0);
  if (maxevents <= 0 || events == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  Time endtime;
  if (timeout > 0)
    {
      endtime = Simulator::Now () + MilliSeconds (timeout);
    }
  PollTable *table = new PollTable ();
  std::map<int, FileUsage *> referenced;
  int count = 0;
  int result = 0;
  bool registered = false;
  while (true)
    {
      // Register the entries in the table the first time only, like poll(2).
      count = Scan (events, maxevents, (timeout != 0 && !registered) ? table : 0, referenced);
      registered = true;
      if (count > 0 || timeout == 0)
        {
          result = count;
          break;
        }
      PollTable::Result res;
      if (timeout < 0)
        {
          current->pollTable = table;
          res = table->Wait (Seconds (0));
          current->pollTable = 0;
        }
      else
        {
          Time diff = endtime - Simulator::Now ();
          if (!diff.IsStrictlyPositive ())
            {
              result = 0;
              break;
            }
          current->pollTable = table;
          res = table->Wait (diff);
          current->pollTable = 0;
        }
      if (res == PollTable::INTERRUPTED)
        {
          UtilsDoSignal ();
          current->err = EINTR;
          result = -1;
          break;
        }
      if (res == PollTable::TIMEOUT)
        {
          // last scan without blocking
          timeout = 0;
        }
    }
  table->FreeWait ();
  delete table;
  for (std::map<int, FileUsage *>::iterator i = referenced.begin (); i != referenced.end (); ++i)
    {
      i->second->DecUsage ();
    }
  return result;
}

int
EpollFd::Poll (PollTable* ptable)
{
  // An epoll fd is readable when one of its entries is ready. Registering
  // the entries in the caller's table lets a poll() on the epoll fd wake up.
  struct epoll_event scratch[16];
  std::map<int, FileUsage *> referenced;
  int count = Scan (scratch, 16, ptable, referenced);
  for (std::map<int, FileUsage *>::iterator i = referenced.begin (); i != referenced.end (); ++i)
    {
      i->second->DecUsage ();
    }
  return count > 0 ? POLLIN : 0;
}

int
EpollFd::Close (void)
{
  m_entries.clear ();
  return 0;
}

bool
EpollFd::HangupReceived (void) const
{
  return false;
}

ssize_t
EpollFd::Read (void *buf, size_t count)
{
  Current ()->err = EINVAL;
  return -1;
}
ssize_t
EpollFd::Write (const void *buf, size_t count)
{
  Current ()->err = EINVAL;
  return -1;
}
ssize_t
EpollFd::Recvmsg (struct msghdr *msg, int flags)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
ssize_t
EpollFd::Sendmsg (const struct msghdr *msg, int flags)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
bool
EpollFd::Isatty (void) const
{
  return false;
}
int
EpollFd::Setsockopt (int level, int optname, const void *optval, socklen_t optlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Getsockopt (int level, int optname, void *optval, socklen_t *optlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Getsockname (struct sockaddr *name, socklen_t *namelen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Getpeername (struct sockaddr *name, socklen_t *namelen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Ioctl (unsigned long request, char *argp)
{
  Current ()->err = EINVAL;
  return -1;
}
int
EpollFd::Bind (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Connect (const struct sockaddr *my_addr, socklen_t addrlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Listen (int backlog)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Shutdown (int how)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
int
EpollFd::Accept (struct sockaddr *my_addr, socklen_t *addrlen)
{
  Current ()->err = ENOTSOCK;
  return -1;
}
void *
EpollFd::Mmap (void *start, size_t length, int prot, int flags, off64_t offset)
{
  Current ()->err = ENODEV;
  return MAP_FAILED;
}
off64_t
EpollFd::Lseek (off64_t offset, int whence)
{
  Current ()->err = ESPIPE;
  return -1;
}
int
EpollFd::Fxstat (int ver, struct ::stat *buf)
{
  memset (buf, 0, sizeof (*buf));
  buf->st_mode = S_IFREG | 0600;
  return 0;
}
int
EpollFd::Fxstat64 (int ver, struct ::stat64 *buf)
{
  memset (buf, 0, sizeof (*buf));
  buf->st_mode = S_IFREG | 0600;
  return 0;
}
int
EpollFd::Settime (int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{
  Current ()->err = EINVAL;
  return -1;
}
int
EpollFd::Gettime (struct itimerspec *cur_value) const
{
  Current ()->err = EINVAL;
  return -1;
}
int
EpollFd::Ftruncate (off_t length)
{
  Current ()->err = EINVAL;
  return -1;
}
int
EpollFd::Fsync (void)
{
  Current ()->err = EINVAL;
  return -1;
}

} // namespace ns3
