/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
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
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 */
#ifndef LOCAL_DATAGRAM_SOCKET_FD_H
#define LOCAL_DATAGRAM_SOCKET_FD_H

#include "unix-fd.h"
#include "ns3/ptr.h"
#include <set>
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "local-socket-fd.h"

namespace ns3 {

class Waiter;

/**
* \brief A LocalSocket is  used  to transfer data between processes in same node.
*
* it is used to emulate AF_UNIX sockets.
* The emulation supports only data transfert and not fd transfert and not security transfert.
*
*/

class LocalSocketFdFactory;

#define LOCAL_SOCKET_MAX_BUFFER 1024 * 128

class LocalDatagramSocketFd : public LocalSocketFd
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  LocalDatagramSocketFd (Ptr<LocalSocketFdFactory> f);
  virtual ~LocalDatagramSocketFd ();

  virtual void DoDispose (void);
  virtual int Close (void);
  virtual ssize_t Write (const void *buf, size_t count);
  virtual ssize_t Read (void *buf, size_t count);
  virtual ssize_t Recvmsg (struct msghdr *msg, int flags);
  virtual ssize_t Sendmsg (const struct msghdr *msg, int flags);

  virtual int Setsockopt (int level, int optname,
                          const void *optval, socklen_t optlen);
  virtual int Getsockopt (int level, int optname,
                          void *optval, socklen_t *optlen);
  virtual int Getsockname (struct sockaddr *name, socklen_t *namelen);
  virtual int Getpeername (struct sockaddr *name, socklen_t *namelen);
  virtual int Bind (const struct sockaddr *my_addr, socklen_t addrlen);
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen);
  virtual int Listen (int backlog);
  virtual int Shutdown (int how);
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen);

  virtual bool CanRecv (void) const;
  virtual bool CanSend (void) const;
  virtual bool HangupReceived (void) const;
  virtual int Poll (PollTable* ptable);

protected:
  virtual void ClearAll (bool andWakeUp);
  virtual bool IsClosed (void) const;

private:
  bool IsBinded (void);
  void UnBind (void);
  ssize_t Read (void *buf, size_t count, bool noWait, bool peek);
  void AddConnected (LocalDatagramSocketFd *newone);
  void RemoveConnected (LocalDatagramSocketFd *freeOne, bool andWakeUp);

  ssize_t Write2Peer (const void *buf, size_t count, LocalDatagramSocketFd* peer);

  enum State
  {
    CREATED,
    BINDED,
    CONNECTED,
    REMOTECLOSED,
    CLOSED
  };
  enum State m_state;

  // Because of the implementation of
  // Filled when CONNECTED
  LocalDatagramSocketFd* m_peer;
  // When I am BINDED I could have much peers.
  std::set<LocalDatagramSocketFd*> m_myPeers;
};

} // namespace ns3

#endif /* LOCAL_SOCKET_FD_H */
