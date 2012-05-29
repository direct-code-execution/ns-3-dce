/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 University of Washington
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
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address-generator.h"
#include "ns3/simulator.h"
#include "ns3/net-device-container.h"
#include "ipv4-linux-address.h"
#include "ipv4-linux-address-helper.h"

NS_LOG_COMPONENT_DEFINE ("Ipv4LinuxAddressHelper");

namespace ns3 {

Ipv4LinuxAddressHelper::Ipv4LinuxAddressHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();

//
// Set the default values to an illegal state.  Do this so the client is
// forced to think at least briefly about what addresses get used and what
// is going on here.
//
  m_network = 0xffffffff;
  m_mask = 0;
  m_address = 0xffffffff;
  m_base = 0xffffffff;
  m_shift = 0xffffffff;
  m_max = 0xffffffff;
}

Ipv4LinuxAddressHelper::Ipv4LinuxAddressHelper (
  const Ipv4Address network,
  const Ipv4Mask    mask,
  const Ipv4Address address)
{
  NS_LOG_FUNCTION_NOARGS ();
  SetBase (network, mask, address);
}

void
Ipv4LinuxAddressHelper::SetBase (
  const Ipv4Address network,
  const Ipv4Mask mask,
  const Ipv4Address address)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_network = network.Get ();
  m_mask = mask.Get ();
  m_base = m_address = address.Get ();

//
// Some quick reasonableness testing.
//
  NS_ASSERT_MSG ((m_network & ~m_mask) == 0,
                 "Ipv4AddressHelper::SetBase(): Inconsistent network and mask");

//
// Figure out how much to shift network numbers to get them aligned, and what
// the maximum allowed address is with respect to the current mask.
//
  m_shift = NumAddressBits (m_mask);
  m_max = (1 << m_shift) - 2;

  NS_ASSERT_MSG (m_shift <= 32,
                 "Ipv4AddressHelper::SetBase(): Unreasonable address length");

//
// Shift the network down into the normalized position.
//
  m_network >>= m_shift;

  NS_LOG_LOGIC ("m_network == " << m_network);
  NS_LOG_LOGIC ("m_mask == " << m_mask);
  NS_LOG_LOGIC ("m_address == " << m_address);
}

const uint32_t N_BITS = 32;

uint32_t
Ipv4LinuxAddressHelper::NumAddressBits (uint32_t maskbits) const
{
  NS_LOG_FUNCTION_NOARGS ();
  for (uint32_t i = 0; i < N_BITS; ++i)
    {
      if (maskbits & 1)
        {
          NS_LOG_LOGIC ("NumAddressBits -> " << i);
          return i;
        }
      maskbits >>= 1;
    }

  NS_ASSERT_MSG (false, "Ipv4AddressHelper::NumAddressBits(): Bad Mask");
  return 0;
}

void
Ipv4LinuxAddressHelper::Assign (const NetDeviceContainer &c)
{
  NS_LOG_FUNCTION_NOARGS ();
  for (uint32_t i = 0; i < c.GetN (); ++i) {
      Ptr<NetDevice> device = c.Get (i);

      Ptr<Node> node = device->GetNode ();
      NS_ASSERT_MSG (node, "Ipv4LinuxAddressHelper::Assign(): NetDevice is not not associated "
                     "with any node -> fail");
      Ptr<Ipv4LinuxAddressContainer> zadresses = node->GetObject<Ipv4LinuxAddressContainer> ();
      if (!zadresses)
        {
          zadresses = CreateObject<Ipv4LinuxAddressContainer>(node->GetId ());
          node->AggregateObject (zadresses);
        }
      uint32_t d = device->GetIfIndex ();
      Ipv4Address a = NewAddress ();
      zadresses->AddAddress (d, a, m_mask, device->IsPointToPoint());
    }
}

Ipv4Address
Ipv4LinuxAddressHelper::NewAddress (void)
{
//
// The way this is expected to be used is that an address and network number
// are initialized, and then NewAddress() is called repeatedly to allocate and
// get new addresses on a given subnet.  The client will expect that the first
// address she gets back is the one she used to initialize the generator with.
// This implies that this operation is a post-increment.
//
  NS_ASSERT_MSG (m_address <= m_max,
                 "Ipv4AddressHelper::NewAddress(): Address overflow");

  Ipv4Address addr ((m_network << m_shift) | m_address);
  ++m_address;
//
// The Ipv4AddressGenerator allows us to keep track of the addresses we have
// allocated and will assert if we accidentally generate a duplicate.  This
// avoids some really hard to debug problems.
//
  Ipv4AddressGenerator::AddAllocated (addr);
  return addr;
}

Ipv4Address
Ipv4LinuxAddressHelper::NewNetwork (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  ++m_network;
  m_address = m_base;
  return Ipv4Address (m_network << m_shift);
}
}
