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
#ifndef IPV4_LINUX_ADDRESS_HELPER_H
#define IPV4_LINUX_ADDRESS_HELPER_H

#include <list>

namespace ns3 {

class Ipv4Address;
class Ipv4Mask;
class NetDeviceContainer;

/**
 * @brief A helper class to make life easier while doing simple IPv4 address
 * assignment in scripts. Similar to Ipv4AddressHelper.
 *
 * @see Ipv4AddressHelper
 */
class Ipv4LinuxAddressHelper
{
public:
/**
 * @brief Construct a helper class to make life easier while doing simple IPv4
 * address assignment in scripts.
 */
  Ipv4LinuxAddressHelper ();
/**
 * @brief Construct a helper class to make life easier while doing simple IPv4
 * address assignment in scripts.  This version sets the base and mask
 * in the constructor
 */
  Ipv4LinuxAddressHelper (Ipv4Address network, Ipv4Mask mask, Ipv4Address base = "0.0.0.1");
  void SetBase (Ipv4Address network, Ipv4Mask mask, Ipv4Address base = "0.0.0.1");
  void Assign (const NetDeviceContainer &c);
  Ipv4Address NewNetwork (void);


private:
  /**
   * @internal
   */
  uint32_t NumAddressBits (uint32_t maskbits) const;
  Ipv4Address NewAddress (void);

  uint32_t m_network;
  uint32_t m_mask;
  uint32_t m_address;
  uint32_t m_base;
  uint32_t m_shift;
  uint32_t m_max;
};
}


#endif // IPV4_LINUX_ADDRESS_HELPER_H
