
#include "dce-syslog.h"
#include "dce-stdio.h"
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/simulator.h>

#include "dce-manager.h"
#include "process.h"
#include "utils.h"

#include <sstream>

NS_LOG_COMPONENT_DEFINE ("DceSyslog");

using namespace ns3;

void
dce_openlog (const char *ident, int logopt, int facility)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << ident << logopt << facility);
  NS_ASSERT (Current () != 0);
  Process *process = Current ()->process;

  std::ostringstream os; // form the syslog filename
  os << "/var/log/" << process->pid << "/syslog";
  process->syslog = dce_fopen (os.str ().c_str (), "w");
  NS_ASSERT_MSG (process->syslog != 0, "Cannot open " << os.str () << " file to output all syslog messages");
}

void
dce_closelog (void)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Process *process = Current ()->process;

  dce_fclose (process->syslog);
}

int
dce_setlogmask (int maskpri)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << maskpri);
  NS_ASSERT (Current () != 0);
  // ignore
}

void
dce_syslog (int priority, const char *message, ...)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << priority << message);
  NS_ASSERT (Current () != 0);

  va_list ap;
  va_start (ap, message);
  dce_vsyslog (priority, message, ap);
  va_end (ap);
}

void
dce_vsyslog (int priority, const char *message, va_list args)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << priority << message);
  NS_ASSERT (Current () != 0);
  Process *process = Current ()->process;

  vfprintf (process->syslog, message, args);
  fprintf (process->syslog, "\n");
}
