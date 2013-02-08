#include "unix-fd.h"
#include "waiter.h"
#include "ns3/log.h"
#include "process.h"
#include "utils.h"
#include <fcntl.h>
#include <errno.h>

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

UnixFd::UnixFd () : m_fdCount (0),
                    m_fdFlags (0),
                    m_statusFlags (0)
{
}
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
char *
UnixFd::Ttyname (void)
{
  return 0;
}
int
UnixFd::GetRealFd (void) const
{
  return -1;
}
int
UnixFd::Fcntl (int cmd, unsigned long arg)
{
  switch (cmd)
    {
    case F_GETFL:
      return m_statusFlags;
      break;
    case F_SETFL:
      m_statusFlags = arg;
      return 0;
      break;

    case F_GETFD:
      return m_fdFlags;
      break;
    case F_SETFD:
      m_fdFlags = arg;
      return 0;
      break;

    default:
      //XXX commands missing
      NS_FATAL_ERROR ("fcntl not implemented on socket");
      return -1;
    }
}
} // namespace ns3
