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
#ifndef LOCAL_SOCKET_FD_FACTORY_H
#define LOCAL_SOCKET_FD_FACTORY_H

#include "socket-fd-factory.h"
#include "ns3/ptr.h"
#include <map>
#include <sys/types.h>
#include "ns3/object.h"

namespace ns3 {

class LocalSocketFd;
class LocalStreamSocketFd;

class LocalSocketFdFactory : public SocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  LocalSocketFdFactory ();
  virtual void DoDispose ();
  virtual ~LocalSocketFdFactory ();
  virtual UnixFd * CreateSocket (int domain, int type, int protocol);
  virtual void UnlinkNotify (std::string path);
  LocalSocketFd* FindBinder (std::string path, TypeId type) const;
  void RegisterBinder (std::string path, LocalSocketFd* socket);
  void UnRegisterBinder (std::string path);

private:
  typedef std::map<std::string, LocalSocketFd*> BindMap;
  BindMap m_bindByPath;
  size_t m_totalBuffersSize;
};

} // namespace ns3

#endif /* LOCAL_SOCKET_FD_FACTORY_H */
