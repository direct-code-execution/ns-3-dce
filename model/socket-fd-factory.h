#ifndef SOCKET_FD_FACTORY_H
#define SOCKET_FD_FACTORY_H

#include "ns3/ptr.h"
#include "ns3/object.h"

namespace ns3 {

class UnixFd;
class Object;

class SocketFdFactory : public Object
{
public:
  static TypeId GetTypeId (void);
  virtual ~SocketFdFactory ();
  virtual UnixFd * CreateSocket (int domain, int type, int protocol) = 0;
  virtual void UnlinkNotify (std::string path);
};

} // namespace ns3

#endif /* SOCKET_FD_FACTORY_H */
