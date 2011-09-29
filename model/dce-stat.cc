#include "sys/dce-stat.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <errno.h>
#include "file-usage.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimuStat");

int dce_xstat (int ver, const char *path, struct stat *buf)
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
int dce_xstat64 (int ver, const char *path, struct stat64 *buf)
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
int dce_fxstat (int ver, int fd, struct stat *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Fxstat (ver, buf);
  FdDecUsage (fd);

  return retval;
}
int dce_fxstat64 (int ver, int fd, struct stat64 *buf)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << fd);
  NS_ASSERT (current != 0);

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd = current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Fxstat64 (ver, buf);
  FdDecUsage (fd);

  return retval;
}
int dce_lxstat (int ver, const char *pathname, struct stat *buf)
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
int dce_lxstat64 (int ver, const char *pathname, struct stat64 *buf)
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
  return dce_fxstat (_STAT_VER, fd, buf);
}
int dce_fstat64 (int fd, struct stat64 *buf)
{
  return dce_fxstat64 (_STAT_VER, fd, buf);
}
