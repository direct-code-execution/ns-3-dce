/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LKL_KERNEL_H
#define LKL_KERNEL_H

#include "ns3/object.h"
#include <map>
#include <set>
#include <string>

struct lkl_host_operations;

namespace ns3 {

class TaskManager;
class Task;

/**
 * A Linux kernel (LKL, the Linux Kernel Library) for one node.
 *
 * Each node loads its own copy of liblkl.so (a separate file, so that the
 * dynamic linker gives it separate global data) and runs it on DCE tasks:
 * the LKL host operations map threads to tasks, semaphores and mutexes to
 * task wait lists, the clock to the simulation clock and the one-shot timer
 * to simulator events. The kernel therefore only runs when a
 * task or an event drives it, and an idle kernel lets the simulator jump to
 * the next event.
 */
class LklKernel : public Object
{
public:
  static TypeId GetTypeId (void);
  LklKernel ();
  virtual ~LklKernel ();

  /**
   * Boot the kernel. Must be called from a DCE task, as booting blocks
   * until the kernel init thread has started.
   */
  void Boot (void);
  bool IsBooted (void) const;

  /** Issue a kernel system call (LKL syscall number, parameters). */
  long Syscall (long no, long *params);

  /** Look up a symbol of this node's copy of the kernel library. */
  void * Lookup (const char *symbol) const;

  /** The kernel of the node running the current task. */
  static LklKernel *Current (void);

  // Used by the host operations.
  TaskManager *GetTaskManager (void) const;
  uint32_t GetNodeId (void) const;
  /** The thread-local storage keys the kernel allocated. */
  std::set<void *> &GetTlsKeys (void);
  /** The memory the kernel allocated with page_alloc. */
  std::map<void *, unsigned long> &GetPages (void);

private:
  virtual void DoDispose (void);
  virtual void NotifyNewAggregate (void);
  void Load (void);
  void InstallHostOps (struct lkl_host_operations *ops);

  std::string m_library;
  std::string m_cmdline;
  TaskManager *m_manager;
  void *m_handle;
  int (*m_init)(struct lkl_host_operations *ops);
  int (*m_start)(const char *cmdline, ...);
  static void MainSyscallTrampoline (void *context);
  static void TaskEnd (Task *task, bool running, void *context);
  static void TlsDestructorsTrampoline (void *context);
  long (*m_syscall)(long no, long *params);
  bool m_booted;
  std::set<void *> m_tlsKeys;
  std::map<void *, unsigned long> m_pages;
};

} // namespace ns3

#endif /* LKL_KERNEL_H */
