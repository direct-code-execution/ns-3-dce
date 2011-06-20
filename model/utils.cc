#include "utils.h"
#include "dce-manager.h"
#include "unix-fd.h"
#include "process.h"
#include "task-manager.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("ProcessUtils");

namespace ns3 {

uint32_t UtilsGetNodeId (void)
{
  return Simulator::GetContext ();
}
static std::string UtilsGetRealFilePath (uint32_t node)
{
  std::ostringstream oss;
  oss << "files-" << node;
  return oss.str ();
}
static std::string UtilsGetRealFilePath (void)
{
  return UtilsGetRealFilePath (UtilsGetNodeId ());
}

std::string UtilsGetAbsRealFilePath (uint32_t node, std::string path)
{
  NS_LOG_FUNCTION (Current () << path);

  std::string nodeDir = UtilsGetRealFilePath (node);
  UtilsEnsureDirectoryExists (nodeDir);
  return nodeDir + path;  
}

std::string 
UtilsGetRealFilePath (std::string path)
{
  NS_LOG_FUNCTION (Current () << path);

  std::string nodeDir = UtilsGetRealFilePath ();
  UtilsEnsureDirectoryExists (nodeDir);
  return nodeDir + UtilsGetVirtualFilePath (path);
}

void
UtilsEnsureAllDirectoriesExist (std::string realPath)
{
  int idx = 0;
  
  while ((idx = realPath.find ('/', idx)) >= 0)
    {
      UtilsEnsureDirectoryExists (realPath.substr (0, idx + 1));
      idx++;
    }
}
  
void UtilsEnsureDirectoryExists (std::string realPath)
{
  ::DIR *dir = ::opendir (realPath.c_str ());
  if (dir != 0)
    {
      ::closedir (dir);
    }
  else if (errno == ENOENT)
    {
      int status = ::mkdir (realPath.c_str (), S_IRWXU | S_IRWXG);
      if (status == -1)
	{
	  NS_FATAL_ERROR ("Could not create directory " << realPath <<
			  ": " << strerror (errno));
	}
    }
}

std::string UtilsGetVirtualFilePath (std::string path)
{
  NS_LOG_FUNCTION (Current () << path);

  std::string::size_type slash = path.find ("/");
  if (slash == 0)
    {
      // path relative to root of node.
      return path;
    }
  else
    {
      NS_ASSERT (Current () != 0);
      Thread *current = Current ();
      // path relative to cwd in node
      return current->process->cwd + "/" + path;
    }
}

int
UtilsSearchOpenFd (int fd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << fd);
  NS_ASSERT (current != 0);
  int index = 0;
  for (std::vector<std::pair<int,UnixFd *> >::iterator i = current->process->openFiles.begin (); 
       i != current->process->openFiles.end (); ++i)
    {
      if (i->first == fd)
	{
          return index;
        }
      index++;
    }
  return -1;
}

Thread *gDisposingThreadContext = 0;

