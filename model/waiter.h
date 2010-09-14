#ifndef WAITER_H
#define WAITER_H

#include "ns3/nstime.h"
#include "ns3/callback.h"
#include "ns3/event-id.h"

namespace ns3 {

struct Thread;

class Waiter
{
public:
  typedef enum
  {
    INTERRUPTED,
    TIMEOUT,
    OK
  } Result;

  Waiter ();
  ~Waiter ();

  void SetTimeout (Time timeout);
  Time GetTimeoutLeft (void) const;
  // wait for a call to Wakeup, return OK if Wakeup is called
  // and no signal is pending.
  // return TIMEOUT if timeout expired
  // return INTERRUPTED if signal pending.
  Waiter::Result Wait (void);
  // wait for a call to Wakeup, handle signals and stop timeout 
  // if any are pending. Return true only if Wakeup is called
  // and no signal is pending.
  bool WaitDoSignal (void);

  // wakeup someone waiting on Wait.
  void Wakeup (void);
private:

  Thread *m_waiting;
  Time m_timeout;
};

} // namespace ns3

#endif /* WAITER_H */
