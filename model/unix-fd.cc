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
    m_sendWaiter (0)
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

} // namespace ns3
