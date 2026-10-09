#ifndef LINUX_SOCKET_FD_FACTORY_H
#define LINUX_SOCKET_FD_FACTORY_H

#include "lkl-socket-fd-factory.h"

namespace ns3 {

/**
 * The Linux network stack: the Linux kernel as a library (LKL); see
 * LklSocketFdFactory.
 */
class LinuxSocketFdFactory : public LklSocketFdFactory
{
public:
  static TypeId GetTypeId (void);
};

} // namespace ns3

#endif /* LINUX_SOCKET_FD_FACTORY_H */
