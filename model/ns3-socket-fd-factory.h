#ifndef NS3_SOCKET_FD_FACTORY_H
#define NS3_SOCKET_FD_FACTORY_H

#include "socket-fd-factory.h"

namespace ns3 {

class SocketFactory;

class Ns3SocketFdFactory : public SocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  Ns3SocketFdFactory ();
  void NotifyNewAggregate (void);

  virtual UnixFd * CreateSocket (int domain, int type, int protocol);
private:
  Ptr<SocketFactory> m_netlink;
};

} // namespace ns3

#endif /* NS3_SOCKET_FD_FACTORY_H */
