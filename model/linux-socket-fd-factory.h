#ifndef LINUX_SOCKET_FD_FACTORY_H
#define LINUX_SOCKET_FD_FACTORY_H

#include "lkl-socket-fd-factory.h"

namespace ns3 {

/**
 * The Linux network stack: the Linux kernel as a library (LKL); see
 * LklSocketFdFactory. Like the libos stack it replaces, it enables IPv4
 * forwarding by default.
 */
class LinuxSocketFdFactory : public LklSocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  LinuxSocketFdFactory ();
};

} // namespace ns3

#endif /* LINUX_SOCKET_FD_FACTORY_H */
