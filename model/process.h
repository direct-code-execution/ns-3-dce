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
#include <vector>
#include <list>
#include <stdio.h>
#include "ns3/callback.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "unix-fd.h"
#include "ns3/random-variable.h"

class KingsleyAlloc;

extern "C" {
struct SimTask;
}


namespace ns3 {

struct Process;
struct Thread;
class DceManager;
class UnixFd;
class Loader;
class Task;

struct Mutex
{
  uint32_t mid; // mutex id
  enum {
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
  void (*handler) (int);
  void (*sigaction) (int, siginfo_t *, void *);
};
enum ThreadState_e {
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
  enum {
    NORMAL,
    CXA
  } type;
  union {
    void (*normal) (void);
    struct {
      void (*fn) (void *);
      void *arg;
      void *d;
    } cxa;
  } value;
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
  std::string name;
  std::string stdinFilename;
  std::vector<std::pair<int,UnixFd *> > openFiles;
  std::vector<FILE *> openStreams;
  std::vector<SignalHandler> signalHandlers;
  std::vector<Thread *> threads;
  std::vector<Mutex *> mutexes;
  std::vector<Semaphore *> semaphores;
  std::vector<Condition *> conditions;
  std::vector<struct AtExitHandler> atExitHandlers;
  sigset_t pendingSignals;
  Time itimerInterval;
  EventId itimer;
  uint32_t nextMid;
  uint32_t nextSid;
  uint32_t nextCid;
  pthread_key_t nextThreadKey;
  int exitValue;
  DceManager *manager;
  Loader *loader;
  void *mainHandle;
  std::string cwd;
  KingsleyAlloc *alloc;
  Callback<void,uint16_t,int> finished;
  // the values specified by the user
  char **originalEnvp;
  char **originalArgv;
  int originalArgc;
  // pointers to the global variables present in the libc loader 
  // in the corresponding process.
  FILE **pstdin;
  FILE **pstdout;
  FILE **pstderr;
  char ***penvp;
  // an array of memory buffers which must be freed upon process 
  // termination to avoid memory leaks. We stick in there a bunch
  // of buffers we allocate but for which we cannot control the 
  // lifetime due to weirdness in the posix API.
  std::vector<void *> allocated;
  //random variable for rand and random implementation
  RandomVariable rndVarible;
  // srand48 seed
  struct drand48_data seed48Current;
  // Current umask
  mode_t hurd_mask;
};

struct ThreadKeyValue
{
  pthread_key_t key;
  void (*destructor) (void*);
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
  /* errno of the thread. */
  int err;
  /* thread id. */
  uint16_t tid;
  Task *task;
  Thread *joinWaiter;
  Process *process;
  std::list<struct ThreadKeyValue> keyValues;
  sigset_t signalMask;
  sigset_t pendingSignals;
  Time lastTime; // Last time of a possible infinite loop checkpoint.
};

} // namespace ns3

#endif /* PROCESS_H */
