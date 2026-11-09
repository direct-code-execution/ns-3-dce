/* -*-	Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
#ifndef PROCESS_H
#define PROCESS_H

#include <signal.h>
#include <map>
#include <vector>
#include <unordered_map>
#include <list>
#include <set>
#include <stdio.h>
#include <dirent.h>
#include "ns3/callback.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "unix-fd.h"
#include "ns3/random-variable-stream.h"

class KingsleyAlloc;

extern "C" {
struct SimTask;
}


namespace ns3 {

struct Process;
struct ProgramAllocator
{
  void * (*malloc)(size_t);
  void (*free)(void *);
  void * (*calloc)(size_t, size_t);
  void * (*realloc)(void *, size_t);
  int (*posix_memalign)(void **, size_t, size_t);
  size_t (*malloc_usable_size)(void *);
};
struct Thread;
class DceManager;
class UnixFd;
class Loader;
class Task;
class FileUsage;
class PollTable;

struct Mutex
{
  uint32_t mid; // mutex id
  enum
  {
    NORMAL,
    RECURSIVE
  } type;
  uint32_t count;
  std::list<Thread *> waiting;
  Thread *current;
};
struct Semaphore
{
  uint32_t sid; // semaphore id
  uint32_t count;
  std::list<Thread *> waiting;
};
struct Condition
{
  uint32_t cid; // condition var id
  std::list<Thread *> waiting;
};
struct SignalHandler
{
  int signal;
  int flags;
  sigset_t mask;
  void (*handler)(int);
  void (*sigaction)(int, siginfo_t *, void *);
};
enum ThreadState_e
{
  // the thread is executing
  THREAD_RUNNING,
  // the thread is in the run queue, not executing yet
  THREAD_ACTIVE,
  // the thread is blocked, is not in the run queue, is not executing
  THREAD_BLOCKED,
  // the thread is dead: it will never move back to any other state
  // and will be deleted soon.
  THREAD_DEAD
};

struct AtExitHandler
{
  enum
  {
    NORMAL,
    CXA
  } type;
  union
  {
    void (*normal)(void);
    struct
    {
      void (*fn)(void *);
      void *arg;
      void *d;
    } cxa;
  } value;
};

struct ProcessActivity
{
  int64_t ns3Start;
  time_t realStart;
  int64_t ns3End;
  time_t realEnd;
  int exitValue;
  std::string cmdLine;
};

struct Process
{
  uid_t euid;
  uid_t ruid;
  uid_t suid;
  gid_t egid;
  gid_t rgid;
  gid_t sgid;
  uint16_t ppid;
  uint16_t pid;
  uint16_t pgid;
  std::string name;
  std::string stdinFilename;
  // Key is the fd
  std::map<int,FileUsage *> openFiles;
  std::vector<FILE *> openStreams;
  // descriptor behind each stream of openStreams (fopencookie streams have no fileno)
  std::map<FILE *, int> streamFds;
  std::vector<DIR *> openDirs;
  std::vector<SignalHandler> signalHandlers;
  std::vector<Thread *> threads;
  std::vector<Mutex *> mutexes;
  // the same, by id: programs such as Chromium create tens of thousands
  std::unordered_map<uint32_t, Mutex *> mutexById;
  std::vector<Semaphore *> semaphores;
  std::vector<Condition *> conditions;
  std::unordered_map<uint32_t, Condition *> conditionById;
  std::vector<struct AtExitHandler> atExitHandlers;
  std::set<uint16_t> children;
  bool getoptStarted; // getopt has been called by this process
  sigset_t pendingSignals;
  Time itimerInterval;
  EventId itimer;
  uint32_t nextMid;
  uint32_t nextSid;
  uint32_t nextCid;
  pthread_key_t nextThreadKey;
  DceManager *manager;
  Loader *loader;
  void *mainHandle;
  // what /proc/self/exe links to: the program, in the node's file system
  // if it was found there, else on the host (DCE_PATH)
  std::string exePath;
  // what dlerror() reports, and whether it has to
  std::string dlError;
  bool hasDlError;
  // a signal is ending it (dce_exit_signal)
  bool killedBySignal;
  // the initial value of the thread local variables of a program loaded
  // from an executable, below the thread pointer (DceExecutableTlsImage)
  std::vector<uint8_t> executableTls;
  // the program's own malloc() and friends (Chrome's PartitionAlloc), which
  // the libraries it uses must use too, as they would without DCE
  struct ProgramAllocator *programAllocator;
  std::string cwd;
  KingsleyAlloc *alloc;
  Callback<void,uint16_t,int> finished;
  // the values specified by the user
  char **originalEnvp;
  char **originalArgv;
  int originalArgc;
  char *originalProgname; // some programs use it instead argv[0]

  // pointers to the global variables present in the libc loader
  // in the corresponding process.
  FILE **pstdin;
  FILE **pstdout;
  FILE **pstderr;
  char ***penvp;
  char **poptarg;
  int *poptind;
  int *popterr;
  int *poptopt;
  FILE *syslog; // instead of real syslog, everything is written to a file /var/log/<pid>/syslog
  struct tm struct_tm;
  char asctime_result[ 3 + 1 + 3 + 1 + 20 + 1 + 20 + 1 + 20 + 1 + 20 + 1 + 20 + 1 + 1]; // definition is stolen from glibc
  uint32_t nodeId; // NS3 NODE ID
  uint8_t minimizeFiles; // If true close stderr and stdout between writes .
  // an array of memory buffers which must be freed upon process
  // termination to avoid memory leaks. We stick in there a bunch
  // of buffers we allocate but for which we cannot control the
  // lifetime due to weirdness in the posix API.
  std::vector<void *> allocated;
  //random variable for rand and random implementation
  Ptr<RandomVariableStream> rndVariable;
  // srand48 seed
  struct drand48_data seed48Current;
  // Current umask
  mode_t uMask;
  struct ProcessActivity timing;
};

struct ThreadKeyValue
{
  pthread_key_t key;
  void (*destructor)(void*);
  void *value;
};

struct Thread
{
  /* true: this thread has been detached with pthread_detach. */
  bool isDetached;
  /* true: exitValue field is valid */
  bool hasExitValue;
  /* value passed to pthread_exit or returned from thread function */
  void *exitValue;
  /* (h_)errno of the thread. */
  int err;
  int h_err;
  /* thread id. */
  uint16_t tid;
  // mutexes unlocked without letting other threads run (dce-pthread-mutex.cc)
  uint32_t unlocksWithoutYield;
  Task *task;
  Thread *joinWaiter;
  Process *process;
  std::list<struct ThreadKeyValue> keyValues;
  sigset_t signalMask;
  sigset_t pendingSignals;
  sigset_t sigwaitSet; // Signals this thread waits for in sigwait, empty otherwise
  Time lastTime; // Last time of a possible infinite loop checkpoint.
  Waiter *childWaiter; // Not zero if thread waiting for a child in wait or waitall ...
  PollTable *pollTable; // No 0 if a poll is running on this thread
  std::pair <UnixFd*, WaitQueueEntry*> ioWait;   // Filled if the current thread is currently waiting for IO
};

} // namespace ns3

#endif /* PROCESS_H */
