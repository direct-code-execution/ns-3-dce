#ifndef UNIX_SOCKET_FD_H
#define UNIX_SOCKET_FD_H

#include "unix-fd.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include <linux/types.h>
#include <linux/errqueue.h>
#include <netinet/in.h>
#include <list>

struct sockaddr_ll;

namespace ns3 {

class Socket;
class Thread;
class Waiter;
class Packet;

class UnixSocketFd : public UnixFd
{
public:
  virtual ~UnixSocketFd ();
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
  virtual int Ioctl (int request, char *argp);
  virtual int Bind (const struct sockaddr *my_addr, socklen_t addrlen);
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen);
  virtual void *Mmap (void *start, size_t length, int prot, int flags, off64_t offset);
  virtual off64_t Lseek (off64_t offset, int whence);
  virtual int Fxstat (int ver, struct stat *buf);
  virtual int Fxstat64 (int ver, struct stat64 *buf);
  virtual int Fcntl (int cmd, unsigned long arg);
  virtual int Settime (int flags,
                       const struct itimerspec *new_value,
                       struct itimerspec *old_value);
  virtual int Gettime (struct itimerspec *cur_value) const;
  // CanRecv and CanSend return true only if a following Read or Write will not block,
  // so a remote closed socket should return true because read or write will not block but will fail.
  virtual bool CanRecv (void) const = 0;
  virtual bool CanSend (void) const = 0;

private:
  virtual ssize_t DoRecvmsg (struct msghdr *msg, int flags) = 0;
  virtual ssize_t DoSendmsg (const struct msghdr *msg, int flags) = 0;

protected:
  UnixSocketFd (Ptr<Socket> sock);
  int ErrnoToSimuErrno (void) const;
  Address PosixAddressToNs3Address (const struct sockaddr *my_addr, socklen_t addrlen) const;
  int Ns3AddressToPosixAddress (const Address& nsaddr,
                                struct sockaddr *addr, socklen_t *addrlen) const;
  int Ns3AddressToDeviceIndependantPhysicalLayerAddress (const Address& nsaddr, const Packet& pac,
                                                         struct sockaddr_ll *addr, socklen_t *addrlen) const;
  bool WaitRecvDoSignal (bool blocking);
  Time GetRecvTimeout (void);
  Time GetSendTimeout (void);
  bool IsRecvErr (void) const;
  bool IsRecvTtl (void) const;

  Ptr<Socket> m_socket;
  void ChangeSocket (Ptr<Socket> socket);
  void ClearSocket (void);

private:
  void RecvSocketData (Ptr<Socket> socket);
  void SendSocketData (Ptr<Socket> socket, uint32_t available);
  // Clear m_socket and its callbacks.

  Time m_sendTimeout;
  Time m_recvTimeout;
  int m_recvttl;
  int m_recverr;
  int m_statusFlags;
};

} // namespace ns3

#endif /* UNIX_SOCKET_FD_H */
