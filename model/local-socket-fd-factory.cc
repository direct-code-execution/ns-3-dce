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
#include "local-socket-fd-factory.h"
#include "local-socket-fd.h"
#include "local-stream-socket-fd.h"
#include "local-datagram-socket-fd.h"
#include "unix-fd.h"
#include "ns3/log.h"
#include <map>

NS_LOG_COMPONENT_DEFINE ("LocalSocketFdFactory");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LocalSocketFdFactory);

TypeId
LocalSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LocalSocketFdFactory").SetParent<Object> ()
    .AddConstructor<LocalSocketFdFactory> ();

  return tid;
}
TypeId
LocalSocketFdFactory::GetInstanceTypeId (void) const
{
  return LocalSocketFdFactory::GetTypeId ();
}
LocalSocketFdFactory::LocalSocketFdFactory ()
  : m_totalBuffersSize (0)
{
}

LocalSocketFdFactory::~LocalSocketFdFactory ()
{
}

void
LocalSocketFdFactory::DoDispose (void)
{
  m_bindByPath.clear ();
  // chain up
  SocketFdFactory::DoDispose ();
}

UnixFd *
LocalSocketFdFactory::CreateSocket (int domain, int type, int protocol)
{
  NS_ASSERT (domain == AF_UNIX);

  if (SOCK_STREAM == type)
    {
      UnixFd *fd = new LocalStreamSocketFd (this);

      return fd;
    }
  if (SOCK_DGRAM == type)
    {
      UnixFd *fd = new LocalDatagramSocketFd (this);

      return fd;
    }

  return 0;
}

LocalSocketFd*
LocalSocketFdFactory::FindBinder (std::string path, TypeId type) const
{
  BindMap::const_iterator i = m_bindByPath.find (path);
  if (m_bindByPath.end () == i)
    {
      return 0;
    }

  LocalSocketFd* winner = i->second;

  if ((winner != 0) && (winner->GetInstanceTypeId () == type))
    {
      return winner;
    }

  return 0;
}

void
LocalSocketFdFactory::RegisterBinder (std::string path, LocalSocketFd* socket)
{
  m_bindByPath[path] = socket;
}

void
LocalSocketFdFactory::UnRegisterBinder (std::string path)
{
  if (0 == m_bindByPath.count (path))
    {
      return;
    }
  m_bindByPath.erase (path);
}
void
LocalSocketFdFactory::UnlinkNotify (std::string path)
{
  UnRegisterBinder (path);
}
} // namespace ns3
