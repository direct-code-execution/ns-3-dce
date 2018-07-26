#ifndef FREEBSD_SOCKET_FD_FACTORY_H
#define FREEBSD_SOCKET_FD_FACTORY_H

#include "kernel-socket-fd-factory.h"
#include <vector>

extern "C" {
struct KernelHandle;
struct SimDevice;
struct SimSocket;
struct SimTask;
struct SimKernel;
struct SimSysFile;
}

namespace ns3 {

class FreeBSDSocketFdFactory : public KernelSocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  FreeBSDSocketFdFactory ();
  virtual ~FreeBSDSocketFdFactory ();

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

#endif /* FREEBSD_SOCKET_FD_FACTORY_H */
