#include "sys/dce-timerfd.h"
#include "utils.h"
#include "process.h"
#include "unix-timer-fd.h"
#include "ns3/log.h"
#include <errno.h>
#include "file-usage.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimuTimerFd");

int dce_timerfd_create(int clockid, int flags)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << clockid << flags);
  NS_ASSERT (current != 0);

  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }

  UnixFd *unixFd = new UnixTimerFd (clockid, flags);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);
  return fd;
}

int dce_timerfd_settime(int fd, int flags,
			 const struct itimerspec *new_value,
			 struct itimerspec *old_value)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << flags << new_value << old_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Settime (flags, new_value, old_value);
  FdDecUsage (fd);

  return retval;
}

int dce_timerfd_gettime(int fd, struct itimerspec *cur_value)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << fd << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  if ((0 == current->process->openFiles[fd])||(current->process->openFiles[fd]->IsClosed()))
    {
      current->err = EBADF;
      return -1;
    }
  UnixFd *unixFd =  current->process->openFiles[fd]->GetFileInc ();
  int retval = unixFd->Gettime (cur_value);
  FdDecUsage (fd);

  return retval;
}

