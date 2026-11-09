/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef EPOLL_FD_H
#define EPOLL_FD_H

#include "unix-fd.h"
#include "file-usage.h"
#include <sys/epoll.h>
#include <map>

namespace ns3 {

/**
 * \brief An epoll(7) instance.
 *
 * It is a set of (fd, requested events, user data) entries. Waiting on it
 * uses the same PollTable mechanism as poll(2) and select(2): every entry
 * is polled and registered in a table, the task sleeps until one of them
 * wakes it up. Events are level-triggered; EPOLLET is accepted and treated
 * as level-triggered, EPOLLONESHOT is honoured.
 */
class EpollFd : public UnixFd
{
public:
  EpollFd (int flags);
  virtual ~EpollFd ();

  int Ctl (int op, int fd, struct epoll_event *event);
  int Wait (struct epoll_event *events, int maxevents, int timeout);

  virtual int Close (void);
  virtual ssize_t Write (const void *buf, size_t count);
  virtual ssize_t Read (void *buf, size_t count);
  virtual ssize_t Recvmsg (struct msghdr *msg, int flags);
  virtual ssize_t Sendmsg (const struct msghdr *msg, int flags);
  virtual bool Isatty (void) const;
  virtual int Setsockopt (int level, int optname,
                          const void *optval, socklen_t optlen);
  virtual int Getsockopt (int level, int optname,
                          void *optval, socklen_t *optlen);
  virtual int Getsockname (struct sockaddr *name, socklen_t *namelen);
  virtual int Getpeername (struct sockaddr *name, socklen_t *namelen);
  virtual int Ioctl (unsigned long request, char *argp);
  virtual int Bind (const struct sockaddr *my_addr, socklen_t addrlen);
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen);
  virtual int Listen (int backlog);
  virtual int Shutdown (int how);
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen);
  virtual void * Mmap (void *start, size_t length, int prot, int flags, off64_t offset);
  virtual off64_t Lseek (off64_t offset, int whence);
  virtual int Fxstat (int ver, struct ::stat *buf);
  virtual int Fxstat64 (int ver, struct ::stat64 *buf);
  virtual int Settime (int flags,
                       const struct itimerspec *new_value,
                       struct itimerspec *old_value);
  virtual int Gettime (struct itimerspec *cur_value) const;
  virtual int Ftruncate (off_t length);
  virtual bool HangupReceived (void) const;
  virtual int Poll (PollTable* ptable);
  virtual int Fsync (void);

private:
  struct Entry
  {
    uint32_t events;
    epoll_data_t data;
    bool disabled; // EPOLLONESHOT fired, waiting for EPOLL_CTL_MOD
  };
  // Poll the entries once: fill events (up to maxevents) and return their
  // number; register them in table if it is not null.
  int Scan (struct epoll_event *events, int maxevents, PollTable *table,
            std::map<int, FileUsage *> &referenced);

  std::map<int, Entry> m_entries;
};

} // namespace ns3

#endif /* EPOLL_FD_H */
