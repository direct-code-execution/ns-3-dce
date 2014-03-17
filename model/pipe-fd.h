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
#ifndef PIPE_FD_H
#define PIPE_FD_H

#include "unix-fd.h"
#include "fifo-buffer.h"
namespace ns3 {
/**
*
* \brief PipeFd simulate pipe file descriptors created by posix pipe method.
*
* Two PipeFd are linked together in order to create the pipe system call emulation.
*
*/

class LocalSocketFdFactory;


#define LOCAL_SOCKET_MAX_BUFFER 1024 * 128

class PipeFd : public UnixFd
{
public:
  PipeFd ();
  PipeFd (PipeFd *peer);

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
  virtual int Ioctl (unsigned long request, char *argp);
  virtual int Bind (const struct sockaddr *myaddr, socklen_t addrlen);
  virtual int Connect (const struct sockaddr *my_addr, socklen_t addrlen);
  virtual int Listen (int backlog);
  virtual int Shutdown (int how);
  virtual int Accept (struct sockaddr *my_addr, socklen_t *addrlen);
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
  virtual bool Isatty (void) const;

  // Return true if a select on this fd should return POLLHUP
  virtual bool HangupReceived (void) const;

  virtual int Poll (PollTable* ptable);
  virtual int Fsync (void);

private:
  ssize_t DoRecvPacket (uint8_t* buf, size_t len);

  PipeFd* m_peer;
  bool m_readSide;
  int m_statusFlags;
  FifoBuffer m_buf;

};
} // namespace ns3

#endif /* PIPE_FD_H */
