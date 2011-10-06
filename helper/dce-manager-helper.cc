#include "dce-manager-helper.h"
#include "dce-manager.h"
#include "socket-fd-factory.h"
#include "local-socket-fd-factory.h"
#include "task-scheduler.h"
#include "task-manager.h"
#include "loader-factory.h"
#include "ns3/random-variable.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/config.h"
#include "dce-node-context.h"
//#include "ns3/attribute-list.h"

namespace ns3 {

UniformVariable g_firstPid;

NS_OBJECT_ENSURE_REGISTERED (DceManagerHelper);

TypeId 
DceManagerHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DceManagerHelper")
    .SetParent<ObjectBase> ()
    .AddAttribute ("LoaderFactory", 
		   "The kind of loader factory created when Install is called", 
		   StringValue ("ns3::CoojaLoaderFactory[]"),
		   MakeObjectFactoryAccessor (&DceManagerHelper::m_loaderFactory),
		   MakeObjectFactoryChecker ())
    ;
  return tid;
}
TypeId 
DceManagerHelper::GetInstanceTypeId (void) const
{
  return DceManagerHelper::GetTypeId ();
}

DceManagerHelper::DceManagerHelper ()
{
  ConstructSelf ( AttributeConstructionList() );
  m_taskManagerFactory.SetTypeId ("ns3::TaskManager");
  m_schedulerFactory.SetTypeId ("ns3::RrTaskScheduler");
  m_managerFactory.SetTypeId ("ns3::DceManager");
  m_networkStackFactory.SetTypeId ("ns3::Ns3SocketFdFactory");
}
void 
DceManagerHelper::SetScheduler (std::string type, 
				    std::string n0, const AttributeValue &v0,
				    std::string n1, const AttributeValue &v1)
{
  m_schedulerFactory.SetTypeId (type);
  m_schedulerFactory.Set (n0, v0);
  m_schedulerFactory.Set (n1, v1);
}
void 
DceManagerHelper::SetTaskManagerAttribute (std::string n0, const AttributeValue &v0)
{
  m_taskManagerFactory.Set (n0, v0);
}
void 
DceManagerHelper::SetLoader (std::string type)
{
  m_loaderFactory.SetTypeId (type);
}
void 
DceManagerHelper::SetNetworkStack (std::string type,
				       std::string n0, const AttributeValue &v0)
{
  m_networkStackFactory.SetTypeId (type);
  m_networkStackFactory.Set (n0, v0);
}
void 
DceManagerHelper::SetAttribute (std::string n1, const AttributeValue &v1)
{
  m_managerFactory.Set (n1, v1);
}
void
DceManagerHelper::Install (NodeContainer nodes)
{
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
    {
      Ptr<DceManager> manager = m_managerFactory.Create<DceManager> ();
      Ptr<TaskManager> taskManager = m_taskManagerFactory.Create<TaskManager> ();
      Ptr<TaskScheduler> scheduler = m_schedulerFactory.Create<TaskScheduler> ();
      Ptr<LoaderFactory> loader = m_loaderFactory.Create<LoaderFactory> ();
      Ptr<SocketFdFactory> networkStack = m_networkStackFactory.Create<SocketFdFactory> ();

      taskManager->SetScheduler (scheduler);
      manager->SetAttribute ("FirstPid", UintegerValue (g_firstPid.GetInteger (0, 0xffff)));
      Ptr<Node> node = *i;
      node->AggregateObject (taskManager);
      node->AggregateObject (loader);
      node->AggregateObject (manager);
      node->AggregateObject (networkStack);
      node->AggregateObject (CreateObject<LocalSocketFdFactory> ());
      manager->AggregateObject(CreateObject<DceNodeContext> () );
    }
}

} // namespace ns3
