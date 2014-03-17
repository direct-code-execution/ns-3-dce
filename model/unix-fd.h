#ifndef UNIX_FD_H
#define UNIX_FD_H

#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include "ns3/object.h"
#include "wait-queue.h"
#include <list>

namespace ns3 {

class Waiter;
class DceManager;

// This class heritate from Object for Dispose and Reference Counting features.
class UnixFd : public Object
{
public:
  static TypeId GetTypeId (void);
  virtual int Close (void) = 0;
  virtual ssize_t Write (const void *buf, size_t count) = 0;
  virtual ssize_t Read (void *buf, size_t count) = 0;
  virtual ssize_t Recvmsg (struct msghdr *msg, int flags) = 0;
  virtual ssize_t Sendmsg (const struct msghdr *msg, int flags) = 0;
  virtual bool Isatty (void) const = 0;
  virtual char * Ttyname (void);
  virtual int Setsockopt (int level, int optname,
                          const void *optval, socklen_t optlen) = 0;
  virtual int Getsockopt (int level, int optname,
                          void *optval, socklen_t *optlen) = 0;
  virtual int Getsockname (struct sockaddr *name, socklen_t *namelen) = 0;
  virtual int Getpeername (struct sockaddr *name, socklen_t *namelen) = 0;
  virtual int Ioctl (unsigned long request, char *argp) = 0;
  virtual int Bind (const struct sockaddr *my_addr, socklen_t addrlen) = 0;
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen) = 0;
  virtual int Listen (int backlog) = 0;
  virtual int Shutdown (int how) = 0;
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen) = 0;
  virtual void * Mmap (void *start, size_t length, int prot, int flags, off64_t offset) = 0;
  virtual off64_t Lseek (off64_t offset, int whence) = 0;
  virtual int Fxstat (int ver, struct ::stat *buf) = 0;
  virtual int Fxstat64 (int ver, struct ::stat64 *buf) = 0;
  virtual int Fcntl (int cmd, unsigned long arg);
  virtual int Settime (int flags,
                       const struct itimerspec *new_value,
                       struct itimerspec *old_value) = 0;
  virtual int Gettime (struct itimerspec *cur_value) const = 0;
  virtual int Ftruncate (off_t length) = 0;
  virtual int GetRealFd (void) const;

  // Return true if a select on this fd should return POLLHUP
  virtual bool HangupReceived (void) const = 0;

  virtual int Poll (PollTable* ptable) = 0;

  void IncFdCount (void);
  void DecFdCount (void);
  int GetFdCount (void) const;

  virtual int Fsync (void) = 0;

  friend class PollTableEntry;
  friend class PollTable;
  friend class DceManager;
protected:
  UnixFd ();
  void RemoveWaitQueue (WaitQueueEntry*, bool andUnregister);
  void AddWaitQueue (WaitQueueEntry*, bool andRegister);
  void WakeWaiters (void *key);
  int m_fdFlags;
  int m_statusFlags;

private:
  std::list <WaitQueueEntry*> m_waitQueueList;
  // Number of FD referencing me
  int m_fdCount;
};

} // namespace ns3

#endif /* UNIX_FD_H */
