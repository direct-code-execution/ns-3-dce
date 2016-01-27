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
#include <list>
#include <fcntl.h>
#include <unistd.h>
#include "file-usage.h"
#include "ns3/global-value.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE ("DceProcessUtils");

namespace ns3 {

GlobalValue g_timeBase = GlobalValue ("SimulationTimeBase",
                                      "The timebase for this simulation",
                                      // January 1st, 2010, 00:00:00
                                      UintegerValue (1262304000),
                                      MakeUintegerChecker<uint32_t> ());
  //   unsigned long secondsSinceEpochOnFridayApril042008 = 1207284276;
  //   return secondsSinceEpochOnFridayApril042008;

uint32_t UtilsGetNodeId (void)
{
  if (gDisposingThreadContext)
    {
      return gDisposingThreadContext->process->nodeId;
    }
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
Thread *gDisposingThreadContext = 0;

Thread * Current (void)
{
  if (gDisposingThreadContext)
    {
      return gDisposingThreadContext;
    }

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
  int64_t m = time.GetMicroSeconds ();
  tv.tv_sec = m / 1000000L;
  tv.tv_usec = m % 1000000L;
  return tv;
}
struct timespec UtilsTimeToTimespec (Time time)
{
  struct timespec tv;
  int64_t n = time.GetNanoSeconds ();
  tv.tv_sec = (time_t)(n / 1000000000L);
  tv.tv_nsec = n % 1000000000;
  return tv;
}

std::string
UtilsGenerateIfNameFromIndex(uint32_t i) {
    std::stringstream ss;
    ss <<  "ns3-device " << i;
    return ss.str();
}

Time UtilsSimulationTimeToTime (Time time)
{
  UintegerValue uintegerValue;
  GlobalValue::GetValueByName ("SimulationTimeBase", uintegerValue);
  return time + Seconds (uintegerValue.Get ());
}
Time UtilsTimeToSimulationTime (Time time)
{
  UintegerValue uintegerValue;
  GlobalValue::GetValueByName ("SimulationTimeBase", uintegerValue);
  return time - Seconds (uintegerValue.Get ());
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
  if (!current)
    {
      return;
    }

  // we try to check if we
  // have pending signals and we deliver them if we have any.
  for (std::vector<SignalHandler>::iterator i = current->process->signalHandlers.begin ();
       i != current->process->signalHandlers.end (); ++i)
    {
      if (sigismember (&current->signalMask, i->signal) == 1
          && i->signal != SIGKILL
          && i->signal != SIGSTOP)
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
  Thread *current = Current ();
  NS_LOG_FUNCTION (current);
  NS_ASSERT (current != 0);
  std::map<int,FileUsage *>::iterator end = current->process->openFiles.end ();
  std::map<int,FileUsage *>::iterator it = end;

  for (int fd = 0; fd < MAX_FDS; fd++)
    {
      it = current->process->openFiles.find (fd);
      if (it == end)
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
//      NS_LOG_DEBUG ("UtilsAdvanceTime current thread wait 1ms.");
      //current->process->manager->Wait (Time (MilliSeconds (1)));
      NS_LOG_DEBUG ("UtilsAdvanceTime current thread wait 1µs.");
      current->process->manager->Wait (Time (MicroSeconds (1)));
    }

  current->lastTime = Now ();
}

std::string
GetTimeStamp ()
{
  Time now = Now ();
  time_t realnow = time (0);
  std::ostringstream oss;
  oss << ((long)now.GetSeconds ());
  std::string sec = oss.str ();
  uint32_t indent = 10;
  std::string padding = "";
  if (sec.length () < indent)
    {
      padding = std::string (indent - sec.length (), ' ');
    }
  sec = padding + sec;
  padding = "";
  oss.str ("");
  oss.clear ();
  oss << now;
  indent = 25;
  std::string ns = oss.str ();
  if (ns.length () < indent)
    {
      padding = std::string (indent - ns.length (), ' ');
    }
  ns = padding + ns;
  padding = "";
  oss.str ("");
  oss.clear ();
  oss << "NS3 Time: " << sec << "s (" << ns << ") , REAL Time: " << realnow;
  return oss.str ();
}

std::list<std::string>
Split (std::string input, std::string sep)
{
  std::list<std::string> retval;
  std::string::size_type cur = 0, next;
  while (true)
    {
      next = input.find (sep, cur);
      if (next == cur)
        {
          cur++;
          continue;
        }
      else if (next == std::string::npos)
        {
          if (input.size () != cur)
            {
              retval.push_back (input.substr (cur, input.size () - cur));
            }
          break;
        }
      retval.push_back (input.substr (cur, next - cur));
      cur = next + 1;
    }
  return retval;
}

std::string
FindExecFile (std::string root, std::string envPath, std::string fileName, uid_t uid, gid_t gid, int *errNo)
{
  struct stat st;
  std::string found = "";
  *errNo = ENOENT;

  int idx = fileName.find ('/', 0);

  if (idx >= 0)  // fileName contain a '/'
    {
      std::string vFile = UtilsGetRealFilePath (fileName);
      if (0 == ::stat (vFile.c_str (), &st))
        {
          if (((uid) && CheckExeMode (&st, uid, gid)) || (!uid))
            {
              return vFile;
            }
        }
      if (0 == ::stat (fileName.c_str (), &st))
        {
          if (((uid) && CheckExeMode (&st, uid, gid)) || (!uid))
            {
              return fileName;
            }
        }
    }
  else
    {
      std::list<std::string> paths = Split (envPath, ":");
      for (std::list<std::string>::const_iterator i = paths.begin (); i != paths.end (); i++)
        {
          std::string test = root + "/" + *i + "/" + fileName;
          if (0 == ::stat (test.c_str (), &st))
            {
              if (((uid) && CheckExeMode (&st, uid, gid)) || (!uid))
                {
                  found = test;
                  break;
                }
              else
                {
                  *errNo = EACCES;
                }
            }
        }
    }
  return found;
}

bool
CheckExeMode (struct stat *st, uid_t uid, gid_t gid)
{
  return ((gid != st->st_gid) && (uid != st->st_uid) && ((st->st_mode & (S_IROTH | S_IXOTH)) == (S_IROTH | S_IXOTH)))
         || ((gid == st->st_gid) && (uid != st->st_uid) && ((st->st_mode & (S_IRGRP | S_IXGRP)) ==  (S_IRGRP | S_IXGRP)))
         || ((uid == st->st_uid) && ((st->st_mode & (S_IRUSR | S_IXUSR)) == (S_IRUSR | S_IXUSR)));
}
void
FdDecUsage (int fd)
{
  Thread *current = Current ();

  FileUsage *fu = current->process->openFiles[fd];

  if (fu && fu->DecUsage ())
    {
      current->process->openFiles.erase (fd);
      delete fu;
      fu = 0;
    }
}
bool
CheckFdExists (Process* const p, int const fd, bool const opened)
{
  std::map<int,FileUsage *>::iterator it = p->openFiles.find (fd);

  if (it != p->openFiles.end ())
    {
      return !opened || (!(*it).second->IsClosed ());
    }

  return false;
}
int getRealFd (int fd, Thread *current)
{
  std::map<int,FileUsage *>::iterator it = current->process->openFiles.find (fd);
  if (current->process->openFiles.end () == it)
    {
      return -1;
    }
  FileUsage *fu = it->second;
  if (fu->IsClosed ())
    {
      return -1;
    }
  return fu->GetFile ()->GetRealFd ();
}
std::string PathOfFd (int fd)
{
  char proc[50];
  char direc[PATH_MAX + 1];

  sprintf (proc, "/proc/self/fd/%d", fd);

  memset (direc, 0, PATH_MAX + 1);

  ssize_t r = readlink (proc, direc, sizeof(direc) - 1);

  if (r >= 0)
    {
      return std::string (direc);
    }
  return std::string ("");
}
bool
CheckShellScript (std::string fileName,
                  std::ostringstream &shellName, std::ostringstream &optionalArg)
{
  int fd = open (fileName.c_str (), O_RDONLY);

  if (fd < 0)
    {
      return false;
    }
  //A maximum line length of 127 characters is allowed for the first line in a #! executable shell script.
  char firstLine[128];
  ssize_t lg = read (fd, firstLine, 127);
  close (fd);
  if ((lg <= 2) || ('#' != firstLine [0]) || ('!' != firstLine [1]))
    {
      return false;
    }
  ssize_t crsr = 2;
  firstLine [lg] = 0;
  while (' ' == firstLine [crsr])
    {
      crsr++;
      if (crsr >= lg)
        {
          return false;
        }
    }
  ssize_t startShellName = crsr;
  crsr++;
  while (firstLine [crsr] && (' ' != firstLine [crsr]) && ('\n' != firstLine [crsr]))
    {
      crsr++;
      if (crsr >= lg)
        {
          break;
        }
    }
  ssize_t endShellName = crsr;
  ssize_t lShellName = endShellName - startShellName;
  if (lShellName <= 0)
    {
      return false;
    }
  shellName << std::string (firstLine + startShellName, lShellName);
  if (crsr >= lg)
    {
      return true;
    }
  ssize_t startOpt = crsr;
  while (firstLine[crsr]&&('\n' !=  firstLine [crsr]))
    {
      crsr++;
      if (crsr >= lg)
        {
          break;
        }
    }
  ssize_t lOpt = crsr - startOpt;
  if (lOpt <= 0)
    {
      return true;
    }
  optionalArg << std::string (firstLine + startOpt, lOpt);

  return true;
}
char * seek_env (const char *name, char **array)
{
  int namelen = strlen (name);
  char **cur;
  for (cur = array; cur != 0 &&  *cur != 0; cur++)
    {
      char *equal = strchr (*cur, '=');
      if (equal == 0)
        {
          continue;
        }
      if (strncmp (*cur, name, namelen) != 0)
        {
          continue;
        }
      return equal + 1;
    }
  return 0;
}
std::string UtilsGetCurrentDirName (void)
{
  // DCE and NS3 never change the cwd.
  static std::string pwd = "";
  if (pwd.length () > 0)
    {
      return pwd;
    }
  char *thePwd = get_current_dir_name ();
  int fd = open (thePwd, O_RDONLY);
  pwd = PathOfFd (fd);
  close (fd);
  free (thePwd);
  return pwd;
}
} // namespace ns3
