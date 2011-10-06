#include "dce-application.h"
#include "dce-manager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DceApplication");
NS_OBJECT_ENSURE_REGISTERED (DceApplication);

TypeId 
DceApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DceApplication")
    .SetParent<Application> ()
    .AddConstructor<DceApplication> ()
    .AddTraceSource ("ProcessStarted", "notify when the dce is started",
                     MakeTraceSourceAccessor (&DceApplication::m_dceStarted))
  ;
  return tid;
}

DceApplication::DceApplication ()
  : m_stackSize (0),
    m_pid (0)
{}
DceApplication::~DceApplication ()
{}

void
DceApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}
void 
DceApplication::SetBinary (std::string filename)
{
  m_filename = filename;
}
void 
DceApplication::SetStackSize (uint32_t stackSize)
{
  m_stackSize = stackSize;
}
void 
DceApplication::SetArguments (std::vector<std::string> args)
{
  m_args = args;
}

void 
DceApplication::SetEnvironment (std::vector<std::pair<std::string,std::string> > envs)
{
  m_envs = envs;
}

void 
DceApplication::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<Node> node = GetNode ();
  Ptr<DceManager> manager = node->GetObject<DceManager> ();
  if (manager == 0)
    {
      NS_FATAL_ERROR ("You forgot to aggregate a DceManager to node=" << node->GetId ());
    }
  if (m_stackSize != 0)
    {
      m_pid = manager->Start (m_filename, m_stdinFilename, m_stackSize, m_args, m_envs);
    } 
  else
    {
      m_pid = manager->Start (m_filename, m_stdinFilename, m_args, m_envs);
    }
  if ( !m_finishedCallback.IsNull () )
    {
      manager->SetFinishedCallback (m_pid, m_finishedCallback );
    }
  m_dceStarted (m_pid);
}
void 
DceApplication::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<Node> node = GetNode ();
  Ptr<DceManager> manager = node->GetObject<DceManager> ();
  manager->Stop (m_pid);
}
void
DceApplication::SetStdinFile (std::string filename)
{
  m_stdinFilename = filename;
}
void
DceApplication::SetFinishedCallback (Callback<void,uint16_t,int> cb)
{
  m_finishedCallback = cb;
}
} // namespace ns3
