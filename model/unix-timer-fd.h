#ifndef UNIX_TIMER_FD_H
#define UNIX_TIMER_FD_H

#include "unix-fd.h"
#include "process.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"

namespace ns3 {

class UnixTimerFd : public UnixFd
{
public:
  UnixTimerFd (int clockid, int flags);

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
  virtual int Fcntl (int cmd, unsigned long arg);
  virtual int Settime (int flags,
                       const struct itimerspec *new_value,
                       struct itimerspec *old_value);
  virtual int Gettime (struct itimerspec *cur_value) const;
  virtual int Ftruncate (off_t length);

  virtual bool CanRecv (void) const;
  virtual bool CanSend (void) const;
  virtual bool HangupReceived (void) const;
  virtual int Poll (PollTable* ptable);
  virtual int Fsync (void);

private:
  void TimerExpired (void);
  void SettimeMain (Time t);

  Time m_period;
  uint64_t m_skipped;
  EventId m_timer;
  Thread * m_waiter;
};

} // namespace ns3

#endif /* UNIX_TIMER_FD_H */
