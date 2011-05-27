#include "unix-fd.h"
#include "waiter.h"

namespace ns3 {

TypeId
UnixFd::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UnixFd")
    .SetParent<Object> ()
    ;
  return tid;
}

UnixFd::UnixFd ()
  : m_recvWaiter (0),
    m_sendWaiter (0),
    m_refCount (1)
{}

void 
UnixFd::SetRecvWaiter (Waiter *waiter)
{
  m_recvWaiter = waiter;
}
void 
UnixFd::SetSendWaiter (Waiter *waiter)
{
  m_sendWaiter = waiter;
}

void 
UnixFd::WakeupSend (void)
{
  if (m_sendWaiter != 0)
    {
      m_sendWaiter->Wakeup ();
    }
}
void 
UnixFd::WakeupRecv (void)
{
  if (m_recvWaiter != 0)
    {
      m_recvWaiter->Wakeup ();
    }
}
// File Descriptor Reference count
void
UnixFd::IncRef (void)
{
  m_refCount++;
}
void
UnixFd::DecRef (void)
{
  m_refCount--;
}
int32_t
UnixFd::GetRef (void) const
{
  return m_refCount;
}
} // namespace ns3
