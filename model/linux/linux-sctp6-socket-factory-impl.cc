/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Hajime Tazaki
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "linux-socket-impl.h"
#include "linux-sctp6-socket-factory-impl.h"
#include <netinet/in.h>
#include <arpa/inet.h>

NS_LOG_COMPONENT_DEFINE ("DceLinuxSctp6SocketFactoryImpl");

namespace ns3 {

Ptr<Socket>
LinuxSctp6SocketFactoryImpl::CreateSocket (void)
{
#ifdef KERNEL_STACK
  Ptr<LinuxSocketImpl> socket = CreateObject<LinuxSocketImpl> ();
  Ptr<Node> node = this->GetObject<Node> ();
  socket->SetNode (node);
  socket->SetAttribute ("Family", UintegerValue (AF_INET6));
  socket->SetAttribute ("SockType", UintegerValue (SOCK_STREAM));
  socket->SetAttribute ("Protocol", UintegerValue (IPPROTO_SCTP));
  socket->CreateSocket ();
  return socket;
#endif
}


} // namespace ns3
