#ifndef LINUX_SOCKET_FD_FACTORY_H
#define LINUX_SOCKET_FD_FACTORY_H

#include "kernel-socket-fd-factory.h"

namespace ns3 {

/**
 * The Linux network stack: the Linux kernel as a library (LKL); see
 * KernelSocketFdFactory. Like the libos stack it replaces, it enables IPv4
 * forwarding by default.
 */
class LinuxSocketFdFactory : public KernelSocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  LinuxSocketFdFactory ();
};

} // namespace ns3

#endif /* LINUX_SOCKET_FD_FACTORY_H */
