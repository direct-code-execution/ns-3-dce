/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 INRIA
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
 * Author: Frédéric Urbani
 */
#ifndef IPV4_LINUX_ADDRESS_H
#define IPV4_LINUX_ADDRESS_H

#include <list>

namespace ns3 {

class Ipv4Address;
class Ipv4Mask;

class Ipv4LinuxAddress
{
public:
  Ipv4LinuxAddress (uint32_t n, uint32_t d, Ipv4Address a, Ipv4Mask m, bool p2p);
  void Install (Ptr<Node> node);
  static void RunIp (Ptr<Node> node, Time at, std::string str);
  static void AddAddress (Ptr<Node> node, Time at, std::string name, std::string address);
  static std::string Ipv4AddressToString (Ipv4Address ad);

private:
 uint32_t m_nodeid;
 uint32_t m_deviceid;
 Ipv4Address m_address;
 Ipv4Mask m_mask;
 bool m_deviceIsPoint2point;
};

class Ipv4LinuxAddressContainer : public Object
{
public:
  static TypeId GetTypeId (void);
  Ipv4LinuxAddressContainer (uint32_t n);

  void AddAddress (uint32_t d, Ipv4Address a, Ipv4Mask m, bool p2p);
  void Install (Ptr<Node> node);

private:
  uint32_t m_nodeid;
  std::list <Ipv4LinuxAddress> m_list;
};

}
#endif // IPV4_LINUX_ADDRESS_H
