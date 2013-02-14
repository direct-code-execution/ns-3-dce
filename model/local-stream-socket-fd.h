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
#ifndef LOCAL_STREAM_SOCKET_FD_H
#define LOCAL_STREAM_SOCKET_FD_H

#include "unix-fd.h"
#include "ns3/ptr.h"
#include <list>
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "local-socket-fd.h"
#include "wait-queue.h"

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
class WaitQueueEntryTimeout;

#define LOCAL_SOCKET_MAX_BUFFER 1024 * 128

class LocalStreamSocketFd : public LocalSocketFd
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  LocalStreamSocketFd (Ptr<LocalSocketFdFactory> f);
  LocalStreamSocketFd (LocalStreamSocketFd *peer, std::string connectPath);

  virtual ~LocalStreamSocketFd ();

  virtual void DoDispose (void);

  virtual int Close (void);
  virtual ssize_t Write (const void *buf, size_t count);
  virtual ssize_t Read (void *buf, size_t count);
  virtual ssize_t Recvmsg (struct msghdr *msg, int flags);
  virtual ssize_t Sendmsg (const struct msghdr *msg, int flags);
  virtual bool Isatty (void) const;
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
  bool InternalConnect (void);
  bool IsAccepting (void);
  bool IsListening (void);
  void ConnectionCreated (LocalStreamSocketFd* sock, WaitQueueEntryTimeout *wq);
  void RemoveFromQueue (LocalStreamSocketFd* sock);
  void SetPeer (LocalStreamSocketFd *sock);
  void PeerClosed (void);
  bool IsShutWrite (void) const;

  enum State
  {
    CREATED,
    BINDED,
    LISTENING,
    ACCEPTING,
    CONNECTING,
    CONNECTED,
    REMOTECLOSED,
    CLOSED
  };
  typedef std::list<LocalStreamSocketFd*> FifoCnx;

  enum State m_state;
  FifoCnx m_cnxQueue;
  int m_backLog;
  LocalStreamSocketFd *m_peer;

};

} // namespace ns3

#endif /* LOCAL_STREAM_SOCKET_FD_H */
