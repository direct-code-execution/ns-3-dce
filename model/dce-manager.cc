/* -*-	Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "dce-manager.h"
#include "process.h"
#include "task-manager.h"
#include "libc-dce.h"
#include "unix-fd.h"
#include "unix-file-fd.h"
#include "utils.h"
#include "kingsley-alloc.h"
#include "dce-stdio.h"
#include "dce-unistd.h"
#include "dce-pthread.h"
#include "dce-fcntl.h"
#include "sys/dce-stat.h"
#include "loader-factory.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/enum.h"
#include "file-usage.h"
#include "wait-queue.h"
#include "waiter.h"
#include "dce-dirent.h"
#include "exec-utils.h"

#include <errno.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>

NS_LOG_COMPONENT_DEFINE ("DceManager");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DceManager);

class Waiter;

TypeId
DceManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DceManager")
    .SetParent<Object> ()
    .AddConstructor<DceManager> ()
    .AddTraceSource ("Exit", "A process has exited",
                     MakeTraceSourceAccessor (&DceManager::m_processExit))
    .AddAttribute ("FirstPid", "The PID used by default when creating a process in this manager.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&DceManager::m_nextPid),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MinimizeOpenFiles", "For each DCE virtual process running it exists 3 files opened (stdin,stdout and stderr),"
                   " if you enable this flag, the 2 files stderr and stdout will be closed between each write in order to "
                   " minimize the number of opened files and then maximize the number of DCE virtual process running at the same time.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DceManager::m_minimizeFiles),
                   MakeBooleanChecker ())
  ;
  return tid;
}

DceManager::DceManager ()
{
  NS_LOG_FUNCTION (this);
}
DceManager::~DceManager ()
{
  NS_LOG_FUNCTION (this);
}
void
DceManager::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  struct Process *tmp;
  std::map<uint16_t, Process*> mapCopy = m_processes;

  m_processes.clear ();
  for (std::map<uint16_t, Process*>::iterator it = mapCopy.begin (); it != mapCopy.end (); it++)
    {
      tmp = it->second;
      std::string statusWord = "Never ended.";
      AppendStatusFile (tmp->pid, tmp->nodeId, statusWord);
      DeleteProcess (tmp, PEC_NS3_END);
    }
  mapCopy.clear ();
  Object::DoDispose ();
}

struct ::Libc *
DceManager::GetLibc (void)
{
  static struct ::Libc *libc = 0;
  if (libc != 0)
    {
      return libc;
    }
  libc_dce (&libc);
  return libc;
}

void
DceManager::EnsureDirectoryExists (struct Thread *current, std::string dirName)
{
  int fd = dce_open (dirName.c_str (), O_DIRECTORY | O_RDONLY, 0);
  if (fd != -1)
    {
      dce_close (fd);
    }
  else if (current->err == ENOENT)
    {
      int status = dce_mkdir (dirName.c_str (), S_IRWXU | S_IRWXG);
      if (status == -1)
        {
          NS_FATAL_ERROR ("Could not create directory " << dirName
                                                        << ": " << strerror (current->err));
        }
    }
  else
    {
      NS_FATAL_ERROR ("Could not open \"" << dirName << "\"");
    }
}

int
DceManager::CreatePidFile (struct Thread *current, std::string filename)
{
  EnsureDirectoryExists (current, "/var");
  EnsureDirectoryExists (current, "/var/log");
  std::ostringstream oss;
  oss << "/var/log/" << current->process->pid;
  std::string pidDirName = oss.str ();
  EnsureDirectoryExists (current, oss.str ());
  oss << "/" << filename;
  std::string s = oss.str ();
  int fd = dce_creat (s.c_str (), S_IWUSR | S_IRUSR);
  return fd;
}
int (*DceManager::PrepareDoStartProcess (Thread * current)) (int, char **, char **)
{
  // Note : this preparing process is out of method DoStartProcess in order to
  //        free all local variable in particular the std library ones before the main execution
  //        because main call will reset allocation counters of std library then we must not free std::string
  //        after the main call !
  int err = 0;
  int (*main)(int, char **, char **) = 0;
  GetLibc ();
  UnixFd *unixFd = 0;

  if (current->process->stdinFilename.length () > 0)
    {
      std::string fullpath = UtilsGetRealFilePath (current->process->stdinFilename);
      int realFd = ::open (fullpath.c_str (), O_RDONLY, 0);

      if (-1 == realFd)
        {
          NS_FATAL_ERROR ("Unable to open stdin file : " << current->process->stdinFilename);
        }
      unixFd = new UnixFileFd (realFd);
    }
  else
    {
      unixFd = new TermUnixFileFd (0);
    }
  // create fd 0
  unixFd->IncFdCount ();
  current->process->openFiles[0] = new FileUsage (0, unixFd);

  // create fd 1
  int fd = CreatePidFile (current, "stdout");
  NS_ASSERT (fd == 1);
  // create fd 2
  fd = CreatePidFile (current, "stderr");
  NS_ASSERT (fd == 2);

  fd = CreatePidFile (current, "cmdline");
  NS_ASSERT (fd == 3);
  for (int i = 0; i < current->process->originalArgc; i++)
    {
      char *cur = current->process->originalArgv[i];
      dce_write (fd, cur, strlen (cur));
      dce_write (fd, " ", 1);
      current->process->timing.cmdLine += cur;
      if (i < (current->process->originalArgc - 1))
        {
          current->process->timing.cmdLine += ' ';
        }
    }
  dce_write (fd, "\n", 1);
  dce_close (fd);

  fd = CreatePidFile (current, "status");
  NS_ASSERT (fd == 3);
  {
    std::ostringstream oss;
    oss << "Start Time: " << GetTimeStamp () << std::endl;
    std::string tmp = oss.str ();
    const char *str = tmp.c_str ();
    dce_write (fd, str, strlen (str));
    dce_close (fd);
  }
  std::string vpath = "";
  char *pvpath = seek_env ("PATH", current->process->originalEnvp);
  if (pvpath)
    {
      vpath = std::string (pvpath);
    }
  int errNo = 0;
  std::string exeFullPath = SearchExecFile (current->process->originalArgv[0], vpath, getuid (), getgid (), &errNo);

  if (exeFullPath.length () <= 0)
    {
      std::string line = "Executable '";
      line += current->process->originalArgv[0];
      line += "' not found !  Please check your DCE_PATH and DCE_ROOT environment variables.";
      AppendStatusFile (current->process->pid, current->process->nodeId, line);
      NS_ASSERT_MSG (exeFullPath.length () > 0, line.c_str ());
      dce_exit (-1);
      return 0;
    }

  main = (int (*) (int, char **, char **))LoadMain (current->process->loader,
                                                    exeFullPath,
                                                    current->process,
                                                    err);

  if (!main)
    {
      std::string line = "No main method found in executable file '";
      line += exeFullPath;
      line += "'.";
      AppendStatusFile (current->process->pid, current->process->nodeId, line);
      NS_ASSERT_MSG (main, line.c_str ());
    }
  else
    {
      std::string  line = "Starting: " + exeFullPath;
      AppendStatusFile (current->process->pid, current->process->nodeId, line);
    }

  return main;
}
void
DceManager::DoStartProcess (void *context)
{
  Thread *current = (Thread *)context;
  int (*main)(int, char **, char **) = PrepareDoStartProcess (current);
  int retval = 127;

  if (main)
    {
      StartProcessDebugHook ();
      retval = main (current->process->originalArgc, current->process->originalArgv,
                     current->process->originalEnvp);
    }
  dce_exit (retval);
}

void
DceManager::StartProcessDebugHook (void)
{
}

struct Process *
DceManager::CreateProcess (std::string name, std::string stdinfilename, std::vector<std::string> args,
                           std::vector<std::pair<std::string,std::string> > envs, int pid)
{
  struct Process *process = new Process ();
  process->euid = 0;
  process->ruid = 0;
  process->suid = 0;
  process->egid = 0;
  process->rgid = 0;
  process->sgid = 0;
  process->alloc = new KingsleyAlloc ();
  process->originalArgv = 0;
  process->originalArgc = 0;
  process->originalEnvp = 0;
  SetArgv (process, name, args);
  SetEnvp (process, envs);
  Ptr<LoaderFactory> loaderFactory = this->GetObject<LoaderFactory> ();
  process->loader = loaderFactory->Create (process->originalArgc, process->originalArgv,
                                           process->originalEnvp);
  process->timing.exitValue = 0;
  process->timing.ns3Start = Now ().GetNanoSeconds ();
  process->timing.realStart = time (0);
  process->timing.ns3End = 0;
  process->timing.realEnd = 0;
  process->timing.cmdLine = "";

  process->name = name;
  process->ppid = 0;
  process->pgid = 0;
  process->pid = pid ? pid : AllocatePid ();
  process->manager = this;
  sigemptyset (&process->pendingSignals);

  SetDefaultSigHandler (process->signalHandlers);

  // we reserve 0 for the mutex initialized with PTHREAD_MUTEX_INITIALIZER
  // we reserve 2 for a destroyed mutex
  // 1 is such a common value that we try to avoid it.
  // we unfortunately cannot avoid the value 0 because we
  // have to be compatible with the system PHREAD_MUTEX_INITIALIZER
  // which uses the value 0.
  process->nextMid = 3;
  // we reserve 2 for a destroyed mutex
  // 0 and 1 are such common values that we avoid them.
  process->nextSid = 3;
  // we reserve 2 for a destroyed condition variable
  // 0 and 1 are such common values that we avoid them.
  process->nextCid = 3;
  process->cwd = "/";
  process->pstdin = 0;
  process->pstdout = 0;
  process->pstderr = 0;
  process->penvp = 0;

  process->uMask = 022;

  process->stdinFilename = stdinfilename;

  //"seeding" random variable
  process->rndVariable = CreateObject<UniformRandomVariable> ();
  process->rndVariable->SetAttribute ("Min", DoubleValue (0));
  process->rndVariable->SetAttribute ("Max", DoubleValue (RAND_MAX));

  process->nodeId = UtilsGetNodeId ();

  process->minimizeFiles = (m_minimizeFiles ? 1 : 0);

  if (!pid)
    {
      m_processes[process->pid] = process;
    }

  return process;
}
void
DceManager::TaskSwitch (enum Task::SwitchType type, void *context)
{
  Process *process = (Process *)context;
  switch (type)
    {
    case Task::TO:
      process->loader->NotifyStartExecute ();
      process->alloc->SwitchTo ();
      break;
    case Task::FROM:
      process->loader->NotifyEndExecute ();
      break;
    }
}
uint16_t
DceManager::Start (std::string name, std::string stdinfilename, std::vector<std::string> args,
                   std::vector<std::pair<std::string,std::string> > envs,
                   uid_t uid, uid_t euid, uid_t gid, uid_t egid)
{
  NS_LOG_FUNCTION (this << name << args.size ());
  struct Process *process = CreateProcess (name, stdinfilename, args, envs, 0);
  process->ruid = uid;
  process->euid = euid;
  process->rgid = gid;
  process->egid = egid;
  struct Thread *thread = CreateThread (process);
  Task *task = TaskManager::Current ()->Start (&DceManager::DoStartProcess, thread);
  task->SetContext (thread);
  task->SetSwitchNotifier (&DceManager::TaskSwitch, process);
  thread->task = task;
  return process->pid;
}
uint16_t
DceManager::Start (std::string name, std::string stdinfilename, uint32_t stackSize,
                   std::vector<std::string> args,
                   std::vector<std::pair<std::string,std::string> > envs,
                   uid_t uid, uid_t euid, uid_t gid, uid_t egid)
{
  NS_LOG_FUNCTION (this << name << stackSize << args.size () << envs.size ());
  struct Process *process = CreateProcess (name, stdinfilename, args, envs, 0);
  process->ruid = uid;
  process->euid = euid;
  process->rgid = gid;
  process->egid = egid;
  struct Thread *thread = CreateThread (process);
  Task *task = TaskManager::Current ()->Start (&DceManager::DoStartProcess, thread, stackSize);
  task->SetContext (thread);
  task->SetSwitchNotifier (&DceManager::TaskSwitch, process);
  thread->task = task;
  return process->pid;
}
uint16_t
DceManager::StartInternalTask ()
{
  std::vector<std::string> nullargs;
  std::vector<std::pair<std::string,std::string> > nullenvs;
  struct Process *process = CreateProcess ("internal", "dummy-stdin", nullargs, nullenvs, 0);
  struct Thread *thread = CreateThread (process);
  Task *task = new Task ();
  task->SetContext (thread);
  return process->pid;
}
uint16_t
DceManager::StartTemporaryTask ()
{
  std::vector<std::string> nullargs;
  std::vector<std::pair<std::string,std::string> > envs;
  struct Process *process = CreateProcess ("dummy", "dummy-stdin", nullargs, envs, 0);
  struct Thread *thread = CreateThread (process);
  Task *task = new Task ();
  task->SetContext (thread);
  thread->task = task;
  NS_LOG_FUNCTION (this << process->pid << thread << TaskManager::Current ()->CurrentTask ());
  TaskManager::Current ()->EnterHiTask (task);
  return process->pid;
}

void
DceManager::ResumeTemporaryTask (uint16_t pid)
{
  Process *process = SearchProcess (pid);
  Thread *thread = process->threads.front ();
  NS_LOG_FUNCTION (this << process->pid << thread << thread->task << TaskManager::Current ()->CurrentTask ());
  TaskManager::Current ()->EnterHiTask (thread->task);
  return;
}

void
DceManager::SuspendTemporaryTask (uint16_t pid)
{
  Process *process = SearchProcess (pid);
  Thread *thread = process->threads.front ();
  NS_LOG_FUNCTION (this << process->pid << TaskManager::Current ()->CurrentTask ());
  TaskManager::Current ()->LeaveHiTask (thread->task);
  return;
}
void
DceManager::StopTemporaryTask (uint16_t pid)
{
  NS_LOG_FUNCTION (this);
  Process *process = SearchProcess (pid);
  Thread *thread = process->threads.front ();
  Task *task = thread->task;
  DeleteProcess (process, PEC_EXIT);
  NS_LOG_FUNCTION (this << process->pid << TaskManager::Current ()->CurrentTask ());
  TaskManager::Current ()->LeaveHiTask (task);
  return;
}

uint16_t
DceManager::AllocatePid (void)
{
  // Note that this function purposedly never allocates pid 0
  // to make it useable in the kernel stack layer for kernel-level
  // special tasks (which thus all share the same pid)
  NS_LOG_FUNCTION (this);
  for (uint16_t i = 0; i < 0xffff; i++)
    {
      uint16_t candidatePid = (m_nextPid + i) & 0xffff;
      if (candidatePid > 1 && SearchProcess (candidatePid) == 0)
        {
          m_nextPid = (candidatePid + 1) & 0xffff;
          return candidatePid;
        }
    }
  NS_FATAL_ERROR ("Too many processes");
  return 0; // quiet compiler
}
uint16_t
DceManager::AllocateTid (const struct Process *process) const
{
  for (uint16_t tid = 0; tid < 0xffff; tid++)
    {
      bool found = false;
      for (std::vector<struct Thread *>::const_iterator i = process->threads.begin (); i != process->threads.end (); ++i)
        {
          struct Thread *tmp = *i;
          if (tmp->tid == tid)
            {
              found = true;
              break;
            }
        }
      if (!found)
        {
          return tid;
        }
    }
  NS_FATAL_ERROR ("We attempted to allocate a new tid for this process but none were available: "
                  "too many threads created at the same time.");
  // quiet compiler
  return 0;
}
struct Thread *
DceManager::CreateThread (struct Process *process)
{
  NS_LOG_FUNCTION (this << process);
  struct Thread *thread = new Thread ();
  NS_LOG_DEBUG ("Create " << thread);
  thread->err = 0;
  thread->tid = AllocateTid (process);
  thread->process = process;
  thread->isDetached = false;
  thread->hasExitValue = false;
  thread->joinWaiter = 0;
  thread->lastTime = Time (0);
  thread->childWaiter = 0;
  thread->pollTable = 0;
  thread->ioWait = std::make_pair ((UnixFd*)0,(WaitQueueEntry*)0);
  sigemptyset (&thread->signalMask);
  if (!process->threads.empty ())
    {
      // copy all key values.
      Thread *tmp = process->threads.front ();
      thread->keyValues = tmp->keyValues;
      // reset all values to NULL
      for (std::list<struct ThreadKeyValue>::iterator i = thread->keyValues.begin ();
           i != thread->keyValues.end (); ++i)
        {
          i->value = 0;
        }
    }
  process->threads.push_back (thread);
  return thread;
}

void
DceManager::Exit (void)
{
  TaskManager::Current ()->Exit ();
}

bool
DceManager::ThreadExists (Thread *thread)
{
  if (!thread)
    {
      return false;
    }
  if (!thread->process)
    {
      return false;
    }

  Process *process = thread->process;

  for (std::vector<Thread *>::const_iterator j = process->threads.begin ();
       j != process->threads.end (); ++j)
    {
      Thread *cur = *j;
      if (cur == thread)
        {
          return true;
        }
    }

  return false;
}

uint16_t
DceManager::Clone (Thread *thread)
{
  NS_LOG_FUNCTION (this << thread);
  Process *clone = new Process ();
  clone->euid = 0;
  clone->ruid = 0;
  clone->suid = 0;
  clone->egid = 0;
  clone->rgid = 0;
  clone->sgid = 0;
  // XXX setup
  clone->originalArgv = 0;
  clone->originalArgc = 0;
  clone->originalEnvp = 0;
  clone->timing.exitValue = 0;
  clone->timing.ns3Start = Now ().GetNanoSeconds ();
  clone->timing.realStart = time (0);
  clone->timing.ns3End = 0;
  clone->timing.realEnd = 0;
  clone->name = thread->process->name;
  clone->ppid = thread->process->pid;
  clone->pgid = thread->process->pgid;
  clone->pid = AllocatePid ();
  thread->process->children.insert (clone->pid);
  // dup each file descriptor.
  for (std::map <int, FileUsage*>::iterator i = thread->process->openFiles.begin ();
       i != thread->process->openFiles.end (); ++i)
    {
      int fd = i->first;
      FileUsage* fu = i->second;

      if (fu && fu->GetFile ())
        {
          fu->GetFile ()->IncFdCount ();
          fu->GetFile ()->Ref ();
          clone->openFiles[fd] = new FileUsage (fd, fu->GetFile ());
        }
    }
  // don't copy threads, semaphores, mutexes, condition vars
  // XXX: what about file streams ?
  clone->manager = this;
  sigemptyset (&clone->pendingSignals);

  SetDefaultSigHandler (clone->signalHandlers);

  clone->nextMid = thread->process->nextMid;
  clone->nextSid = thread->process->nextSid;
  clone->nextCid = thread->process->nextCid;
  clone->cwd = thread->process->cwd;
  clone->pstdin = thread->process->pstdin;
  clone->pstdout = thread->process->pstdout;
  clone->pstderr = thread->process->pstderr;
  clone->penvp = thread->process->penvp;

  //"seeding" random variable
  clone->rndVariable = CreateObject<UniformRandomVariable> ();
  clone->rndVariable->SetAttribute ("Min", DoubleValue (0));
  clone->rndVariable->SetAttribute ("Max", DoubleValue (RAND_MAX));
  m_processes[clone->pid] = clone;
  Thread *cloneThread = CreateThread (clone);

  clone->loader = thread->process->loader->Clone ();
  clone->alloc = thread->process->alloc->Clone ();
  Task *task = TaskManager::Current ()->Clone (thread->task);
  if (task != 0)
    {
      // parent. complete setup. return.
      task->SetContext (cloneThread);
      task->SetSwitchNotifier (&DceManager::TaskSwitch, clone);
      cloneThread->task = task;
      return clone->pid;
    }
  // child.
  return 0;
}

void
DceManager::Yield (void)
{
  TaskManager::Current ()->Yield ();
}

void
DceManager::Wait (void)
{
  TaskManager::Current ()->Sleep ();
}

Time
DceManager::Wait (Time timeout)
{
  return TaskManager::Current ()->Sleep (timeout);
}

void
DceManager::Wakeup (Thread *thread)
{
  NS_ASSERT (ThreadExists (thread));
  return TaskManager::Current ()->Wakeup (thread->task);
}

void
DceManager::SetFinishedCallback (uint16_t pid, Callback<void,uint16_t,int> cb)
{
  Process *process = SearchProcess (pid);
  if (process == 0)
    {
      return;
    }
  process->finished = cb;
}
void
DceManager::Stop (uint16_t pid)
{
  NS_LOG_FUNCTION (this << pid);

  Process *process = SearchProcess (pid);
  if (process == 0)
    {
      return;
    }
  std::string statusWord = "Stopped by NS3.";
  AppendStatusFile (process->pid, process->nodeId, statusWord);
  DeleteProcess (process, PEC_NS3_STOP);
}
void
DceManager::SigkillHandler (int signal)
{
  NS_ASSERT (signal == SIGKILL);
  dce_exit (-1);
}
void
DceManager::SigabrtHandler (int signal)
{
  NS_ASSERT (signal == SIGABRT);
  dce_exit (-2);
}
void
DceManager::CleanupThread (struct Thread *thread)
{
  if (thread->ioWait.first != 0 && thread->ioWait.second != 0)
    {
      thread->ioWait.first->RemoveWaitQueue (thread->ioWait.second, false);
      if (thread->ioWait.second)
        {
          delete thread->ioWait.second;
        }
      thread->ioWait = std::make_pair ((UnixFd*)0,(WaitQueueEntry*)0);
    }
  if (thread->pollTable != 0)
    {
      PollTable *lb = thread->pollTable;
      thread->pollTable = 0;
      lb->FreeWait ();
      delete lb;
      lb = 0;
    }
}

void
DceManager::DeleteThread (struct Thread *thread)
{
  NS_LOG_FUNCTION (this << thread);
  if (thread->task != 0)
    {
      // the task could be 0 if it was Exited by
      // pthread_exit and it was not pthread_detached.
      GetObject<TaskManager> ()->Stop (thread->task);
    }
  thread->task = 0;
  for (std::vector<Thread *>::iterator i = thread->process->threads.begin ();
       i != thread->process->threads.end (); ++i)
    {
      if (*i == thread)
        {
          thread->process->threads.erase (i);
          break;
        }
    }
  if (0 != thread->childWaiter)
    {
      Waiter *lb = thread->childWaiter;
      thread->childWaiter = 0;
      delete lb;
    }
  delete thread;
}

void
DceManager::DeleteProcess (struct Process *process, ProcessEndCause type)
{
  NS_LOG_FUNCTION (this << process << "pid" << std::dec << process->pid << "ppid" << process->ppid);

  // Remove Threads Waiters
  struct Thread *tmp;
  std::vector<Thread *> threads = process->threads;

  // First remove from wait queues: I am not more interressed of IO on Files
  while (!threads.empty ())
    {
      tmp = threads.back ();
      threads.pop_back ();
      CleanupThread (tmp);
      tmp = 0;
    }

  if (type == PEC_EXIT)
    {
      // We have a Current so we can call dce_close !
      std::map<int,FileUsage *> openFiles = process->openFiles;

      for (std::map <int, FileUsage*>::iterator i = openFiles.begin ();
           i != openFiles.end (); ++i)
        {
          int fd = i->first;
          FileUsage* fu = i->second;

          if (fu)
            {
              // Nullify count of thread using this file.
              fu->NullifyUsage ();
              // Close my files and eventually wake up others processes.
              dce_close (fd);
            }
        }
      openFiles.clear ();
    }

  // Close all streams opened
  for (uint32_t i =  0; i < process->openStreams.size (); i++)
    {
      // Note that, while this code might look straightforward,
      // it is not so. Specifically, we call here the system fclose
      // function which will call indirectly system_close in
      // system-wrappers.cc which will call dce_close in dce-fd.cc
      // which will return -1 because it is called without a 'current'
      // thread
      dce_fclose_unconditional (process->openStreams[i]);
    }
  process->openStreams.clear ();

  // Close all DIR opened
  for (uint32_t i =  0; i < process->openDirs.size (); i++)
    {
      dce_internalClosedir (process->openDirs[i], 0);
    }
  process->openDirs.clear ();

  if (!process->finished.IsNull ())
    {
      process->finished (process->pid, process->timing.exitValue);
    }
  // stop itimer timers if there are any.
  process->itimer.Cancel ();
  // Delete File References Memory
  std::map<int,FileUsage *> openFiles = process->openFiles;
  process->openFiles.clear ();
  for (std::map <int, FileUsage*>::iterator i = openFiles.begin ();
       i != openFiles.end (); ++i)
    {
      FileUsage* fu = i->second;

      if (fu)
        {
          delete fu;
        }
    }
  openFiles.clear ();

  // finally, delete remaining threads
  while (!process->threads.empty ())
    {
      tmp = process->threads.back ();
      process->threads.pop_back ();
      DeleteThread (tmp);
    }
  // delete all mutexes
  for (uint32_t i = 0; i < process->mutexes.size (); ++i)
    {
      struct Mutex *mutex = process->mutexes[i];
      // XXX: do some error checking here to ensure that no thread is
      // blocked in a critical section.
      delete mutex;
    }
  process->mutexes.clear ();
  // delete all semaphores
  for (uint32_t i = 0; i < process->semaphores.size (); ++i)
    {
      struct Semaphore *semaphore = process->semaphores[i];
      // XXX: do some error checking here to ensure that no thread is
      // blocked in a critical section.
      delete semaphore;
    }
  // delete all extra buffers
  while (!process->allocated.empty ())
    {
      void *buffer = process->allocated.back ();
      process->allocated.pop_back ();
      free (buffer);
    }

  int ppid = process->ppid;

  if ((type == PEC_EXIT)||(type == PEC_NS3_STOP))
    {
      // Re-parent children
      std::set<uint16_t> children = process->children;
      process->children.clear ();
      std::set<uint16_t>::iterator it;

      for (it = children.begin (); it != children.end (); it++)
        {
          Process *child = SearchProcess (*it);

          if (child)
            {
              child->ppid = 1;
              if ((child->pid > 1) && !child->loader && !child->alloc)
                {
                  m_processes.erase (child->pid);
                  delete child;
                }
            }
        }
      m_processExit (process->pid, process->timing.exitValue);
    }
  delete process->loader;
  process->loader = 0;
  if (type == PEC_EXIT)
    {
      // Only dispose from good context
      process->alloc->Dispose ();
    }
  delete process->alloc;
  process->alloc = 0;

  if ((type == PEC_EXIT)||(type == PEC_NS3_STOP))
    {
      if (ppid > 1)
        {
          // Warn father
          ChildFinished (process->pid);
        }
      else
        {
          // ppid == 0 have no father, perhaps DCE, else ppid = 1 init : have lost it's real father.
          // remove ourselves from list of processes
          m_processes.erase (process->pid);
          // delete process data structure.
          delete process;
        }
    }
  else
    {
      delete process;
    }

}

bool
DceManager::CheckProcessContext (void) const
{
  return TaskManager::Current () != 0 && TaskManager::Current ()->CurrentTask () != 0;
}

Thread *
DceManager::SearchThread (uint16_t pid, uint16_t tid)
{
  NS_LOG_FUNCTION (this << pid << tid);
  NS_ASSERT (CheckProcessContext ());
  Process *process = SearchProcess (pid);
  if (process == 0)
    {
      return 0;
    }
  for (std::vector<Thread *>::const_iterator j = process->threads.begin ();
       j != process->threads.end (); ++j)
    {
      Thread *thread = *j;
      if (thread->tid == tid)
        {
          return thread;
        }
    }
  return 0;
}
Process *
DceManager::SearchProcess (uint16_t pid)
{
  NS_LOG_FUNCTION (this << pid);

  std::map<uint16_t, Process *>::iterator it = m_processes.find (pid);

  if (it !=  m_processes.end ())
    {
      return it->second;
    }
  return 0;
}

void
DceManager::SetArgv (struct Process *process, std::string filename, std::vector<std::string> args)
{
  NS_ASSERT (process->originalArgv == 0);
  int argc = args.size () + 1;
  char **argv = (char **)malloc (sizeof (char *) * (argc + 1));
  process->allocated.push_back (argv);
  argv[0] = strdup (filename.c_str ());
  process->allocated.push_back (argv[0]);
  for (uint32_t i = 0; i < args.size (); ++i)
    {
      char *arg = strdup (args[i].c_str ());
      NS_LOG_DEBUG ("argc=" << argc << " i=" << i << " v=" << arg);
      argv[i + 1] = arg;
      process->allocated.push_back (arg);
    }
  argv[argc] = 0;
  process->originalArgv = argv;
  process->originalArgc = argc;
  process->originalProgname = argv[0];
}

void
DceManager::SetEnvp (struct Process *process,
                     std::vector<std::pair<std::string,std::string> > envs)
{
  int envpc = envs.size ();
  char **envp = (char **)malloc (sizeof (char *) * (envpc + 1));
  process->allocated.push_back (envp);
  for (uint32_t i = 0; i < envs.size (); ++i)
    {
      int size = envs[i].first.size () + envs[i].second.size () + 1;
      envp[i] = (char*)malloc (size + 1);
      process->allocated.push_back (envp[i]);
      memcpy (envp[i], envs[i].first.c_str (), envs[i].first.size ());
      envp[i][envs[i].first.size ()] = '=';
      memcpy (envp[i] + envs[i].first.size () + 1, envs[i].second.c_str (), envs[i].second.size ());
      envp[i][size] = 0;
      NS_LOG_DEBUG ("envpc=" << envpc << " i=" << i << " v=" << envp[i]);
    }
  envp[envpc] = 0;
  process->originalEnvp = envp;
}

void
DceManager::AppendStatusFile (uint16_t pid, uint32_t nodeId,  std::string &line)
{
  std::ostringstream oss;
  oss << "files-" << nodeId << "/var/log/" << pid << "/status";
  std::string s = oss.str ();

  int fd = ::open (s.c_str (), O_WRONLY | O_APPEND, 0);

  if (fd >= 0) // XXX: When fork is used the pid directory is not created, I plan to fix it when I will work on fork/exec/wait...
    {
      oss.str ("");
      oss.clear ();
      oss << "      Time: " << GetTimeStamp () << " --> " << line << std::endl;
      std::string wholeLine = oss.str ();
      int l =  wholeLine.length ();
      const char *str = wholeLine.c_str ();
      ::write (fd, str, l);
      ::close (fd);
    }
}
void
DceManager::AppendProcFile (Process *p)
{
  if (!p)
    {
      return;
    }
  std::ostringstream oss;
  int fd = ::open ("exitprocs", O_WRONLY | O_APPEND | O_CREAT, 0644);

  if (fd >= 0)
    {
      struct stat st;
      if ((!fstat (fd, &st)) && (0 == st.st_size))
        {
          const char *header =  "NODE EXIT-CODE PID NS3-START-TIME NS3-END-TIME REAL-START-TIME REAL-END-TIME NS3-DURATION REAL-DURATION CMDLINE\n";

          ::write (fd, header, strlen (header));
        }

      oss << p->nodeId
          << ' ' << p->timing.exitValue
          << ' ' << p->pid
          << ' ' <<  p->timing.ns3Start
          << ' ' <<  p->timing.ns3End
          << ' ' <<  p->timing.realStart
          << ' ' <<  p->timing.realEnd
          << ' ' <<  ((p->timing.ns3End - p->timing.ns3Start) / (double) 1000000000)
          << ' ' <<  (p->timing.realEnd - p->timing.realStart)
          << ' ' <<  p->timing.cmdLine  << std::endl;
      std::string wholeLine = oss.str ();
      int l =  wholeLine.length ();
      const char *str = wholeLine.c_str ();
      ::write (fd, str, l);
      ::close (fd);
    }
}

std::map<uint16_t, Process *>
DceManager::GetProcs ()
{
  return m_processes;
}

void
DceManager::ChildFinished (uint16_t pid)
{
  // Get pid process.
  Process *child = SearchProcess (pid);
  NS_ASSERT (child);
  Process *p = SearchProcess (child->ppid);
  NS_ASSERT (p);
  WakeupChildWaiters (p);
  if (!true) // IF WAIT DONE !
    {
      std::set<uint16_t>::iterator it = p->children.find (pid);
      if (it != p->children.end ())
        {
          p->children.erase (it);
        }
      m_processes.erase (pid);
      delete child;
    }
}
bool
DceManager::WakeupChildWaiters (struct Process *p)
{
  bool ret = false;
  std::vector<Thread *> tt = p->threads;

  for (std::vector<Thread*>::iterator it = tt.begin (); it != tt.end (); it++)
    {
      Thread *t = *it;
      if (t && t->childWaiter)
        {
          t->childWaiter->Wakeup ();
        }
    }

  return ret;
}
void
DceManager::FinishChild (uint16_t pid)
{
  // A wait success on this proc
  Process *child = SearchProcess (pid);
  if (child)
    {
      m_processes.erase (pid);
      delete child;
    }
}
std::vector<std::string>
DceManager::CopyArgs (char *const argv[])
{
  std::vector<std::string> args;

  // Copy argv
  if (0 != argv)
    {
      char **v = (char **) argv;
      if (*v)
        {
          v++;      // Ignore argv[0]
        }
      while (*v)
        {
          char *s = *v++;
          args.push_back (s);
        }
    }
  return args;
}
int
DceManager::CopyEnv (char *const envp[], std::vector<std::pair<std::string,std::string> > &envs)
{
  if (0 != envp)
    {
      char **e = (char **) envp;

      while (*e)
        {
          char *s = *e++;
          char *c = s;

          while ((*c != 0) && (*c != '='))
            {
              c++;
            }

          if (('=' != *c) || (s == c))
            {
              Current ()->err = EINVAL;
              return -1;
            }
          std::string key = std::string (s, c);
          std::string val = std::string (1 + c);
          envs.push_back (std::make_pair (key, val));
        }
    }

  return 0;
}
void*
DceManager::LoadMain (Loader *ld, std::string filename, Process *proc, int &err)
{
  void *h = ld->Load ("libc-ns3.so", RTLD_GLOBAL);
  void *symbol = 0;
  struct ::Libc *libc = GetLibc ();

  if (h == 0)
    {
      err = ENOMEM;
      return 0;
    }
  else
    {
      symbol = ld->Lookup (h, "libc_setup");
      if (symbol == 0)
        {
          NS_FATAL_ERROR ("This is not our fake libc !");
        }
      // construct the libc now
      void (*libc_setup)(const struct Libc *fn);
      libc_setup = (void (*) (const struct Libc *))(symbol);
      libc_setup (libc);

      h = ld->Load ("libpthread-ns3.so", RTLD_GLOBAL);
      if (h == 0)
        {
          err = ENOMEM;
          return 0;
        }
      else
        {
          symbol = ld->Lookup (h, "libpthread_setup");
          if (symbol == 0)
            {
              NS_FATAL_ERROR ("This is not our fake libpthread !");
            }
          // construct libpthread now
          void (*libpthread_setup)(const struct Libc *fn);
          libpthread_setup = (void (*) (const struct Libc *))(symbol);
          libpthread_setup (libc);

          h = ld->Load ("librt-ns3.so", RTLD_GLOBAL);
          if (h == 0)
            {
              err = ENOMEM;
              return 0;
            }
          symbol = ld->Lookup (h, "librt_setup");
          if (symbol == 0)
            {
              NS_FATAL_ERROR ("This is not our fake librt !");
            }
          // construct librt now
          void (*librt_setup)(const struct Libc *fn);
          librt_setup = (void (*) (const struct Libc *))(symbol);
          librt_setup (libc);

          h = ld->Load ("libm-ns3.so", RTLD_GLOBAL);
          if (h == 0)
            {
              err = ENOMEM;
              return 0;
            }
          symbol = ld->Lookup (h, "libm_setup");
          if (symbol == 0)
            {
              NS_FATAL_ERROR ("This is not our fake libm !");
            }
          // construct libm now
          void (*libm_setup)(const struct Libc *fn);
          libm_setup = (void (*) (const struct Libc *))(symbol);
          libm_setup (libc);

          // finally, call into 'main'.
          h = ld->Load (filename, RTLD_GLOBAL);

          if (h == 0)
            {
              err = EACCES;
              return 0;
            }
          else
            {
              proc->mainHandle = h;
              symbol = ld->Lookup (h, "main");
              err = (0 != symbol) ? 0 : ENOEXEC;
            }
        }
    }
  return symbol;
}
void
DceManager::DoExecProcess (void *c)
{
  int (*main)(int, char **, char **) = (int (*) (int, char **, char **))c;
  Thread *current = Current ();

  StartProcessDebugHook ();

  int retval = main (current->process->originalArgc, current->process->originalArgv,
                     current->process->originalEnvp);
  dce_exit (retval);
}
void
DceManager::SetDefaultSigHandler (std::vector<SignalHandler> &signalHandlers)
{
  // setup a signal handler for SIGKILL which calls dce_exit.
  struct SignalHandler handler;
  handler.signal = SIGKILL;
  handler.flags = 0;
  sigemptyset (&handler.mask);
  handler.handler = &DceManager::SigkillHandler;
  signalHandlers.push_back (handler);

  // setup a signal handler for SIGABRT which calls dce_exit.
  handler.signal = SIGABRT;
  handler.flags = 0;
  sigemptyset (&handler.mask);
  handler.handler = &DceManager::SigabrtHandler;
  signalHandlers.push_back (handler);
}
int
DceManager::Execve (const char *path, const char *argv0, char *const argv[], char *const envp[])
{
  Process pTemp;
  Process *process = Current ()->process;
  std::vector<std::pair<std::string,std::string> > envs;
  memset (&pTemp, 0, sizeof (Process));

  // Parse Verify environnement and arguments
  if (CopyEnv (envp, envs))
    {
      return -1;
    }
  std::ostringstream interpreter, optArgs;
  std::string filename = std::string (path);
  if (CheckShellScript (path, interpreter, optArgs))
    {
      std::vector<std::string> args, lasts = CopyArgs (argv);
      std::string opt = optArgs.str ();
      std::string shell = interpreter.str ();

      if (opt.length () > 0)
        {
          args.push_back (opt);
        }
      args.push_back (argv0);

      if (lasts.size () > 0)
        {
          args.insert (args.end (), lasts.begin (), lasts.end ());
        }

      SetArgv (&pTemp, argv0, args);

      std::string vpath = "";
      char *pvpath = seek_env ("PATH", *process->penvp);
      if (pvpath)
        {
          vpath = std::string (pvpath);
        }
      filename = SearchExecFile (shell, vpath, getuid (), getgid (), &(Current ()->err));
      if (filename.length () <= 0)
        {
          return -1;
        }
    }
  else
    {
      SetArgv (&pTemp, path, CopyArgs (argv));
    }
  SetEnvp (&pTemp, envs);
  envs.clear ();

  std::ostringstream oss;
  oss << "EXEC: ";
  for (int i = 0; i < pTemp.originalArgc; i++)
    {
      oss << pTemp.originalArgv[i] << ' ';
    }
  std::string line = oss.str ();
  AppendStatusFile (process->pid, process->nodeId, line);

  // Try to load the code and find a MAIN
  Ptr<LoaderFactory> loaderFactory = this->GetObject<LoaderFactory> ();
  pTemp.loader = loaderFactory->Create (pTemp.originalArgc, pTemp.originalArgv, pTemp.originalEnvp);

  int err = 0;

  FILE **pstdin = process->pstdin;
  FILE **pstdout = process->pstdout;
  FILE **pstderr = process->pstderr;
  char ***penvp = process->penvp;
  char *originalProgname = process->originalProgname;

  std::vector<FILE *> openStreams = process->openStreams;
  process->openStreams.clear ();
  process->pstdin = 0;
  process->pstdout = 0;
  process->pstderr = 0;
  process->penvp = 0;
  process->originalProgname = pTemp.originalProgname;

  void *main = LoadMain (pTemp.loader, filename, &pTemp, err);

  if (!main)
    {
      process->pstdin = pstdin;
      process->pstdout = pstdout;
      process->pstderr = pstderr;
      process->penvp = penvp;
      process->openStreams = openStreams;
      process->originalProgname = originalProgname;
      delete  pTemp.loader;
      pTemp.loader = 0;
      // delete all extra buffers
      while (!pTemp.allocated.empty ())
        {
          void *buffer = pTemp.allocated.back ();
          pTemp.allocated.pop_back ();
          free (buffer);
        }
      Current ()->err = err;
      line = "EXEC FAILED";
      AppendStatusFile (process->pid, process->nodeId, line);
      return -1;
    }

  while (!openStreams.empty ())
    {
      FILE *f = openStreams.back ();
      openStreams.pop_back ();
      if (f)
        {
          dce_fclose_onexec (f);
        }
    }

  // save old threads.
  std::vector<Thread *> Oldthreads = process->threads;
  process->threads.clear ();

  struct Thread *thread = CreateThread (process);

  Task *task = TaskManager::Current ()->Start (&DceManager::DoExecProcess, main,
                                               TaskManager::Current ()->GetStackSize (Current ()->task));
  task->SetContext (thread);
  task->SetSwitchNotifier (&DceManager::TaskSwitch, process);
  thread->task = task;

  // Review Process to release old stuff and put new stuff in place
  delete process->loader;

  // delete all extra buffers
  while (!process->allocated.empty ())
    {
      void *buffer = process->allocated.back ();
      process->allocated.pop_back ();
      free (buffer);
    }
  process->allocated = pTemp.allocated;

  process->loader = pTemp.loader;
  process->originalArgc = pTemp.originalArgc;
  process->originalArgv = pTemp.originalArgv;
  process->originalEnvp = pTemp.originalEnvp;
  *process->penvp = process->originalEnvp;
  process->name = std::string (path);
  process->signalHandlers.clear ();
  SetDefaultSigHandler (process->signalHandlers);
  process->atExitHandlers.clear ();
  process->nextMid = process->nextSid = process->nextCid = 0;
  process->mainHandle = pTemp.mainHandle;

  // Remove Threads Waiters
  struct Thread *tmp;
  std::vector<Thread *> threads = Oldthreads;

  // First remove from wait queues: I am not more interressed of IO on Files
  while (!threads.empty ())
    {
      tmp = threads.back ();
      threads.pop_back ();
      CleanupThread (tmp);
      tmp = 0;
    }
  threads.clear ();
  threads = Oldthreads;
  // finally, delete remaining threads
  while (!threads.empty ())
    {
      tmp = threads.back ();
      threads.pop_back ();
      DeleteThread (tmp);
    }
  threads.clear ();
  Oldthreads.clear ();
  process->alloc->Dispose ();
  delete process->alloc;
  process->alloc = new KingsleyAlloc ();

  while (!process->mutexes.empty ())
    {
      Mutex * m = process->mutexes.back ();
      process->mutexes.pop_back ();
      if (m)
        {
          delete m;
        }
    }

  while (!process->semaphores.empty ())
    {
      Semaphore * s = process->semaphores.back ();
      process->semaphores.pop_back ();
      if (s)
        {
          delete s;
        }
    }

  while (!process->conditions.empty ())
    {
      Condition * c = process->conditions.back ();
      process->conditions.pop_back ();
      if (c)
        {
          delete c;
        }
    }

  line = "EXEC SUCCESS";
  AppendStatusFile (process->pid, process->nodeId, line);
  TaskManager::Current ()->Exit ();
  // NEVER REACHED
  return -1;
}
void
DceManager::SetVirtualPath (std::string p)
{
  m_virtualPath = p;
}
std::string
DceManager::GetVirtualPath () const
{
  return m_virtualPath;
}
} // namespace ns3
