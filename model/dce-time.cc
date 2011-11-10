
#include "dce-time.h"
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/simulator.h>

NS_LOG_COMPONENT_DEFINE ("DceTime");

using namespace ns3;

time_t dce_time (time_t *t)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  time_t time = (time_t)UtilsSimulationTimeToTime (Now ()).GetSeconds ();
  if (t != 0)
    {
      *t = time;
    }
  return time;
}

struct tm *dce_gmtime (const time_t *timep)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return gmtime_r (timep, &Current ()->process->struct_tm);
}

struct tm *dce_localtime (const time_t *timep)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return localtime_r (timep, &Current ()->process->struct_tm);
}

char *dce_ctime(const time_t *timep)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  
  return ctime_r (timep, Current ()->process->asctime_result);
}

char *dce_asctime(const struct tm *tm)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return asctime_r (tm, Current ()->process->asctime_result);
}
