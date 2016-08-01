#include "dce-stdlib.h"
#include "process.h"
#include "dce-manager.h"
#include "dce-stdio.h"
#include "dce-misc.h"
#include "dce-cxa.h"
#include "loader-factory.h"
#include "utils.h"
#include "unix-fd.h"
#include "unix-file-fd.h"
#include "file-usage.h"
#include "ns3/log.h"
#include <errno.h>
#include <limits.h>


NS_LOG_COMPONENT_DEFINE ("DceStdlib");

using namespace ns3;

long int dce_strtol (const char *nptr, char **endptr, int base)
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  long int retval = strtol (nptr, endptr, base);
  if (retval == LONG_MAX || retval == LONG_MIN)
    {
      current->err = errno;
    }
  return retval;
}
long long int dce_strtoll (const char *nptr, char **endptr, int base)
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  long long int retval = strtoll (nptr, endptr, base);
  if (retval == LLONG_MAX || retval == LLONG_MIN)
    {
      current->err = errno;
    }
  return retval;
}

unsigned long int dce_strtoul (const char *nptr, char **endptr, int base)
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  unsigned long int retval = strtol (nptr, endptr, base);
  if (retval == ULONG_MAX)
    {
      current->err = errno;
    }
  return retval;
}
unsigned long long int dce_strtoull (const char *nptr, char **endptr, int base)
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr << base);
  NS_ASSERT (current != 0);
  unsigned long long int retval = strtoull (nptr, endptr, base);
  if (retval == ULLONG_MAX)
    {
      current->err = errno;
    }
  return retval;
}
double dce_strtod (const char *nptr, char **endptr)
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << nptr << endptr);
  NS_ASSERT (current != 0);
  double retval = strtod (nptr, endptr);
  if (retval == 0.0)
    {
      current->err = errno;
    }
  return retval;
}

int dce_atexit (void (*function)(void))
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << function);
  NS_ASSERT (Current () != 0);
  ns3::Thread *current = Current ();
  struct AtExitHandler handler;
  handler.type = AtExitHandler::NORMAL;
  handler.value.normal = function;
  current->process->atExitHandlers.push_back (handler);
  return 0;
}

// XXX: run function to runall atexit functions
/* The last six characters of temp must be the suffix "XXXXXX".
 This suffix is then replaced with a string that makes the filename unique */
int dce_mkstemp (char *temp)
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);

  std::string fullpath = UtilsGetRealFilePath (temp);
  NS_LOG_FUNCTION (fullpath);

  char* c_fullpath = new char[fullpath.length()+1];
  fullpath.copy (c_fullpath, fullpath.length());
  c_fullpath[fullpath.length()] = '\0';

  int realFd = mkstemp (c_fullpath);
  if (realFd == -1)
    {
      current->err = errno;
      delete c_fullpath;
      return -1;
    }

  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      delete c_fullpath;
      return -1;
    }
  UnixFd *unixFd = 0;
  unixFd = new UnixFileFd (realFd);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);

  strncpy (temp, &c_fullpath[strlen(c_fullpath)-strlen(temp)], strlen(temp));
  delete c_fullpath;
  return fd;
}

FILE * dce_tmpfile (void)
{
  char filename[] = "tempXXXXXX";
  int fd = dce_mkstemp (filename);
  return dce_fdopen (fd, "w+");
}

int dce_rename (const char *oldpath, const char *newpath)
{
  ns3::Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);

  std::string oldFullpath = UtilsGetRealFilePath (oldpath);
  std::string newFullpath = UtilsGetRealFilePath (newpath);

  int ret = rename (oldFullpath.c_str (), newFullpath.c_str ());
  if (ret == -1)
    {
      current->err = errno;
      return -1;
    }
  return 0;
}


void dce_exit (int status)
{
  ns3::Thread *current = Current ();
  std::ostringstream oss;
  std::string line;

  NS_LOG_FUNCTION (current << UtilsGetNodeId () << status);
  NS_ASSERT (current != 0);
  CleanupPthreadKeys ();
  dce___cxa_finalize (0);
  dce_fflush (0);
  current->process->timing.exitValue = __W_EXITCODE (status,  WTERMSIG (current->process->timing.exitValue));
  current->process->timing.ns3End = Now ().GetNanoSeconds ();
  current->process->timing.realEnd = time (0);

  current->task->SetSwitchNotifier (0, 0);
  current->process->loader->UnloadAll ();

  oss << "Exit (" << status << ")";
  line = oss.str ();
  DceManager::AppendStatusFile (current->process->pid, current->process->nodeId, line);
  DceManager::AppendProcFile (current->process);

  current->process->manager->DeleteProcess (current->process, DceManager::PEC_EXIT);
  TaskManager::Current ()->Exit ();
}

