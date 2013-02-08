/* -*-	Mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 Liu Jian
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
 * Author: Liu Jian <liujatp@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "netlink-socket-address.h"

namespace ns3 {

NetlinkSocketAddress::NetlinkSocketAddress (uint32_t pid, uint32_t groups)
  : m_pid (pid),
    m_groups (groups)
{
}

NetlinkSocketAddress::NetlinkSocketAddress ()
  : m_pid (0),
    m_groups (0)
{
}

NetlinkSocketAddress::~NetlinkSocketAddress ()
{
}

void NetlinkSocketAddress::SetProcessID (uint32_t pid)
{
  m_pid = pid;
}

void NetlinkSocketAddress::SetGroupsMask (uint32_t mask)
{
  m_groups = mask;
}

uint32_t NetlinkSocketAddress::GetProcessID (void) const
{
  return m_pid;
}

uint32_t NetlinkSocketAddress::GetGroupsMask (void) const
{
  return m_groups;
}

NetlinkSocketAddress::operator Address (void) const
{
  return ConvertTo ();
}

Address NetlinkSocketAddress::ConvertTo (void) const
{
  uint8_t buffer[8];

  buffer[0] = (m_pid >> 24) & 0xff;
  buffer[1] = (m_pid >> 16) & 0xff;
  buffer[2] = (m_pid >> 8) & 0xff;
  buffer[3] = (m_pid >> 0) & 0xff;
  buffer[4] = (m_groups >> 24) & 0xff;
  buffer[5] = (m_groups >> 16) & 0xff;
  buffer[6] = (m_groups >> 8) & 0xff;
  buffer[7] = (m_groups >> 0) & 0xff;

  return Address (GetType (), buffer, 8);
}

NetlinkSocketAddress NetlinkSocketAddress::ConvertFrom (const Address &address)
{
  NS_ASSERT (IsMatchingType (address));

  NetlinkSocketAddress nl;
  uint8_t buf[8];

  address.CopyTo (buf);

  nl.m_pid = 0;
  nl.m_pid |= buf[0];
  nl.m_pid <<= 8;
  nl.m_pid |= buf[1];
  nl.m_pid <<= 8;
  nl.m_pid |= buf[2];
  nl.m_pid <<= 8;
  nl.m_pid |= buf[3];

  nl.m_groups = 0;
  nl.m_groups |= buf[4];
  nl.m_groups <<= 8;
  nl.m_groups |= buf[5];
  nl.m_groups <<= 8;
  nl.m_groups |= buf[6];
  nl.m_groups <<= 8;
  nl.m_groups |= buf[7];

  return nl;
}

bool NetlinkSocketAddress::IsMatchingType (const Address &address)
{
  return address.IsMatchingType (GetType ());
}

uint8_t NetlinkSocketAddress::GetType (void)
{
  static uint8_t type = Address::Register ();
  return type;
}


std::ostream & operator<< (std::ostream &os, const NetlinkSocketAddress &address)
{
  os << "pid: " << address.GetProcessID () << ", group: " << address.GetGroupsMask ();
  return os;
}

} // namespace ns3
