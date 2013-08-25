#include "dce-unistd.h"

#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <errno.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceCredentials");



gid_t dce_getgid (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current);
  NS_ASSERT (current != 0);

  return current->process->rgid;
}

gid_t dce_getegid (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current);
  NS_ASSERT (current != 0);

  return current->process->egid;
}

pid_t dce_getpgrp (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current);
  NS_ASSERT (current != 0);

  return current->process->pgid;
}

int dce_euidaccess (const char *pathname, int mode)
{
  NS_LOG_FUNCTION (pathname << mode);
  std::string rPath = UtilsGetRealFilePath (pathname);
  int ret = euidaccess (rPath.c_str (), mode);
  return ret;
}
int dce_eaccess (const char *pathname, int mode)
{
  NS_LOG_FUNCTION (pathname << mode);
  std::string rPath = UtilsGetRealFilePath (pathname);
  int ret = eaccess (rPath.c_str (), mode);
  return ret;
}
int dce_chown(const char *path, uid_t owner, gid_t group)
{
  // XXX
  return 0;
}

int dce_initgroups(const char *user, gid_t group)
{
  // XXX
  return 0;
}

