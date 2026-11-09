/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef EVENT_FD_H
#define EVENT_FD_H

#include "unix-fd.h"
#include <stdint.h>

namespace ns3 {

/**
 * \brief An eventfd(2) object: a 64 bit counter that read() drains and
 * write() increases, with POLLIN while the counter is non zero.
 *
 * Both the normal and the EFD_SEMAPHORE modes are supported.
 */
class EventFd : public UnixFd
{
public:
  EventFd (unsigned int initval, int flags);
  virtual ~EventFd ();

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
  short Events (void) const;

  uint64_t m_counter;
  bool m_semaphore;
};

} // namespace ns3

#endif /* EVENT_FD_H */
