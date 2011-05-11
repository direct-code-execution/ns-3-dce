#ifndef UNIX_STREAM_SOCKET_FD_H
#define UNIX_STREAM_SOCKET_FD_H

#include "unix-socket-fd.h"
#include <list>

namespace ns3 {

class UnixStreamSocketFd : public UnixSocketFd
{
public:
  UnixStreamSocketFd (Ptr<Socket> sock);
  virtual ~UnixStreamSocketFd (void);
private:
  virtual ssize_t DoRecvmsg(struct msghdr *msg, int flags);
  virtual ssize_t DoSendmsg(const struct msghdr *msg, int flags);
  virtual int Listen (int backlog);
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen);
  virtual int Shutdown(int how);
  virtual bool CanRecv (void) const;
  virtual bool CanSend (void) const;
  virtual bool HangupReceived (void) const;
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen);

  bool ConnectionRequest (Ptr<Socket> sock, const Address & from);
  void ConnectionCreated (Ptr<Socket> sock, const Address & from);

  void ConnectionSuccess (Ptr<Socket> sock);
  void ConnectionError (Ptr<Socket> sock);

  void CloseSuccess (Ptr<Socket> sock);
  void CloseError (Ptr<Socket> sock);

  std::list<std::pair<Ptr<Socket>,Address> > m_connectionQueue;
  int m_backlog;
  bool m_connected;

  bool m_connecting;

};

} // namespace ns3

#endif /* UNIX_STREAM_SOCKET_FD_H */
