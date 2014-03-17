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
#ifndef LOCAL_SOCKET_FD_H
#define LOCAL_SOCKET_FD_H

#include "unix-fd.h"
#include "ns3/ptr.h"
#include <list>
#include "ns3/nstime.h"
#include "ns3/object.h"

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

class LocalSocketFd : public UnixFd
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  LocalSocketFd ();
  virtual ~LocalSocketFd ();

  virtual int Close (void) = 0;
  virtual ssize_t Write (const void *buf, size_t count) = 0;
  virtual ssize_t Read (void *buf, size_t count) = 0;
  virtual ssize_t Recvmsg (struct msghdr *msg, int flags) = 0;
  virtual ssize_t Sendmsg (const struct msghdr *msg, int flags) = 0;
  virtual bool Isatty (void) const;
  virtual int Setsockopt (int level, int optname,
                          const void *optval, socklen_t optlen) = 0;
  virtual int Getsockopt (int level, int optname,
                          void *optval, socklen_t *optlen) = 0;
  virtual int Getsockname (struct sockaddr *name, socklen_t *namelen) = 0;
  virtual int Getpeername (struct sockaddr *name, socklen_t *namelen) = 0;
  virtual int Ioctl (unsigned long request, char *argp);
  virtual int Bind (const struct sockaddr *my_addr, socklen_t addrlen) = 0;
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen) = 0;
  virtual int Listen (int backlog) = 0;
  virtual int Shutdown (int how) = 0;
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen) = 0;
  virtual void * Mmap (void *start, size_t length, int prot, int flags, off64_t offset);
  virtual off64_t Lseek (off64_t offset, int whence);
  virtual int Fxstat (int ver, struct ::stat *buf);
  virtual int Fxstat64 (int ver, struct ::stat64 *buf);
  virtual int Fcntl (int cmd, unsigned long arg);
  virtual int Settime (int flags,
                       const struct itimerspec *new_value,
                       struct itimerspec *old_value);
  virtual int Gettime (struct itimerspec *cur_value) const;
  virtual int Ftruncate (off_t length);
  virtual int Fsync (void);

  virtual bool CanRecv (void) const = 0;
  virtual bool CanSend (void) const = 0;
  virtual bool HangupReceived (void) const = 0;

protected:
  void ClearReadBuffer (void);
  virtual void ClearAll (bool andWakeUp) = 0;
  virtual bool IsClosed (void) const = 0;

  Time GetRecvTimeout (void);
  Time GetSendTimeout (void);
  // Return :
  //   the size readed ,
  //  or 0 if no more space available,
  //  or -1 if fatal error occurs
  //  or -2 if read part is shuted down
  ssize_t DoRecvPacket (uint8_t* buf, size_t len);

  size_t ReadData (uint8_t* buf, size_t len, bool peek);

  struct Buffer
  {
    uint8_t *data;
    size_t size;
    size_t readOffset;
  };
  typedef std::list<struct Buffer*> FifoData;

  FifoData m_readBuffer;
  size_t m_readBufferSize;

  Time m_sendTimeout;
  Time m_recvTimeout;
  Ptr<LocalSocketFdFactory> m_factory;
  int32_t m_linger;
  std::string m_bindPath;
  std::string m_connectPath;
  bool m_shutRead;
  bool m_shutWrite;
};

} // namespace ns3

#endif /* LOCAL_SOCKET_FD_H */
