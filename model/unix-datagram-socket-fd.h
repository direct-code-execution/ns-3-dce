#ifndef UNIX_DATAGRAM_SOCKET_FD_H
#define UNIX_DATAGRAM_SOCKET_FD_H

#include "unix-socket-fd.h"
#include "ns3/ipv4-address.h"
#include "ns3/icmpv4.h"
#include <netinet/in.h>

namespace ns3 {

class Address;

class UnixDatagramSocketFd : public UnixSocketFd
{
public:
  UnixDatagramSocketFd (Ptr<Socket> sock);

private:
  virtual ssize_t DoRecvmsg (struct msghdr *msg, int flags);
  virtual ssize_t DoSendmsg (const struct msghdr *msg, int flags);
  virtual int Listen (int backlog);
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen);
  virtual int Shutdown (int how);
  virtual bool CanRecv (void) const;
  virtual bool CanSend (void) const;
  virtual bool HangupReceived (void) const;
  virtual int Poll (PollTable* ptable);
  void IcmpCallback (Ipv4Address icmpSource, uint8_t icmpTtl,
                     uint8_t icmpType, uint8_t icmpCode,
                     uint32_t icmpInfo);
  void QueueErr (sock_extended_err ee, struct sockaddr_in offender, uint8_t ttl);
  void CopyMacAddress (const Address &a,  uint8_t* const buf);
  void MainSendTo (int *r, Ptr<Packet> p, uint32_t f, Address ad);
  void MainSend (int *r, Ptr<Packet> p);

  struct Error
  {
    sock_extended_err ee;
    struct sockaddr_in offender;
    uint8_t ttl;
  };
  std::list<struct Error> m_errQueue;
};

} // namespace ns3

#endif /* UNIX_DATAGRAM_SOCKET_FD_H */
