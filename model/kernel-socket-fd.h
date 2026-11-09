/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef KERNEL_SOCKET_FD_H
#define KERNEL_SOCKET_FD_H

#include "unix-fd.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"

namespace ns3 {

class KernelSocketFdFactory;

/**
 * A socket of a node's LKL kernel: every operation is the corresponding
 * kernel system call on the kernel's file descriptor.
 */
class KernelSocketFd : public UnixFd
{
public:
  KernelSocketFd (Ptr<KernelSocketFdFactory> factory, int fd);
  virtual ~KernelSocketFd ();

  int GetKernelFd (void) const;
  /** Wake the tasks waiting in poll or select for these events. */
  void NotifyEvents (short events);

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
  virtual bool HangupReceived (void) const;
  virtual int Poll (PollTable* ptable);
  virtual int Fsync (void);

private:
  // Kernel system call; on error, sets the current thread's errno.
  long Call (long no, long a0 = 0, long a1 = 0, long a2 = 0,
             long a3 = 0, long a4 = 0, long a5 = 0) const;
  // Kernel system call; returns -errno on error.
  long RawCall (long no, long a0 = 0, long a1 = 0, long a2 = 0,
                long a3 = 0, long a4 = 0, long a5 = 0) const;
  // A system call that may block, made without blocking in the kernel;
  // waits in DCE for the events instead. Returns -errno on error.
  long BlockingCall (short events, int timeoutOption, long no, long a0 = 0, long a1 = 0,
                     long a2 = 0, long a3 = 0, long a4 = 0, long a5 = 0);
  // Waits in DCE for one of the events or the timeout.
  int WaitEvents (short events, Time timeout);
  Time GetTimeout (int option) const;
  // Sets errno from a RawCall result.
  long Result (long ret) const;

  Ptr<KernelSocketFdFactory> m_factory;
  int m_fd;
};

} // namespace ns3

#endif /* KERNEL_SOCKET_FD_H */