Thread *Current (void)
{
  if ( 0 != gDisposingThreadContext ) return gDisposingThreadContext;

  TaskManager *manager = TaskManager::Current ();
  if (manager == 0)
    {
      return 0;
    }
  return (struct Thread *)manager->CurrentTask ()->GetContext ();
}
bool HasPendingSignal (void)
{
  bool isPending = sigisemptyset (&(Current ()->process->pendingSignals)) == 0;
  return isPending;
}
struct timeval UtilsTimeToTimeval (Time time)
{
  struct timeval tv;
  tv.tv_sec = (time_t) time.GetSeconds ();
  tv.tv_usec = time.GetMicroSeconds () % 1000000;
  return tv;
}
struct timespec UtilsTimeToTimespec (Time time)
{
  struct timespec tv;
  tv.tv_sec = (time_t) time.GetSeconds ();
  tv.tv_nsec = time.GetNanoSeconds () % 1000000000;
  return tv;
}
static unsigned long TimeBase (void)
{
  unsigned long secondsSinceEpochOnFridayApril042008 = 1207284276;
  return secondsSinceEpochOnFridayApril042008;

}
Time UtilsSimulationTimeToTime (Time time)
{
  return time + Seconds (TimeBase ());
}
Time UtilsTimeToSimulationTime (Time time)
{
  return time - Seconds (TimeBase ());
}
Time UtilsTimevalToTime (struct timeval tv)
{
  Time time = Seconds (tv.tv_sec);
  time += MicroSeconds (tv.tv_usec);
  return time;
}
Time UtilsTimevalToTime (const struct timeval *tv)
{
  if (tv == 0)
    {
      return Seconds (0.0);
    }
  return UtilsTimevalToTime (*tv);
}
Time UtilsTimespecToTime (struct timespec tm)
{
  Time time = Seconds (tm.tv_sec);
  time += NanoSeconds (tm.tv_nsec);
  return time;
}
void
UtilsSendSignal (Process *process, int signum)
{
  sigaddset (&process->pendingSignals, signum);
  for (std::vector<Thread *>::iterator i = process->threads.begin (); 
       i != process->threads.end (); ++i)
    {
      Thread *thread = *i;
      if (sigismember (&thread->signalMask, signum) == 0)
	{
	  // signal not blocked by thread.
	  if (thread->task->IsBlocked ())
	    {
	      process->manager->Wakeup (thread);
	      return;
	    }
	}
    }
  // Could not find any candidate thread to receive signal.
  // signal pending until a thread unblocks it.
}
void UtilsDoSignal (void)
{
  Thread *current = Current ();
  // we try to check if we 
  // have pending signals and we deliver them if we have any.
  for (std::vector<SignalHandler>::iterator i = current->process->signalHandlers.begin (); 
       i != current->process->signalHandlers.end (); ++i)
    {
      if (sigismember (&current->signalMask, i->signal) == 1 &&
	  i->signal != SIGKILL &&
	  i->signal != SIGSTOP)
	{
	  // don't deliver signals which are masked
	  // ignore the signal mask for SIGKILL and SIGSTOP
	  // though.
	  continue;
	}
      if (sigismember (&current->pendingSignals, i->signal) == 1)
	{
	  NS_LOG_DEBUG ("deliver signal=" << i->signal);
	  // the signal is pending so, we try to deliver it.
	  if (i->flags & SA_SIGINFO)
	    {
	      siginfo_t info;
	      ucontext_t ctx;
	      i->sigaction (i->signal, &info, &ctx);
	    }
	  else
	    {
	      i->handler (i->signal);
	    }
	  sigdelset (&current->pendingSignals, i->signal);
	}
      if (sigismember (&current->process->pendingSignals, i->signal) == 1)
	{
	  NS_LOG_DEBUG ("deliver signal=" << i->signal);
	  // the signal is pending so, we try to deliver it.
	  if (i->flags & SA_SIGINFO)
	    {
	      siginfo_t info;
	      ucontext_t ctx;
	      i->sigaction (i->signal, &info, &ctx);
	    }
	  else
	    {
	      i->handler (i->signal);
	    }
	  sigdelset (&current->process->pendingSignals, i->signal);
	}
    }
}
int UtilsAllocateFd (void)
{
  for (int fd = 0; fd < MAX_FDS; fd++)
    {
      if (UtilsSearchOpenFd (fd) == -1)
	{
	  NS_LOG_DEBUG ("Allocated fd=" << fd);
	  return fd;
	}
    }
  return -1;
}
// Little hack to advance time when detecting a possible infinite loop.
void UtilsAdvanceTime (Thread *current)
{
  Time now = Now ();

  if (now == current->lastTime)
    {
      NS_LOG_DEBUG ("UtilsAdvanceTime current thread wait 1ms.");
      current->process->manager->Wait (Time( MilliSeconds(1) ) );
    }

  current->lastTime = Now ();
}

} // namespace ns3
