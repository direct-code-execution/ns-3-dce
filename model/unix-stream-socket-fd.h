#ifndef UNIX_STREAM_SOCKET_FD_H
#define UNIX_STREAM_SOCKET_FD_H

#include "unix-socket-fd.h"
#include <list>

namespace ns3 {

class UnixStreamSocketFd : public UnixSocketFd
{
public:
  UnixStreamSocketFd (Ptr<Socket> sock);
private:
  virtual ssize_t DoRecvmsg(struct msghdr *msg, int flags);
  virtual ssize_t DoSendmsg(const struct msghdr *msg, int flags);
  virtual int Listen (int backlog);
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen);
  virtual int Shutdown(int how);
  virtual bool CanRecv (void) const;
  virtual bool CanSend (void) const;

  bool ConnectionRequest (Ptr<Socket> sock, const Address & from);
  void ConnectionCreated (Ptr<Socket> sock, const Address & from);

  std::list<std::pair<Ptr<Socket>,Address> > m_connectionQueue;
  int m_backlog;
};

} // namespace ns3

#endif /* UNIX_STREAM_SOCKET_FD_H */
