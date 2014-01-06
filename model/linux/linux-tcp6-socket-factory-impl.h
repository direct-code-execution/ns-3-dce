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
#ifndef LINUX_TCP6_SOCKET_FACTORY_IMPL_H
#define LINUX_TCP6_SOCKET_FACTORY_IMPL_H

#include "linux-tcp6-socket-factory.h"
#include "ns3/ptr.h"
#include "ns3/node.h"

namespace ns3 {

/**
 * \ingroup tcp
 * \brief Object to create TCP socket instances
 * \internal
 *
 * This class implements the API for creating TCP sockets.
 * It is a socket factory (deriving from class SocketFactory).
 */
class LinuxTcp6SocketFactoryImpl : public LinuxTcp6SocketFactory
{
public:
  /**
   * \brief Implements a method to create a Tcp-based socket and return
   * a base class smart pointer to the socket.
   * \internal
   *
   * \return smart pointer to Socket
   */
  virtual Ptr<Socket> CreateSocket (void);

protected:
private:
};

} // namespace ns3

#endif /* LINUX_TCP6_SOCKET_FACTORY_IMPL_H */
