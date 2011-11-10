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

#ifndef NETLINK_SOCKET_ADDRESS_H
#define NETLINK_SOCKET_ADDRESS_H

#include "ns3/ptr.h"
#include "ns3/address.h"

namespace ns3 {

class NetlinkSocketAddress
{
public:
  NetlinkSocketAddress (uint32_t pid, uint32_t groups);
  NetlinkSocketAddress ();
  ~NetlinkSocketAddress ();

  void SetProcessID (uint32_t pid);
  void SetGroupsMask (uint32_t mask);

  uint32_t GetProcessID (void) const;
  uint32_t GetGroupsMask (void) const;

  /**
  * \returns an Address instance which represents this
  * NetlinkSocketAddress instance.
  */
  operator Address (void) const;
  /**
  * \param address the Address instance to convert from.
  *
  * Returns an NetlinkSocketAddress which corresponds to the input
  * Address
  */
  static NetlinkSocketAddress ConvertFrom (const Address &address);
  /**
  * \returns true if the address matches, false otherwise.
  */
  static bool IsMatchingType (const Address &address);
private:
  static uint8_t GetType (void);
  Address ConvertTo (void) const;

  uint32_t m_pid;
  uint32_t m_groups;

};

std::ostream & operator<< (std::ostream &, const NetlinkSocketAddress &);

} // namespace ns3

#endif /* NETLINK_SOCKET_ADDRESS_H */
