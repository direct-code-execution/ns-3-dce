#include "utils.h"
#include "process.h"
#include "loader-factory.h"
#include "task-manager.h"
#include "ns3/log.h"
#include "ns3/breakpoint.h"
#include <list>
#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimuDebug");

static std::list<uint32_t> g_simu_debug_nodes;

void * simu_debug_lookup (char *str)
{
  Thread *current = Current ();
  if (current == 0)
    {
      return 0;
    }
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << str);
  NS_ASSERT (current != 0);
  return current->process->loader->Lookup (current->process->mainHandle, str);
}
uint32_t simu_debug_nodeid (void)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  return UtilsGetNodeId ();
}
const char *simu_debug_processname (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  return current->process->name.c_str ();
}
bool simu_debug_is_node (uint32_t node)
{
  return std::find (g_simu_debug_nodes.begin (), 
		    g_simu_debug_nodes.end (), 
		    node) != g_simu_debug_nodes.end ();
}
static void simu_debug_switch_notify (void)
{
  if (Current () != 0 &&
      simu_debug_is_node (simu_debug_nodeid ()))
    {
      NS_BREAKPOINT ();
    }
}
void simu_debug_monitor (uint32_t node)
{
  if (simu_debug_is_node (node))
    {
      return;
    }
  g_simu_debug_nodes.push_back (node);
  TaskManager::Current ()->SetSwitchNotify (simu_debug_switch_notify);
}
void simu_debug_unmonitor (uint32_t node)
{
  g_simu_debug_nodes.remove (node);
  if (g_simu_debug_nodes.empty ())
    {
      TaskManager::Current ()->SetSwitchNotify (0);
    }
}
