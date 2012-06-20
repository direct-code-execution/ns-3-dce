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
#include "ns3/log.h"
#include "dce-node-context.h"
#include "ipv4-linux.h"
#include "dce-application-helper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-global-routing.h"

NS_LOG_COMPONENT_DEFINE ("DceManagerHelper");

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
  m_virtualPath ="";
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
      manager->SetVirtualPath (GetVirtualPath());

      TypeId b = TypeId::LookupByName ("ns3::Ns3SocketFdFactory");
      TypeId c = m_networkStackFactory.GetTypeId();

      // Job specific for Linux Stack
      if ( b != c )
        {
          Ptr<Ipv4Linux> stack = node->GetObject<Ipv4Linux> ();

          if (stack)
            {
              uint32_t nbi =  stack->GetNInterfaces ();
              std::ostringstream oss;

              // Add adresses to interfaces.
              for (uint32_t i=0;i < nbi; i++)
                {
                  uint32_t nba = stack->GetNAddresses (i);
                  Ptr<NetDevice> device =  stack->GetNetDevice (i);

                  oss.clear (); oss.str ("");
                  oss << "sim" << i;
                  std::string deviceName = oss.str ();

                  for (int a=0; a < nba; a++)
                    {
                      Ipv4InterfaceAddress ia = stack->GetAddress (i,a);

                      oss.clear (); oss.str ("");
                      ia.GetLocal ().Print (oss);
                      oss << '/' ;
                      ia.GetMask ().Print (oss);
                      std::string addrMask = oss.str ();
                      AddAddress (node,  NanoSeconds (1), deviceName, addrMask);
                      oss.clear (); oss.str ("");
                      oss << "link set " << deviceName << " up arp " <<  ((device->IsPointToPoint())?"off":"on");
                      RunIp (node, NanoSeconds (2), oss.str () );
                    }
                }
              // Install static routes.
              Ptr<Ipv4RoutingProtocol> rp =  stack->GetRoutingProtocol ();
              if (rp)
                {
                  oss.clear (); oss.str ("");
                  Ptr<OutputStreamWrapper> s = Create<OutputStreamWrapper> (&oss);
                  rp->PrintRoutingTable (s);
                  AddRoutes (node, oss.str ());
                }
            }
        }
    }
}
void
DceManagerHelper::SetVirtualPath (std::string p)
{
  m_virtualPath = p;
}
std::string
DceManagerHelper::GetVirtualPath () const
{
  return m_virtualPath;
}
void
DceManagerHelper::RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1<<16);
  process.ResetArguments();
  process.ParseArguments(str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

void
DceManagerHelper::AddAddress (Ptr<Node> node, Time at, std::string name, std::string address)
{
  std::ostringstream oss;
  oss << "-f inet addr add " << address << " dev " << name;
  RunIp (node, at, oss.str ());
}

void
DceManagerHelper::AddRoutes (Ptr<Node> node, std::string r)
{
  char *c = (char*)r.c_str ();
  bool first = true;

  while (*c)
    {
      char *d = c;
      while (*c && (*c!='\n'))
        {
          c++;
        }
      if (c > d)
        {
           std::string ligne (d, c-d);

           if (first)
             {
               first = false;
             }
           else
             {
               AddRoute (node, ligne);
             }
        }
      if (*c)
        {
          c++;
        }
    }
}
void
DceManagerHelper::AddRoute (Ptr<Node> node, std::string r)
{
  char *c = (char*)r.c_str ();
  char *dd = c;
  while (*c && ( (*c != ' ') && (*c != '\t') ) )
    {
      c++;
    }
  if (!(*c)) return;
  std::string desti (dd, c-dd);
  while (*c && ( (*c == ' ') || (*c == '\t') ) )
    {
      c++;
    }
  if (!(*c)) return;
  char *gg = c;
  while (*c && ( (*c != ' ') && (*c != '\t') ) )
    {
      c++;
    }
  if (!(*c)) return;
  std::string gateway (gg, c-gg);
  while (*c && ( (*c == ' ') || (*c == '\t') ) )
    {
      c++;
    }
  if (!(*c)) return;
  char *mm = c;
  while (*c && ( (*c != ' ') && (*c != '\t') ) )
    {
      c++;
    }
  if (!(*c)) return;
  std::string mask (mm, c-mm);
  while (*c && ( (*c == ' ') || (*c == '\t') ) )
    {
      c++;
    }
  if (!(*c)) return;
  char *ff = c;
  while (*c && ( (*c != ' ') && (*c != '\t') ) )
    {
      c++;
    }
  if (!(*c)) return;
  std::string flags (ff, c-ff);

  // Create routes with flag H or G
  if ( (std::string::npos!=flags.find('H')) || (std::string::npos!=flags.find('h'))
      || (std::string::npos!=flags.find('G'))|| (std::string::npos!=flags.find('g')))
    {
      std::ostringstream oss;

      oss << "route add to "<< desti << '/' << mask << " via " << gateway << " metric 1";
      RunIp (node , NanoSeconds (3), oss.str ());
    }
}
} // namespace ns3
