#include "socket-fd-factory.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SocketFdFactory);

TypeId
SocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SocketFdFactory")
    .SetParent<Object> ()
  ;
  return tid;
}

SocketFdFactory::~SocketFdFactory ()
{
}

void
SocketFdFactory::UnlinkNotify (std::string path)
{
}

} // namespace ns3
