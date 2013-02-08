#ifndef PTHREAD_FIBER_MANAGER_H
#define PTHREAD_FIBER_MANAGER_H

#include "fiber-manager.h"
#include <signal.h>
#include <list>

namespace ns3 {

struct PthreadFiber;
class StackTrampoline;

class PthreadFiberManager : public FiberManager
{
public:
  PthreadFiberManager ();
  virtual ~PthreadFiberManager ();
  virtual struct Fiber * Clone (struct Fiber *fiber);
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
  static void * Run (void *arg);
  void Yield (struct PthreadFiber *fiber);
  void Wakeup (struct PthreadFiber *fiber);
  void Start (struct PthreadFiber *fiber);
  static void * SelfStackBottom (void);
  void RestoreFiber (struct PthreadFiber *fiber);
  void (*m_notifySwitch)(void);
  StackTrampoline *m_trampoline;
};

} // namespace ns3

#endif /* PTHREAD_FIBER_MANAGER_H */
