#include "linux-socket-fd-factory.h"
#include "dce_init.h"
#include "kernel-socket-fd-factory.h"
#include "loader-factory.h"
#include "utils.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/simulator.h"
#include "ns3/event-id.h"
#include "ns3/node.h"


NS_LOG_COMPONENT_DEFINE ("DceLinuxSocketFdFactory");

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (LinuxSocketFdFactory);

TypeId
LinuxSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinuxSocketFdFactory")
    .SetParent<KernelSocketFdFactory> ()
    .AddConstructor<LinuxSocketFdFactory> ()
    .AddAttribute ("Library", "File to load in memory",
                   StringValue ("liblinux.so"),
                   MakeStringAccessor (&KernelSocketFdFactory::m_library),
                   MakeStringChecker ())
  ;
  return tid;
}
LinuxSocketFdFactory::LinuxSocketFdFactory ()
{
}

LinuxSocketFdFactory::~LinuxSocketFdFactory ()
{
}

void
LinuxSocketFdFactory::NotifyNewAggregate (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<Node> node = this->GetObject<Node> ();
  Ptr<LoaderFactory> loaderFactory = this->GetObject<LoaderFactory> ();
  Ptr<TaskManager> taskManager = this->GetObject<TaskManager> ();
  if (node != 0 && loaderFactory != 0 && taskManager != 0 && m_loader == 0)
    {
      m_manager = taskManager;
      m_loader = loaderFactory->Create (0, 0, 0);
      UtilsEnsureDirectoryExists (UtilsGetAbsRealFilePath (node->GetId (), "/var"));
      UtilsEnsureDirectoryExists (UtilsGetAbsRealFilePath (node->GetId (), "/var/log"));
      std::string path = UtilsGetAbsRealFilePath (node->GetId (), "/var/log/messages");
      m_logFile = fopen (path.c_str (), "w");
      setlinebuf (m_logFile);
      // must use ScheduleWithContext to ensure that the initialization task gets
      // a node context to be able to retrieve the task manager when it runs.
      // i.e., TaskManager::Current() needs it.
      Simulator::ScheduleWithContext (node->GetId (), Seconds (0.0),
                                      &LinuxSocketFdFactory::ScheduleTask, this,
                                      MakeEvent (&LinuxSocketFdFactory::InitializeStack, this));
    }
}

void
LinuxSocketFdFactory::SetTask (std::string path, std::string value)
{
  NS_LOG_FUNCTION (path << value);
  m_kernelHandle->dce_lkl_sysctl (path.c_str(), value.c_str());
}

void
LinuxSocketFdFactory::Set (std::string path, std::string value)
{
  if (m_manager == 0)
    {
      m_earlySysfs.push_back (std::make_pair (path,value));
    }
  else
    {
      KernelSocketFdFactory::ScheduleTask (MakeEvent (&LinuxSocketFdFactory::SetTask, this, path, value));
    }
}

std::string
LinuxSocketFdFactory::Get (std::string path)
{
  NS_LOG_FUNCTION (path);
  std::string ret;
  char buffer[512];
  memset (buffer, 0, sizeof(buffer));
  m_kernelHandle->dce_lkl_sysctl_get (path.c_str(), buffer, sizeof(buffer));
  NS_LOG_FUNCTION ("sysctl read: " << buffer);
  ret = std::string (buffer);
  return ret;
}

void
LinuxSocketFdFactory::InitializeStack (void)
{
  KernelSocketFdFactory::InitializeStack ();
  Set (".net.ipv4.conf.all.forwarding", "1");
  Set (".net.ipv4.conf.all.log_martians", "1");
  Set (".net.ipv6.conf.all.forwarding", "0");

  while (!m_earlySysfs.empty ())
    {
      std::pair<std::string,std::string> op = m_earlySysfs.front ();
      Set (op.first, op.second);
      m_earlySysfs.pop_front ();
    }
}

} // namespace ns3
