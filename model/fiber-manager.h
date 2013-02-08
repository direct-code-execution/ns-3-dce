#ifndef FIBER_MANAGER_H
#define FIBER_MANAGER_H

#include <stdint.h>

namespace ns3 {

struct Fiber
{};

class FiberManager
{
public:
  virtual ~FiberManager ();

  virtual struct Fiber * Clone (struct Fiber *fiber)
  {
    return 0;
  }

  /**
   * \param callback function to use as main loop for the
   *        newly-created fiber
   * \param stackSize size of the stack to allocate for this
   *        fiber.
   * \returns a newly-created fiber context.
   */
  virtual struct Fiber *Create (void (*callback)(void *),
                                void *context,
                                uint32_t stackSize) = 0;
  /**
   * \returns an empty context
   *
   * Create a fiber context which can be used as a from
   * argument to the SwitchTo method
   * This method is usually used to create a context which
   * references the main 'normal' thread. i.e., the thread
   * initially managed by the underlying kernel which
   * runs on the kernel-managed stack.
   */
  virtual struct Fiber * CreateFromCaller (void) = 0;

  /**
   * \param context to delete
   *
   * Release any ressource associated to this context.
   * Obviously, this method must be called from another
   * context than the one which is being deleted.
   */
  virtual void Delete (struct Fiber *fiber) = 0;

  /**
   * \param from from context
   * \param to to context
   *
   * This function saves the caller's context into the from
   * context and restores the current context from the to
   * context. The first time this function is called, it is
   * usually called with a from set to a context obtained
   * through CreateFromCaller and with a to set to a context
   * obtained through Create.
   */
  virtual void SwitchTo (struct Fiber *from,
                         const struct Fiber *to) = 0;

  /**
   * \return size of stack allocated in fiber.
   */
  virtual uint32_t GetStackSize (struct Fiber *fiber) const = 0;

  /**
   * \param fn a function which will be invoked whenever SwitchTo
   * is invoked, just before it returns to the destination fiber.
   */
  virtual void SetSwitchNotification (void (*fn)(void)) = 0;
};

} // namespace ns3

#endif /* FIBER_MANAGER_H */
