#ifndef KERNEL_SOCKET_FD_H
#define KERNEL_SOCKET_FD_H

#include "unix-fd.h"
#include "ns3/ptr.h"

extern "C" {
struct DceSocket;
}

namespace ns3 {

class KernelSocketFdFactory;

class Waiter;

class KernelSocketFd : public UnixFd
{
public:
  KernelSocketFd (Ptr<KernelSocketFdFactory> factory, struct DceSocket *socket);
  virtual ~KernelSocketFd ();

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

  virtual bool HangupReceived (void) const;

  virtual int Poll (PollTable* ptable);
  virtual int Ftruncate (off_t length);
  virtual int Fsync (void);

  void PollEvent (int flag);


private:
  Ptr<KernelSocketFdFactory> m_factory;
  struct DceSocket *m_socket;
  int m_statusFlags;
};

} // namespace ns3

#endif /* KERNEL_SOCKET_FD_H */
