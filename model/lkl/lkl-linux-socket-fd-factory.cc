/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "linux-socket-fd-factory.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LinuxSocketFdFactory);

TypeId
LinuxSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinuxSocketFdFactory")
    .SetParent<LklSocketFdFactory> ()
    .AddConstructor<LinuxSocketFdFactory> ()
  ;
  return tid;
}

} // namespace ns3
