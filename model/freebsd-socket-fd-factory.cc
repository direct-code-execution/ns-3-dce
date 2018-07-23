#include "freebsd-socket-fd-factory.h"
#include "kernel-socket-fd-factory.h"
#include "loader-factory.h"
#include "kernel-socket-fd.h"
#include "utils.h"
#include "dce_init.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/simulator.h"
#include "ns3/node.h"

NS_LOG_COMPONENT_DEFINE ("DceFreeBSDSocketFdFactory");

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (FreeBSDSocketFdFactory);

TypeId
FreeBSDSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FreeBSDSocketFdFactory")
    .SetParent<KernelSocketFdFactory> ()
    .AddConstructor<FreeBSDSocketFdFactory> ()
    .AddAttribute ("Library", "File to load in memory",
                   StringValue ("libfreebsd.so"),
                   MakeStringAccessor (&FreeBSDSocketFdFactory::m_library),
                   MakeStringChecker ())
  ;
  return tid;
}
FreeBSDSocketFdFactory::FreeBSDSocketFdFactory ()
{
}

FreeBSDSocketFdFactory::~FreeBSDSocketFdFactory ()
{
}

void
FreeBSDSocketFdFactory::NotifyNewAggregate (void)
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
                                      &FreeBSDSocketFdFactory::ScheduleTask, this,
                                      MakeEvent (&FreeBSDSocketFdFactory::InitializeStack, this));
    }
}

std::vector<std::pair<std::string,struct SimSysFile *> >
FreeBSDSocketFdFactory::GetSysFileList (void)
{
  struct MyIterator
  {
    struct SimSysIterator head;
    static void ReportStartDir (const struct SimSysIterator *iter, const char *dirname)
    {
      struct MyIterator *self = (struct MyIterator *)iter;
      self->m_stack.push_back (self->m_currentPath);
      self->m_currentPath += "." + std::string (dirname);
    }
    static void ReportEndDir (const struct SimSysIterator *iter)
    {
      struct MyIterator *self = (struct MyIterator *)iter;
      self->m_currentPath = self->m_stack.back ();
      self->m_stack.pop_back ();
    }
    static void ReportFile (const struct SimSysIterator *iter, const char *filename, int flags, struct SimSysFile *file)
    {
      struct MyIterator *self = (struct MyIterator *)iter;
      std::string path = self->m_currentPath + "." + filename;
      self->m_list.push_back (std::make_pair (path, file));
    }
    std::vector<std::string> m_stack;
    std::vector<std::pair<std::string,struct SimSysFile *> > m_list;
    std::string m_currentPath;
  } iter;
  iter.head.report_start_dir = &MyIterator::ReportStartDir;
  iter.head.report_end_dir = &MyIterator::ReportEndDir;
  iter.head.report_file = &MyIterator::ReportFile;
#ifdef FIXME
  m_loader->NotifyStartExecute ();
  m_kernelHandle->sys_iterate_files ((struct SimSysIterator *)&iter);
  m_loader->NotifyEndExecute ();
#endif
  return iter.m_list;
}

void
FreeBSDSocketFdFactory::SetTask (std::string path, std::string value)
{
  NS_LOG_FUNCTION (path << value);
  std::vector<std::pair<std::string,struct SimSysFile *> > files = GetSysFileList ();
  for (uint32_t i = 0; i < files.size (); i++)
    {
      if (files[i].first == path)
        {
          const char *s = value.c_str ();
          int toWrite = value.size ();
          int written;
          written = m_kernelHandle->sys_file_write (files[i].second, s, toWrite, 0);
          break;
        }
    }
}

void
FreeBSDSocketFdFactory::Set (std::string path, std::string value)
{
  if (m_manager == 0)
    {
      m_earlySysfs.push_back (std::make_pair (path,value));
    }
  else
    {
      ScheduleTask (MakeEvent (&FreeBSDSocketFdFactory::SetTask, this, path, value));
    }
}

std::string
FreeBSDSocketFdFactory::Get (std::string path)
{
  NS_LOG_FUNCTION (path);
  std::string ret;
  std::vector<std::pair<std::string,struct SimSysFile *> > files = GetSysFileList ();
  for (uint32_t i = 0; i < files.size (); i++)
    {
      if (files[i].first == path)
        {
          char buffer[512];
          memset (buffer, 0, sizeof(buffer));
          m_kernelHandle->sys_file_read (files[i].second, buffer, sizeof(buffer), 0);
          NS_LOG_FUNCTION ("sysctl read: " << buffer);
          ret = std::string (buffer);
          break;
        }
    }
  return ret;
}


void
FreeBSDSocketFdFactory::InitializeStack (void)
{
  KernelSocketFdFactory::InitializeStack ();

#ifdef NOT_YET_SUPPORTED
  Set (".net.ipv4.conf.all.forwarding", "1");
  Set (".net.ipv4.conf.all.log_martians", "1");
  Set (".net.ipv6.conf.all.forwarding", "0");

  while (!m_earlySysfs.empty ())
    {
      std::pair<std::string,std::string> op = m_earlySysfs.front ();
      Set (op.first, op.second);
      m_earlySysfs.pop_front ();
    }
#endif
}

} // namespace ns3
