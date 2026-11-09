/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef HOST_SOCKET_FD_H
#define HOST_SOCKET_FD_H

#include "unix-file-fd.h"
#include "ns3/event-id.h"
#include <string>
#include <sys/socket.h>

namespace ns3 {

/**
 * \brief A connected stream socket of the host, seen by a DCE process as
 * one of its own file descriptors.
 *
 * It lets a DCE application talk to a service of the host over an AF_UNIX
 * socket, typically an X server: when a DCE process connects an AF_UNIX
 * stream socket to a path listed in the DceHostUnixSocketPaths global
 * value (e.g. /tmp/.X11-unix/), the socket is replaced by one of these.
 *
 * The host socket is kept non-blocking: a blocking read or write of the
 * application is emulated by sleeping the DCE task until the host socket is
 * ready, and the readiness of the socket is re-checked every millisecond
 * while a DCE task polls it. Since the host side lives in wall-clock time,
 * this only makes sense with ns3::RealtimeSimulatorImpl.
 */
class HostSocketFd : public UnixFileFd
{
public:
  /**
   * Create a host AF_UNIX stream socket connected to the given address.
   * \return the new fd, or 0 with errno set when the connection failed.
   */
  static HostSocketFd *ConnectHost (const struct sockaddr *addr, socklen_t addrlen);

  virtual ~HostSocketFd ();

  virtual int Close (void);
  virtual ssize_t Write (const void *buf, size_t count);
  virtual ssize_t Read (void *buf, size_t count);
  virtual ssize_t Recvmsg (struct msghdr *msg, int flags);
  virtual ssize_t Sendmsg (const struct msghdr *msg, int flags);
  virtual int Setsockopt (int level, int optname,
                          const void *optval, socklen_t optlen);
  virtual int Getsockopt (int level, int optname,
                          void *optval, socklen_t *optlen);
  virtual int Getsockname (struct sockaddr *name, socklen_t *namelen);
  virtual int Getpeername (struct sockaddr *name, socklen_t *namelen);
  virtual int Shutdown (int how);
  virtual int Ioctl (unsigned long request, char *argp);
  virtual int Fcntl (int cmd, unsigned long arg);
  virtual bool CanRecv (void) const;
  virtual bool CanSend (void) const;
  virtual bool HangupReceived (void) const;
  virtual int Poll (PollTable *ptable);

private:
  HostSocketFd (int realFd);
  // Events currently pending on the host socket (poll(2) with no timeout).
  short HostEvents (void) const;
  // Sleep the current DCE task until one of the events is pending.
  // Returns false if the task was woken up for another reason (signal).
  bool WaitFor (short events);
  // Periodic re-check of the host socket while DCE tasks poll it.
  void CheckReadiness (void);
  int SetErrno (int result);

  EventId m_check;
  bool m_closed;
};

/**
 * \return true if path (an AF_UNIX socket path, without a leading NUL for
 * abstract sockets) matches one of the DceHostUnixSocketPaths prefixes.
 */
bool UtilsIsHostUnixSocketPath (const std::string &path);

} // namespace ns3

#endif /* HOST_SOCKET_FD_H */
