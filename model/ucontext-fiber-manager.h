#ifndef UCONTEXT_FIBER_MANAGER_H
#define UCONTEXT_FIBER_MANAGER_H

#include "fiber-manager.h"
#include <signal.h>
#include <list>

namespace ns3 {

class UcontextFiberManager : public FiberManager
{
public:
  UcontextFiberManager ();
  virtual ~UcontextFiberManager ();

  virtual struct Fiber *Create (void (*callback)(void *),
                                void *context,
                                uint32_t stackSize);
  virtual struct Fiber * CreateFromCaller (void);
  virtual void Delete (struct Fiber *fiber);
  virtual void SwitchTo (struct Fiber *from,
                         const struct Fiber *to);
  virtual uint32_t GetStackSize (struct Fiber *fiber) const;
  virtual void SetSwitchNotification (void (*fn)(void));
private:
  static void SegfaultHandler (int sig, siginfo_t *si, void *unused);
  // invoked as atexit handler
  static void FreeAlternateSignalStack (void);

  void SetupSignalHandler (void);
  uint32_t CalcStackSize (uint32_t size);
  uint8_t * AllocateStack (uint32_t stackSize);
  void DeallocateStack (uint8_t *buffer, uint32_t stackSize);
  static void Trampoline (int a0, int a1, int a2, int a3);


  void (*m_notifySwitch)(void);
  static void *g_alternateSignalStack;
  static std::list<unsigned long> g_guardPages;
};

} // namespace ns3

#endif /* UCONTEXT_FIBER_MANAGER_H */
