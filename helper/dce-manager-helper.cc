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
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("DceManagerHelper");

namespace ns3 {

UniformVariable g_firstPid;

NS_OBJECT_ENSURE_REGISTERED (DceManagerHelper);

unsigned long DceManagerHelper::nanoCpt = 0;

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
  ConstructSelf (AttributeConstructionList ());
  m_taskManagerFactory.SetTypeId ("ns3::TaskManager");
  m_schedulerFactory.SetTypeId ("ns3::RrTaskScheduler");
  m_managerFactory.SetTypeId ("ns3::DceManager");
  m_networkStackFactory.SetTypeId ("ns3::Ns3SocketFdFactory");
  m_delayFactory.SetTypeId ("ns3::RandomProcessDelayModel");
  m_virtualPath = "";
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
DceManagerHelper::SetDelayModel (std::string type,
                                 std::string n0, const AttributeValue &v0,
                                 std::string n1, const AttributeValue &v1)
{
  m_delayFactory.SetTypeId (type);
  m_delayFactory.Set (n0, v0);
  m_delayFactory.Set (n1, v1);
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
      Ptr<ProcessDelayModel> delay = m_delayFactory.Create<ProcessDelayModel> ();

      taskManager->SetScheduler (scheduler);
      taskManager->SetDelayModel (delay);
      manager->SetAttribute ("FirstPid", UintegerValue (g_firstPid.GetInteger (0, 0xffff)));
      Ptr<Node> node = *i;
      node->AggregateObject (taskManager);
      node->AggregateObject (loader);
      node->AggregateObject (manager);
      node->AggregateObject (networkStack);
      node->AggregateObject (CreateObject<LocalSocketFdFactory> ());
      manager->AggregateObject (CreateObject<DceNodeContext> ());
      manager->SetVirtualPath (GetVirtualPath ());

      TypeId b = TypeId::LookupByName ("ns3::Ns3SocketFdFactory");
      TypeId c = m_networkStackFactory.GetTypeId ();

      // Job specific for Linux Stack
      if (b != c)
        {
          Ptr<Ipv4Linux> stack = node->GetObject<Ipv4Linux> ();

          if (stack)
            {
              uint32_t nbi =  stack->GetNInterfaces ();
              std::ostringstream oss;

              // Add adresses to interfaces.
              for (uint32_t i = 0; i < nbi; i++)
                {
                  uint32_t nba = stack->GetNAddresses (i);
                  Ptr<NetDevice> device =  stack->GetNetDevice (i);

                  oss.clear ();
                  oss.str ("");
                  oss << "sim" << i;
                  std::string deviceName = oss.str ();

                  for (uint32_t a = 0; a < nba; a++)
                    {
                      Ipv4InterfaceAddress ia = stack->GetAddress (i,a);

                      oss.clear ();
                      oss.str ("");
                      ia.GetLocal ().Print (oss);
                      oss << '/' ;
                      ia.GetMask ().Print (oss);
                      std::string addrMask = oss.str ();
                      AddAddress (node,  NanoSeconds (++nanoCpt), deviceName, addrMask);
                      oss.clear ();
                      oss.str ("");
                      oss << "link set " << deviceName << " up arp " <<  ((device->IsPointToPoint ()) ? "off" : "on");
                      RunIp (node, NanoSeconds (++nanoCpt), oss.str ());
                    }
                }
              // Install static routes.
              Ptr<Ipv4RoutingProtocol> rp =  stack->GetRoutingProtocol ();
              if (rp)
                {
                  oss.clear ();
                  oss.str ("");
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
  process.SetStackSize (1 << 16);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
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
      while (*c && (*c != '\n'))
        {
          c++;
        }
      if (c > d)
        {
          std::string ligne (d, c - d);

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
  while (*c && ((*c != ' ') && (*c != '\t')))
    {
      c++;
    }
  if (!(*c))
    {
      return;
    }
  std::string desti (dd, c - dd);
  while (*c && ((*c == ' ') || (*c == '\t')))
    {
      c++;
    }
  if (!(*c))
    {
      return;
    }
  char *gg = c;
  while (*c && ((*c != ' ') && (*c != '\t')))
    {
      c++;
    }
  if (!(*c))
    {
      return;
    }
  std::string gateway (gg, c - gg);
  while (*c && ((*c == ' ') || (*c == '\t')))
    {
      c++;
    }
  if (!(*c))
    {
      return;
    }
  char *mm = c;
  while (*c && ((*c != ' ') && (*c != '\t')))
    {
      c++;
    }
  if (!(*c))
    {
      return;
    }
  std::string mask (mm, c - mm);
  while (*c && ((*c == ' ') || (*c == '\t')))
    {
      c++;
    }
  if (!(*c))
    {
      return;
    }
  char *ff = c;
  while (*c && ((*c != ' ') && (*c != '\t')))
    {
      c++;
    }
  if (!(*c))
    {
      return;
    }
  std::string flags (ff, c - ff);

  // Create routes with flag H or G
  if ((std::string::npos != flags.find ('H')) || (std::string::npos != flags.find ('h'))
      || (std::string::npos != flags.find ('G'))|| (std::string::npos != flags.find ('g')))
    {
      std::ostringstream oss;

      oss << "route add to " << desti << '/' << mask << " via " << gateway << " metric 1";
      RunIp (node, NanoSeconds (++nanoCpt), oss.str ());
    }
}

std::vector<ProcStatus>
DceManagerHelper::GetProcStatus (void)
{
  FILE *f = fopen ("exitprocs","r");
  std::vector<ProcStatus> res;

  if (f)
    {
      char buffer[10 * 1024];

      while ((!feof (f)) && (fgets (buffer, sizeof(buffer),f)))
        {
          if (0 == strncmp (buffer, "NODE",4))
            {
              // SKIP First line
            }
          else
            {
              long int ret = 0;
              char *crsr = buffer;
              char *next = 0;

              errno = 0;
              ret = strtol (crsr, &next, 10);
              if ((ret == LONG_MIN) || (ret == LONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              int node = (int) ret;
              crsr = next;
              next = 0;

              errno = 0;
              ret = strtol (crsr, &next, 10);
              if ((ret == LONG_MIN) || (ret == LONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              int exitcode = (int) ret;
              crsr = next;
              next = 0;

              errno = 0;
              ret = strtol (crsr, &next, 10);
              if ((ret == LONG_MIN) || (ret == LONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              int pid = (int) ret;
              crsr = next;
              next = 0;

              unsigned long long int ret2 = 0;
              errno = 0;
              ret2 = strtoll (crsr, &next, 10);
              if ((ret == LLONG_MIN) || (ret == LLONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              int64_t nst = (int64_t) ret2;
              crsr = next;
              next = 0;

              errno = 0;
              ret2 = strtoll (crsr, &next, 10);
              if ((ret == LLONG_MIN) || (ret == LLONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              int64_t ned = (int64_t) ret2;
              crsr = next;
              next = 0;

              errno = 0;
              ret = strtol (crsr, &next, 10);
              if ((ret == LONG_MIN) || (ret == LONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              long rst = (long) ret;
              crsr = next;
              next = 0;

              errno = 0;
              ret = strtol (crsr, &next, 10);
              if ((ret == LONG_MIN) || (ret == LONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              long red = (long) ret;
              crsr = next;
              next = 0;

              double ret3;
              errno = 0;
              ret3 = strtod (crsr, &next);
              if ((ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              double dur3 = ret3;
              crsr = next;
              next = 0;

              errno = 0;
              ret = strtol (crsr, &next, 10);
              if ((ret == LONG_MIN) || (ret == LONG_MAX) || (ERANGE == errno) || (next == crsr))
                {
                  continue;
                }
              long durr = (long) ret;
              crsr = next;
              next = 0;

              ProcStatus st (node, exitcode, pid, nst, ned, rst, red, dur3, durr, crsr + 1);

              res.push_back (st);
            }
        }

      fclose (f);
    }

  return res;
}

void
DceManagerHelper::AddRoute (Ptr<Node> node, std::string dest, std::string mask, std::string gateway, int metric)
{
  std::ostringstream oss;

  oss << "route add to " << dest << '/' << mask << " via " << gateway << " metric " << metric;
  RunIp (node, NanoSeconds (++nanoCpt), oss.str ());
}

ProcStatus::ProcStatus (int n, int e, int p, int64_t ns, int64_t ne, long rs, long re, double nd, long rd, std::string cmd)
  : m_node (n),
    m_exitCode (e),
    m_pid (p),
    m_ns3StartTime (ns),
    m_ns3EndTime (ne),
    m_realStartTime (rs),
    m_realEndTime (re),
    m_ns3Duration (nd),
    m_realDuration (rd),
    m_cmdLine (cmd)
{
}

int
ProcStatus::GetNode (void) const
{
  return m_node;
}

int
ProcStatus::GetExitCode (void) const
{
  return m_exitCode;
}

int
ProcStatus::GetPid (void) const
{
  return m_pid;
}

int64_t
ProcStatus::GetSimulatedStartTime (void) const
{
  return m_ns3StartTime;
}

int64_t ProcStatus::GetSimulatedEndTime (void) const
{
  return m_ns3EndTime;
}

long
ProcStatus::GetRealStartTime (void) const
{
  return m_realStartTime;
}

/* Real End Time  */
long
ProcStatus::GetRealEndTime (void) const
{
  return m_realEndTime;
}

double
ProcStatus::GetSimulatedDuration (void) const
{
  return m_ns3Duration;
}

long
ProcStatus::GetRealDuration (void) const
{
  return m_realDuration;
}

std::string
ProcStatus::GetCmdLine (void) const
{
  return m_cmdLine;
}

} // namespace ns3
