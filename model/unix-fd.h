#ifndef UNIX_FD_H
#define UNIX_FD_H

#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
//#include "ns3/ref-count-base.h"
#include "ns3/object.h"

namespace ns3 {

class Waiter;

class UnixFd: public Object
{
public:
  static TypeId GetTypeId (void);
  virtual int Close (void) = 0;
  virtual ssize_t Write (const void *buf, size_t count) = 0;
  virtual ssize_t Read(void *buf, size_t count) = 0;
  virtual ssize_t Recvmsg(struct msghdr *msg, int flags) = 0;
  virtual ssize_t Sendmsg(const struct msghdr *msg, int flags) = 0;
  virtual bool Isatty (void) const = 0;
  virtual int Setsockopt (int level, int optname,
			  const void *optval, socklen_t optlen) = 0;
  virtual int Getsockopt (int level, int optname,
			  void *optval, socklen_t *optlen) = 0;
  virtual int Getsockname(struct sockaddr *name, socklen_t *namelen) = 0;
  virtual int Getpeername(struct sockaddr *name, socklen_t *namelen) = 0;
  virtual int Ioctl (int request, char *argp) = 0;
  virtual int Bind (const struct sockaddr *my_addr, socklen_t addrlen) = 0;
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen) = 0;
  virtual int Listen (int backlog) = 0;
  virtual int Shutdown(int how) = 0;
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen) = 0;
  virtual void *Mmap (void *start, size_t length, int prot, int flags, off64_t offset) = 0;
  virtual off64_t Lseek (off64_t offset, int whence) = 0;
  virtual int Fxstat (int ver, struct ::stat *buf) = 0;
  virtual int Fxstat64 (int ver, struct ::stat64 *buf) = 0;
  virtual int Fcntl (int cmd, unsigned long arg) = 0;
  virtual int Settime (int flags,
		       const struct itimerspec *new_value,
		       struct itimerspec *old_value) = 0;
  virtual int Gettime (struct itimerspec *cur_value) const = 0;

  void SetRecvWaiter (Waiter *waiter);
  void SetSendWaiter (Waiter *waiter);
  // CanRecv and CanSend return true only if a following Read or Write will not block,
  // so a remote closed socket should return true because read or write will not block but will fail.
  virtual bool CanRecv (void) const = 0;
  virtual bool CanSend (void) const = 0;
  // Return true if a select on this fd should return POLLHUP
  virtual bool HangupReceived (void) const = 0;

  // File Descriptor Reference count
  void IncRef (void);
  void DecRef (void);
  int32_t GetRef (void) const;


protected:
  UnixFd ();
  void WakeupSend (void);
  void WakeupRecv (void);
private:
  Waiter *m_recvWaiter;
  Waiter *m_sendWaiter;
  int32_t m_refCount;
};

} // namespace ns3

#endif /* UNIX_FD_H */
