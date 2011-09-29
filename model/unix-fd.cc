#include "unix-fd.h"
#include "waiter.h"
#include "ns3/log.h"
#include "process.h"
#include "utils.h"

NS_LOG_COMPONENT_DEFINE ("UnixFd");

namespace ns3 {

TypeId
UnixFd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UnixFd")
    .SetParent<Object> ()
    ;
  return tid;
}

UnixFd::UnixFd () : m_fdCount (0)
{}
void
UnixFd::RemoveWaitQueue (WaitQueueEntry* old, bool andUnregister)
{
  m_waitQueueList.remove (old);
  if (andUnregister)
    {
      Current ()->ioWait = std::make_pair ((UnixFd*)0,(WaitQueueEntry*)0);
    }
}
void
UnixFd::AddWaitQueue (WaitQueueEntry* newOne, bool andRegister)
{
  m_waitQueueList.push_back (newOne);
  if (andRegister)
    {
      Current ()->ioWait = std::make_pair (this, newOne);
    }
}
void
UnixFd::WakeWaiters (void* key)
{
  for (std::list<WaitQueueEntry*>::iterator i = m_waitQueueList.begin ();
      i != m_waitQueueList.end (); ++i)
    {
      (*i)->WakeUp (key);
    }
}
void
UnixFd::IncFdCount (void)
{
  m_fdCount++;
}
void
UnixFd::DecFdCount (void)
{
  m_fdCount--;
}
int
UnixFd::GetFdCount (void) const
{
  return m_fdCount;
}

} // namespace ns3
