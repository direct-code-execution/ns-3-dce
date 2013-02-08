#include <sys/stat.h>
#include <fcntl.h>
#include "sys/dce-stat.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <errno.h>
#include "file-usage.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimuStat");

int dce___xstat (int ver, const char *path, struct stat *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << path << buf);
  NS_ASSERT (current != 0);
  if (std::string (path) == "")
    {
      current->err = ENOENT;
      return -1;
    }
  int retval = ::__xstat (ver, UtilsGetRealFilePath (path).c_str (), buf);
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }
  return retval;
}
int dce___xstat64 (int ver, const char *path, struct stat64 *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << path << buf);
  NS_ASSERT (current != 0);
  if (std::string (path) == "")
    {
      current->err = ENOENT;
      return -1;
    }
  int retval = ::__xstat64 (ver, UtilsGetRealFilePath (path).c_str (), buf);
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }
  return retval;
}
int dce___fxstat (int ver, int fd, struct stat *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Fxstat (ver, buf))
}
int dce___fxstat64 (int ver, int fd, struct stat64 *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  NS_ASSERT (current != 0);

  OPENED_FD_METHOD (int, Fxstat64 (ver, buf))
}
int dce___lxstat (int ver, const char *pathname, struct stat *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << pathname << buf);
  NS_ASSERT (current != 0);
  if (std::string (pathname) == "")
    {
      current->err = ENOENT;
      return -1;
    }
  int retval = ::__lxstat (ver, UtilsGetRealFilePath (pathname).c_str (), buf);
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }
  return retval;
}
int dce___lxstat64 (int ver, const char *pathname, struct stat64 *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << pathname << buf);
  NS_ASSERT (current != 0);
  if (std::string (pathname) == "")
    {
      current->err = ENOENT;
      return -1;
    }
  int retval = ::__lxstat64 (ver, UtilsGetRealFilePath (pathname).c_str (), buf);
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }
  return retval;
}

int dce_fstat (int fd, struct stat *buf)
{
  return dce___fxstat (_STAT_VER, fd, buf);
}
int dce_fstat64 (int fd, struct stat64 *buf)
{
  return dce___fxstat64 (_STAT_VER, fd, buf);
}
