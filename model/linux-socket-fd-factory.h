#ifndef LINUX_SOCKET_FD_FACTORY_H
#define LINUX_SOCKET_FD_FACTORY_H

#ifdef LKL_LINUX

#include "lkl-socket-fd-factory.h"

namespace ns3 {

/**
 * Without libos (configure --with-lkl only), the Linux network stack is
 * LKL; see LklSocketFdFactory.
 */
class LinuxSocketFdFactory : public LklSocketFdFactory
{
public:
  static TypeId GetTypeId (void);
};

} // namespace ns3

#else

#include "kernel-socket-fd-factory.h"
#include <vector>

extern "C" {
struct SimExported;
struct SimDevice;
struct SimSocket;
struct SimTask;
struct SimKernel;
struct SimSysFile;
}

namespace ns3 {

class LinuxSocketFdFactory : public KernelSocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  LinuxSocketFdFactory ();
  virtual ~LinuxSocketFdFactory ();

  void Set (std::string path, std::string value);
  std::string Get (std::string path);

private:
  virtual void NotifyNewAggregate (void);
  void InitializeStack (void);
  std::vector<std::pair<std::string,struct SimSysFile *> > GetSysFileList (void);
  void SetTask (std::string path, std::string value);

  std::list<std::pair<std::string,std::string> > m_earlySysfs;
};

} // namespace ns3

#endif /* LKL_LINUX */

#endif /* LINUX_SOCKET_FD_FACTORY_H */
