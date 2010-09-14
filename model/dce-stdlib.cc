#include "dce-stdlib.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "ns3/log.h"
#include <errno.h>
#include <limits.h>


NS_LOG_COMPONENT_DEFINE ("SimuStdlib");

using namespace ns3;

long int dce_strtol(const char *nptr, char **endptr, int base)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  long int retval = strtol (nptr, endptr, base);
  if (retval == LONG_MAX || retval == LONG_MIN)
    {
      current->err = errno;
    }
  return retval;
}
long long int dce_strtoll(const char *nptr, char **endptr, int base)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  long long int retval = strtoll (nptr, endptr, base);
  if (retval == LLONG_MAX || retval == LLONG_MIN)
    {
      current->err = errno;
    }
  return retval;
}

unsigned long int dce_strtoul(const char *nptr, char **endptr, int base)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  unsigned long int retval = strtol (nptr, endptr, base);
  if (retval == ULONG_MAX)
    {
      current->err = errno;
    }
  return retval;
}
unsigned long long int dce_strtoull(const char *nptr, char **endptr, int base)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  unsigned long long int retval = strtoull (nptr, endptr, base);
  if (retval == ULLONG_MAX)
    {
      current->err = errno;
    }
  return retval;
}
double dce_strtod(const char *nptr, char **endptr)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr);
  NS_ASSERT (current != 0);
  double retval = strtod (nptr, endptr);
  if (retval == 0.0)
    {
      current->err = errno;
    }
  return retval;
}

int dce_atexit(void (*function)(void))
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << function);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  struct AtExitHandler handler;
  handler.type = AtExitHandler::NORMAL;
  handler.value.normal = function;
  current->process->atExitHandlers.push_back (handler);
  return 0;
}

// XXX: run function to runall atexit functions
