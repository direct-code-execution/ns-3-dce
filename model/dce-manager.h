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

#ifndef DCE_MANAGER_H
#define DCE_MANAGER_H

#include <string>
#include <map>
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/simulator.h"
#include "task-manager.h"

extern "C" struct Libc;

namespace ns3 {

struct Process;
struct Thread;
struct SignalHandler;
class Loader;


/**
 * \brief Manages a set of virtual POSIX processes within the same
 *        address space.
 *
 * Functionality provided by this class:
 *
 *  - a non-preemptive process scheduler. By default, this scheduler
 *    is a round robin scheduler. This scheduler allows a 'synchronous'
 *    programming style where functions can block until certain conditions
 *    are verified as opposed to event-driven programming as is common
 *    in most simulators.
 *  - user-space thread library (named fibers) used by the process
 *    scheduler to allocate stack space to each thread within each process.
 *    The default implementation uses in fact kernel-space POSIX threads
 *    but it's easy to switch to a real userspace implementation using the
 *    FiberManagerType attribute.
 *  - an ELF loader which controls how each user binary is loaded in memory
 *    and makes sure that each instance of the same binary has private global
 *    variables. The default implementation is ns3::ElfLoaderCopy but a
 *    more efficient implementation (both cpu and memory-wise can be found in
 *    ns3::ElfLoaderSmart) and it's easy to control which implementation is
 *    used with the LoaderType attribute.
 *
 * A couple of applications which you can try with this loader:
 *   - iperf: recompile with: "CFLAGS=-fPIC CXXFLAGS=-fPIC LDFLAGS=-pie ./configure && make"
 *   - ip: recompile iproute2 with: "make CCOPTS='-fPIC' LDFLAGS=-pie"
 *         Note: to get tc recompiled, it seems that you need to run bison by hand:
 *         "cd tc && bison -d -t -v -o emp_ematch.yacc.c emp_ematch.y"
 *   - ping: recompile iputils: edit Makefile: replace "CFLAGS=" with "CFLAGS+=" and run:
 *           "make CFLAGS=-fPIC LDFLAGS=-pie"
 */
class DceManager : public Object
{
public:
  typedef Callback<void> Runnable;
  typedef enum
  {
    PEC_EXIT,
    PEC_NS3_END, // NO MORE EVENTS
    PEC_NS3_STOP, // STOP AT PREDEFINED TIME
  } ProcessEndCause;

  static TypeId GetTypeId (void);

  DceManager ();
  virtual ~DceManager ();

  uint16_t Start (std::string name, std::string stdinfilename, std::vector<std::string> args,
                  std::vector<std::pair<std::string,std::string> > envs,
                  uid_t uid, uid_t euid, uid_t gid, uid_t egid);
  uint16_t Start (std::string name, std::string stdinfilename, uint32_t stackSize,
                  std::vector<std::string> args,
                  std::vector<std::pair<std::string,std::string> > envs,
                  uid_t uid, uid_t euid, uid_t gid, uid_t egid);
  uint16_t StartInternalTask ();
  void SetFinishedCallback (uint16_t pid, Callback<void,uint16_t,int> cb);
  void Stop (uint16_t pid);

  // internal methods
  struct Thread * CreateThread (struct Process *process);
  void DeleteProcess (struct Process *process, ProcessEndCause type);
  void DeleteThread (struct Thread *thread);

  Thread * SearchThread (uint16_t pid, uint16_t tid);
  Process * SearchProcess (uint16_t pid);
  void FinishChild (uint16_t pid); // A wait success on this proc

  void Exit (void);
  void Wakeup (Thread *thread);
  void Wait (void);
  Time Wait (Time timeout);
  void Yield (void);
  uint16_t Clone (Thread *thread);
  std::map<uint16_t, Process *> GetProcs ();
  static void AppendStatusFile (uint16_t pid, uint32_t nodeId, std::string &line);
  int Execve (const char *path, const char *argv0, char *const argv[], char *const envp[]);
  // Path used by simulated methods 'execvp' and 'execlp'
  void SetVirtualPath (std::string p);
  std::string GetVirtualPath () const;
  static void AppendProcFile (Process *p);
  uint16_t StartTemporaryTask ();
  void StopTemporaryTask (uint16_t pid);
  void ResumeTemporaryTask (uint16_t pid);
  void SuspendTemporaryTask (uint16_t pid);
  struct Process* CreateProcess (std::string name, std::string stdinfilename, std::vector<std::string> args,
                                 std::vector<std::pair<std::string,std::string> > envs, int pid);
  void Panic ();

private:
  // inherited from Object.
  virtual void DoDispose (void);

  static int (*PrepareDoStartProcess (Thread * current)) (int, char **, char **);
  static void DoStartProcess (void *context);
  bool CheckProcessContext (void) const;
  uint16_t AllocatePid (void);
  uint16_t AllocateTid (const struct Process *process) const;
  static void SigkillHandler (int signal);
  static void SigabrtHandler (int signal);
  bool ThreadExists (Thread *thread);
  static struct ::Libc * GetLibc (void);
  void SetArgv (struct Process *process, std::string filename, std::vector<std::string> args);
  void SetEnvp (struct Process *process, std::vector<std::pair<std::string,std::string> > envp);
  static void EnsureDirectoryExists (struct Thread *current, std::string dirName);
  static int CreatePidFile (struct Thread *current, std::string prefix);
  static void TaskSwitch (enum Task::SwitchType type, void *context);
  static void StartProcessDebugHook (void);
  void ChildFinished (uint16_t pid);
  bool WakeupChildWaiters (struct Process *p);
  // Remove memory used by thread poll table and iowait, remove from wait queues
  void CleanupThread (struct Thread *thread);
  std::vector<std::string> CopyArgs (char *const argv[]);
  int CopyEnv (char *const envp[], std::vector<std::pair<std::string,std::string> > &res);
  static void* LoadMain (Loader *ld, std::string filename, Process *proc, int &err);
  static void DoExecProcess (void *c);
  static void SetDefaultSigHandler (std::vector<SignalHandler> &signalHandlers);

  std::map<uint16_t, Process *> m_processes; // Key is the pid
  uint16_t m_nextPid;
  TracedCallback<uint16_t, int> m_processExit;
  // If true close stderr and stdout between writes .
  bool m_minimizeFiles;
  std::string m_virtualPath;
  std::string m_release;  //!< Returned by `uname -r`
  std::string m_version;  //!< Returned by `uname -v`
};

} // namespace ns3

#endif /* DCE_MANAGER_H */
