#include "dce-time.h"
#include "sys/dce-time.h"
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/simulator.h>
#include <errno.h>
#include "sys/dce-timerfd.h"
#include "unix-timer-fd.h"
#include "file-usage.h"
#include "dce-utime.h"

NS_LOG_COMPONENT_DEFINE ("DceTime");

using namespace ns3;


static void Itimer (Process *process)
{
  if (!process->itimerInterval.IsZero ())
    {
      process->itimer = Simulator::Schedule (process->itimerInterval,
                                             &Itimer, process);
    }
  // wakeup one thread
  UtilsSendSignal (process, SIGALRM);
}

int dce_setitimer (int which, const struct itimerval *value,
                   struct itimerval *ovalue) noexcept
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << which << value << ovalue);
  NS_ASSERT (current != 0);
  if (value == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  // We don't support other kinds of timers.
  NS_ASSERT (which == ITIMER_REAL);
  if (ovalue != 0)
    {
      ovalue->it_interval = UtilsTimeToTimeval (current->process->itimerInterval);
      ovalue->it_value = UtilsTimeToTimeval (Simulator::GetDelayLeft (current->process->itimer));
    }

  current->process->itimer.Cancel ();
  current->process->itimerInterval = UtilsTimevalToTime (value->it_interval);
  if (value->it_value.tv_sec == 0
      && value->it_value.tv_usec == 0)
    {
      return 0;
    }
  TaskManager *manager = TaskManager::Current ();
  current->process->itimer = manager->ScheduleMain (
      UtilsTimevalToTime (value->it_value),
      MakeEvent (&Itimer, current->process));

  return 0;
}

int dce_getitimer (int which, struct itimerval *value) noexcept
{

  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << which << value);
  NS_ASSERT (current != 0);
  if (value == 0)
    {
      current->err = EFAULT;
      return -1;
    }
  // We don't support other kinds of timers.
  NS_ASSERT (which == ITIMER_REAL);
  value->it_interval = UtilsTimeToTimeval (current->process->itimerInterval);
  value->it_value = UtilsTimeToTimeval (Simulator::GetDelayLeft (current->process->itimer));
  return 0;
}


time_t dce_time (time_t *t) noexcept
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

struct tm * dce_gmtime (const time_t *timep) noexcept
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return gmtime_r (timep, &Current ()->process->struct_tm);
}

struct tm * dce_localtime (const time_t *timep)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return localtime_r (timep, &Current ()->process->struct_tm);
}

char * dce_ctime (const time_t *timep) noexcept
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return ctime_r (timep, Current ()->process->asctime_result);
}

char * dce_asctime (const struct tm *tm) noexcept
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  return asctime_r (tm, Current ()->process->asctime_result);
}

int dce_clock_gettime (clockid_t c, struct timespec *tp)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  if (0 == tp)
    {
      Current ()->err = EFAULT;
      return -1;
    }
  *tp = UtilsTimeToTimespec (UtilsSimulationTimeToTime (Now ()));
  return 0;
}
void dce_tzset () noexcept
{

}

int dce_clock_getres (clockid_t c, struct timespec *r)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  switch (c)
    {
    case CLOCK_REALTIME:
      {
        if (0 == r)
          {
            Current ()->err = EFAULT;
            return -1;
          }
        r->tv_sec = 0;
        r->tv_nsec = 1;

        return 0;
      }
      break;

    default:
      {
        Current ()->err = EINVAL;
        return -1;
      }
    }
  return 0;
}

int dce_utime (const char *filename, const struct utimbuf *times) noexcept
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  std::string fullpath = UtilsGetRealFilePath (filename);

  return utime (fullpath.c_str (), times);
}


int dce_timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid) noexcept
{
  int flags = 0;   // just required to set the parameter of class UnixTimerFd::UnixTimerFd(int clockid, int flags)

  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << clockid << flags);
  NS_ASSERT (current != 0);

  int fd = UtilsAllocateFd ();
  memcpy (timerid, &fd, sizeof (timer_t));
  if (fd == -1)
  {
     current->err = EMFILE;
     return -1;
  }

  UnixTimerFd *unixFd = new UnixTimerFd (clockid, flags);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);
  return 0;
}

int dce_timer_settime (timer_t timerid, int flags,
                       const struct itimerspec *new_value,
                       struct itimerspec *old_value) noexcept
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << timerid << flags << new_value << old_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  int fd = * (int *)timerid;
  OPENED_FD_METHOD (int, Settime (flags, new_value, old_value))
}

int dce_timer_gettime (timer_t timerid, struct itimerspec *cur_value) noexcept
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << timerid << cur_value);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  int fd = * (int*)timerid;
  OPENED_FD_METHOD (int, Gettime (cur_value))
}


