#include "dce-vfs.h"
#include "utils.h"
#include "dce-manager.h"
#include "process.h"
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/simulator.h>
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("DceVfs");

using namespace ns3;

int dce_statfs (const char *path, struct statfs *buf)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Current ()->err = ENOSYS;

  return -1;
}

int dce_fstatfs (int fd, struct statfs *buf)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Current ()->err = ENOSYS;

  return -1;
}

int dce_statfs64 (const char *path, struct statfs *buf)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Current ()->err = ENOSYS;

  return -1;
}

int dce_fstatfs64 (int fd, struct statfs *buf)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Current ()->err = ENOSYS;

  return -1;
}

int dce_statvfs (const char *path, struct statvfs *buf)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Current ()->err = ENOSYS;

  return -1;
}

int dce_fstatvfs (int fd, struct statvfs *buf)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  Current ()->err = ENOSYS;

  return -1;
}
