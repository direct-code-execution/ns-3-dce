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

LinuxSocketFdFactory::LinuxSocketFdFactory ()
{
  // The defaults of the libos stack, which scripts written for it rely on:
  // applied when the kernel boots, before the scripts' own sysctls.
  Set (".net.ipv4.conf.all.forwarding", "1");
}

} // namespace ns3
