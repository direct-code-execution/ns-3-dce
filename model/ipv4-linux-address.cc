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
#include "dce-application-helper.h"

NS_LOG_COMPONENT_DEFINE ("Ipv4LinuxAddress");

namespace ns3 {

Ipv4LinuxAddress::Ipv4LinuxAddress (uint32_t n, uint32_t d, Ipv4Address a, Ipv4Mask m, bool p)
  : m_nodeid (n), m_deviceid (d), m_address (a), m_mask (m), m_deviceIsPoint2point (p)
{
}

void
Ipv4LinuxAddress::Install (Ptr<Node> node)
{
  NS_LOG_INFO ("N:"<< m_nodeid << " D:" << m_deviceid << " A:" << m_address << " M:"<< m_mask);

  std::ostringstream oss;
  oss << "sim" << m_deviceid;
  std::string deviceName = oss.str ();

  oss.clear (); oss.str ("");
  oss << Ipv4AddressToString (m_address) << '/' ;
  m_mask.Print (oss);

  std::string addrMask = oss.str ();

  AddAddress (node,  NanoSeconds(1), deviceName, addrMask);

  oss.clear (); oss.str ("");
  oss << "link set " << deviceName << " up arp " <<  ((m_deviceIsPoint2point)?"off":"on");

  RunIp (node, NanoSeconds (2), oss.str () );

}

void
Ipv4LinuxAddress::RunIp (Ptr<Node> node, Time at, std::string str)
{
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1<<16);
  process.ResetArguments();
  process.ParseArguments(str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

void
Ipv4LinuxAddress::AddAddress (Ptr<Node> node, Time at, std::string name, std::string address)
{
  std::ostringstream oss;
  oss << "-f inet addr add " << address << " dev " << name;
  RunIp (node, at, oss.str ());
}

std::string
Ipv4LinuxAddress::Ipv4AddressToString (Ipv4Address ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}

TypeId
Ipv4LinuxAddressContainer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4LinuxAddressContainer")
    .SetParent<Object> ();

  return tid;
}

Ipv4LinuxAddressContainer::Ipv4LinuxAddressContainer (uint32_t n) : m_nodeid (n)
{
}

void
Ipv4LinuxAddressContainer::AddAddress (uint32_t d, Ipv4Address a, Ipv4Mask m, bool p2p)
{
  m_list.push_back (Ipv4LinuxAddress (m_nodeid, d, a, m, p2p));
}

void
Ipv4LinuxAddressContainer::Install (Ptr<Node> node)
{
  for (std::list<Ipv4LinuxAddress>::iterator i = m_list.begin ();
      i != m_list.end (); ++i)
    {
      i->Install (node);
    }
}

}
