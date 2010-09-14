#include "dce-manager-helper.h"
#include "ns3/dce-manager.h"
#include "ns3/socket-fd-factory.h"
#include "ns3/task-scheduler.h"
#include "ns3/task-manager.h"
#include "ns3/loader-factory.h"
#include "ns3/random-variable.h"
#include "ns3/uinteger.h"

namespace ns3 {

UniformVariable g_firstPid;

DceManagerHelper::DceManagerHelper ()
{
  m_loaderFactory.SetTypeId ("ns3::DlmLoaderFactory");
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
    }
}

} // namespace ns3
