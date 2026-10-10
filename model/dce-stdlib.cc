#include "dce-stdlib.h"
#include "process.h"
#include "dce-manager.h"
#include "utils.h"
#include "unix-fd.h"
#include "unix-file-fd.h"
#include "file-usage.h"
#include "ns3/log.h"
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>


NS_LOG_COMPONENT_DEFINE ("DceStdlib");

using namespace ns3;

long int dce_strtol (const char *nptr, char **endptr, int base)
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
long long int dce_strtoll (const char *nptr, char **endptr, int base)
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

unsigned long int dce_strtoul (const char *nptr, char **endptr, int base)
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
unsigned long long int dce_strtoull (const char *nptr, char **endptr, int base)
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
double dce_strtod (const char *nptr, char **endptr)
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

int dce_atexit (void (*function)(void))
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
/* The last six characters of temp must be the suffix "XXXXXX".
 This suffix is then replaced with a string that makes the filename unique */
int dce_mkstemp (char *temp)
{
  Thread *current = Current ();
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
  Thread *current = Current ();
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

// mkstemp64 is what glibc's mkstemp() resolves to when _FILE_OFFSET_BITS=64.
int dce_mkstemp64 (char *temp)
{
  return dce_mkstemp (temp);
}

// The POSIX (XSI) strerror_r, which glibc's strerror_r() resolves to when
// _GNU_SOURCE is not defined: fills buf and returns 0 or an errno value.
int dce___xpg_strerror_r (int errnum, char *buf, size_t buflen)
{
  if (buf == 0 || buflen == 0)
    {
      return ERANGE;
    }
  const char *msg = strerror (errnum);
  size_t len = strlen (msg);
  if (len >= buflen)
    {
      memcpy (buf, msg, buflen - 1);
      buf[buflen - 1] = 0;
      return ERANGE;
    }
  memcpy (buf, msg, len + 1);
  return 0;
}

void * dce_reallocarray (void *ptr, size_t nmemb, size_t size)
{
  if (size != 0 && nmemb > SIZE_MAX / size)
    {
      Current ()->err = ENOMEM;
      return 0;
    }
  return dce_realloc (ptr, nmemb * size);
}

// glibc 2.38 and later resolve strtol() and friends to these C23 variants.
long int dce___isoc23_strtol (const char *nptr, char **endptr, int base)
{
  return dce_strtol (nptr, endptr, base);
}
long long int dce___isoc23_strtoll (const char *nptr, char **endptr, int base)
{
  return dce_strtoll (nptr, endptr, base);
}
long unsigned int dce___isoc23_strtoul (const char *nptr, char **endptr, int base)
{
  return dce_strtoul (nptr, endptr, base);
}
long long unsigned int dce___isoc23_strtoull (const char *nptr, char **endptr, int base)
{
  return dce_strtoull (nptr, endptr, base);
}

// There is no shell to run commands in the simulation.
int dce_system (const char *command)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_WARN ("system(\"" << (command ? command : "") << "\") is not supported");
  current->err = ENOSYS;
  return -1;
}
