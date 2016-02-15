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
 *         Hajime Tazaki <tazaki@nict.go.jp>
 */

#include "ipv4-linux.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-global-routing.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "linux-stack-helper.h"
#include "linux-ipv4-raw-socket-factory-impl.h"
#include "linux-udp-socket-factory-impl.h"
#include "linux-tcp-socket-factory-impl.h"
#include "linux-dccp-socket-factory-impl.h"
#include "linux-sctp-socket-factory-impl.h"

NS_LOG_COMPONENT_DEFINE ("DceIpv4Linux");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Ipv4Linux);

TypeId
Ipv4Linux::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4Linux")
    .SetParent<Ipv4> ()
    .AddConstructor<Ipv4Linux> ();

  return tid;
}

Ipv4Linux::Ipv4Linux ()
  : m_nanoSec (0)
{
  NS_LOG_FUNCTION (this);
}

Ipv4Linux::~Ipv4Linux ()
{
  NS_LOG_FUNCTION (this);
}

void
Ipv4Linux::SetRoutingProtocol (Ptr<Ipv4RoutingProtocol> routingProtocol)
{
  NS_LOG_FUNCTION (this);
  m_routingProtocol = routingProtocol;
  m_routingProtocol->SetIpv4 (this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4Linux::GetRoutingProtocol (void) const
{
  NS_LOG_FUNCTION (this);
  return m_routingProtocol;
}

uint32_t
Ipv4Linux::AddInterface (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << &device);

  Ptr<Ipv4Interface> interface = CreateObject<Ipv4Interface> ();
  interface->SetDevice (device);
  interface->SetForwarding (m_ipForward);
  return AddIpv4Interface (interface);
}
uint32_t
Ipv4Linux::AddIpv4Interface (Ptr<Ipv4Interface>interface)
{
  NS_LOG_FUNCTION (this << interface);
  uint32_t index = m_interfaces.size ();
  m_interfaces.push_back (interface);
  return index;
}

uint32_t
Ipv4Linux::GetNInterfaces (void) const
{
  NS_LOG_FUNCTION (this);
  return m_interfaces.size ();
}

int32_t
Ipv4Linux::GetInterfaceForAddress (Ipv4Address address) const
{
  NS_LOG_FUNCTION (this);

  int32_t interface = 0;
  for (Ipv4InterfaceList::const_iterator i = m_interfaces.begin ();
       i != m_interfaces.end ();
       i++, interface++)
    {
      for (uint32_t j = 0; j < (*i)->GetNAddresses (); j++)
        {
          if ((*i)->GetAddress (j).GetLocal () == address)
            {
              return interface;
            }
        }
    }

  return -1;
}

void
Ipv4Linux::Send (Ptr<Packet> packet, Ipv4Address source,
                 Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

void
Ipv4Linux::SendWithHeader (Ptr<Packet> packet, Ipv4Header ipHeader, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

void
Ipv4Linux::Insert (Ptr<IpL4Protocol> protocol)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

void
Ipv4Linux::Insert (Ptr<IpL4Protocol> protocol, uint32_t interfaceIndex)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

void
Ipv4Linux::Remove (Ptr<IpL4Protocol> protocol)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

void
Ipv4Linux::Remove (Ptr<IpL4Protocol> protocol, uint32_t interfaceIndex)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

bool
Ipv4Linux::IsDestinationAddress (Ipv4Address address, uint32_t iif) const
{
  // First check the incoming interface for a unicast address match
  for (uint32_t i = 0; i < GetNAddresses (iif); i++)
    {
      Ipv4InterfaceAddress iaddr = GetAddress (iif, i);
      if (address == iaddr.GetLocal ())
        {
          NS_LOG_LOGIC ("For me (destination " << address << " match)");
          return true;
        }
      if (address == iaddr.GetBroadcast ())
        {
          NS_LOG_LOGIC ("For me (interface broadcast address)");
          return true;
        }
    }

  if (address.IsMulticast ())
    {
#ifdef NOTYET
      if (MulticastCheckGroup (iif, address))
#endif
      if (true)
        {
          NS_LOG_LOGIC ("For me (Ipv4Addr multicast address");
          return true;
        }
    }

  if (address.IsBroadcast ())
    {
      NS_LOG_LOGIC ("For me (Ipv4Addr broadcast address)");
      return true;
    }

  if (GetWeakEsModel ())  // Check other interfaces
    {
      for (uint32_t j = 0; j < GetNInterfaces (); j++)
        {
          if (j == uint32_t (iif))
            {
              continue;
            }
          for (uint32_t i = 0; i < GetNAddresses (j); i++)
            {
              Ipv4InterfaceAddress iaddr = GetAddress (j, i);
              if (address == iaddr.GetLocal ())
                {
                  NS_LOG_LOGIC ("For me (destination " << address << " match) on another interface");
                  return true;
                }
              //  This is a small corner case:  match another interface's broadcast address
              if (address == iaddr.GetBroadcast ())
                {
                  NS_LOG_LOGIC ("For me (interface broadcast address on another interface)");
                  return true;
                }
            }
        }
    }
  return false;
}

int32_t
Ipv4Linux::GetInterfaceForPrefix (Ipv4Address address, Ipv4Mask mask) const
{
  NS_LOG_FUNCTION (this);
  int32_t interface = 0;
  for (Ipv4InterfaceList::const_iterator i = m_interfaces.begin ();
       i != m_interfaces.end ();
       i++, interface++)
    {
      for (uint32_t j = 0; j < (*i)->GetNAddresses (); j++)
        {
          if ((*i)->GetAddress (j).GetLocal ().CombineMask (mask) == address.CombineMask (mask))
            {
              return interface;
            }
        }
    }

  return -1;
}

Ptr<NetDevice>
Ipv4Linux::GetNetDevice (uint32_t i)
{
  NS_LOG_FUNCTION (this);
  return GetInterface (i)->GetDevice ();
}

int32_t
Ipv4Linux::GetInterfaceForDevice (Ptr<const NetDevice> device) const
{
  NS_LOG_FUNCTION (this);
  int32_t interface = 0;
  for (Ipv4InterfaceList::const_iterator i = m_interfaces.begin ();
       i != m_interfaces.end ();
       i++, interface++)
    {
      if ((*i)->GetDevice () == device)
        {
          return interface;
        }
    }

  return -1;
}

bool
Ipv4Linux::AddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << i << address);
  Ptr<Ipv4Interface> interface = GetInterface (i);
  bool retVal = interface->AddAddress (address);
  if (m_routingProtocol != 0)
    {
      m_routingProtocol->NotifyAddAddress (i, address);
    }

  std::ostringstream oss;
  Ptr<Node> node = this->GetObject<Node> ();
  oss << "-f inet addr add ";
  address.GetLocal ().Print (oss);
  oss << '/' << address.GetMask ().GetPrefixLength () << " dev sim" << i;
  LinuxStackHelper::RunIp (node, NanoSeconds (++m_nanoSec), oss.str ());
  oss.str ("");
  oss << "link set sim" << i << " up arp "
      << ((interface->GetDevice ()->NeedsArp ()) ? "on" : "off");
  LinuxStackHelper::RunIp (node, NanoSeconds (++m_nanoSec), oss.str ());

  return retVal;
}

uint32_t
Ipv4Linux::GetNAddresses (uint32_t interface) const
{
  Ptr<Ipv4Interface> iface = GetInterface (interface);
  return iface->GetNAddresses ();
}

Ipv4InterfaceAddress
Ipv4Linux::GetAddress (uint32_t interfaceIndex, uint32_t addressIndex) const
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv4Interface> interface = GetInterface (interfaceIndex);
  return interface->GetAddress (addressIndex);
}

bool
Ipv4Linux::RemoveAddress (uint32_t i, uint32_t addressIndex)
{
  NS_LOG_FUNCTION (this << i << addressIndex);
  Ptr<Ipv4Interface> interface = GetInterface (i);
  Ipv4InterfaceAddress address = interface->RemoveAddress (addressIndex);
  if (address != Ipv4InterfaceAddress ())
    {
      if (m_routingProtocol != 0)
        {
          m_routingProtocol->NotifyRemoveAddress (i, address);
        }
      return true;
    }
  return false;
}

bool
Ipv4Linux::RemoveAddress (uint32_t i, Ipv4Address address)
{
  NS_LOG_FUNCTION (this << i << address);

  if (address == Ipv4Address::GetLoopback())
    {
      NS_LOG_WARN ("Cannot remove loopback address.");
      return false;
    }
  Ptr<Ipv4Interface> interface = GetInterface (i);
  Ipv4InterfaceAddress ifAddr = interface->RemoveAddress (address);
  if (ifAddr != Ipv4InterfaceAddress ())
    {
      if (m_routingProtocol != 0)
        {
          m_routingProtocol->NotifyRemoveAddress (i, ifAddr);
        }
      return true;
    }
  return false;
}

Ipv4Address
Ipv4Linux::SourceAddressSelection (uint32_t interfaceIdx, Ipv4Address dest)
{
  return 0;
}

Ipv4Address
Ipv4Linux::SelectSourceAddress (Ptr<const NetDevice> device,
                                Ipv4Address dst, Ipv4InterfaceAddress::InterfaceAddressScope_e scope)
{
  NS_LOG_FUNCTION (this << device << dst << scope);
  Ipv4Address addr ("0.0.0.0");
  Ipv4InterfaceAddress iaddr;
  bool found = false;

  if (device != 0)
    {
      int32_t i = GetInterfaceForDevice (device);
      NS_ASSERT_MSG (i >= 0, "No device found on node");
      for (uint32_t j = 0; j < GetNAddresses (i); j++)
        {
          iaddr = GetAddress (i, j);
          if (iaddr.IsSecondary ())
            {
              continue;
            }
          if (iaddr.GetScope () > scope)
            {
              continue;
            }
          if (dst.CombineMask (iaddr.GetMask ())  == iaddr.GetLocal ().CombineMask (iaddr.GetMask ()) )
            {
              return iaddr.GetLocal ();
            }
          if (!found)
            {
              addr = iaddr.GetLocal ();
              found = true;
            }
        }
    }
  if (found)
    {
      return addr;
    }

  // Iterate among all interfaces
  for (uint32_t i = 0; i < GetNInterfaces (); i++)
    {
      for (uint32_t j = 0; j < GetNAddresses (i); j++)
        {
          iaddr = GetAddress (i, j);
          if (iaddr.IsSecondary ())
            {
              continue;
            }
          if (iaddr.GetScope () != Ipv4InterfaceAddress::LINK
              && iaddr.GetScope () <= scope)
            {
              return iaddr.GetLocal ();
            }
        }
    }
  NS_LOG_WARN ("Could not find source address for " << dst << " and scope "
                                                    << scope << ", returning 0");
  return addr;
}

void
Ipv4Linux::SetMetric (uint32_t i, uint16_t metric)
{
  NS_LOG_FUNCTION (this << i << metric);
  Ptr<Ipv4Interface> interface = GetInterface (i);
  interface->SetMetric (metric);
}

uint16_t
Ipv4Linux::GetMetric (uint32_t i) const
{
  Ptr<Ipv4Interface> interface = GetInterface (i);
  return interface->GetMetric ();
}

uint16_t
Ipv4Linux::GetMtu (uint32_t i) const
{
  Ptr<Ipv4Interface> interface = GetInterface (i);
  return interface->GetDevice ()->GetMtu ();
}

bool
Ipv4Linux::IsUp (uint32_t i) const
{
  Ptr<Ipv4Interface> interface = GetInterface (i);
  return interface->IsUp ();
}

void
Ipv4Linux::SetUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  Ptr<Ipv4Interface> interface = GetInterface (i);
  interface->SetUp ();

  if (m_routingProtocol != 0)
    {
      m_routingProtocol->NotifyInterfaceUp (i);
    }
}

void
Ipv4Linux::SetDown (uint32_t ifaceIndex)
{
  NS_LOG_FUNCTION (this << ifaceIndex);
  Ptr<Ipv4Interface> interface = GetInterface (ifaceIndex);
  interface->SetDown ();

  if (m_routingProtocol != 0)
    {
      m_routingProtocol->NotifyInterfaceDown (ifaceIndex);
    }
}

bool
Ipv4Linux::IsForwarding (uint32_t i) const
{
  NS_LOG_FUNCTION (this << i);
  Ptr<Ipv4Interface> interface = GetInterface (i);
  NS_LOG_LOGIC ("Forwarding state: " << interface->IsForwarding ());
  return interface->IsForwarding ();
}

void
Ipv4Linux::SetForwarding (uint32_t i, bool val)
{
  NS_LOG_FUNCTION (this << i);
  Ptr<Ipv4Interface> interface = GetInterface (i);
  interface->SetForwarding (val);
}

void
Ipv4Linux::SetIpForward (bool forward)
{
  NS_LOG_FUNCTION (this << forward);
  m_ipForward = forward;
  for (Ipv4InterfaceList::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
    {
      (*i)->SetForwarding (forward);
    }
}

bool
Ipv4Linux::GetIpForward (void) const
{
  return m_ipForward;
}

void
Ipv4Linux::SetWeakEsModel (bool model)
{
  m_weakEsModel = model;
}

bool
Ipv4Linux::GetWeakEsModel (void) const
{
  return m_weakEsModel;
}

void
Ipv4Linux::InstallNode (Ptr<Node> node)
{
  ObjectFactory factory;
  factory.SetTypeId ("ns3::Ipv4Linux");
  Ptr<Object> protocol = factory.Create <Object> ();
  node->AggregateObject (protocol);
  Ipv4GlobalRoutingHelper globalRouting;
  // Set routing
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  Ptr<Ipv4RoutingProtocol> ipv4Routing = globalRouting.Create (node);
  ipv4->SetRoutingProtocol (ipv4Routing);
  // Socket related stuff
  Ptr<LinuxIpv4RawSocketFactoryImpl> ipv4rawFactory = CreateObject<LinuxIpv4RawSocketFactoryImpl> ();
  node->AggregateObject (ipv4rawFactory);
  Ptr<LinuxUdpSocketFactoryImpl> udpFactory = CreateObject<LinuxUdpSocketFactoryImpl> ();
  node->AggregateObject (udpFactory);
  Ptr<LinuxTcpSocketFactoryImpl> tcpFactory = CreateObject<LinuxTcpSocketFactoryImpl> ();
  node->AggregateObject (tcpFactory);
  Ptr<LinuxDccpSocketFactoryImpl> dccpFactory = CreateObject<LinuxDccpSocketFactoryImpl> ();
  node->AggregateObject (dccpFactory);
  Ptr<LinuxSctpSocketFactoryImpl> sctpFactory = CreateObject<LinuxSctpSocketFactoryImpl> ();
  node->AggregateObject (sctpFactory);
}
Ptr<Ipv4Interface>
Ipv4Linux::GetInterface (uint32_t index) const
{
  if (index < m_interfaces.size ())
    {
      return m_interfaces[index];
    }
  return 0;
}

Ptr<IpL4Protocol>
Ipv4Linux::GetProtocol (int protocolNumber) const
{
  return 0;
}

Ptr<IpL4Protocol>
Ipv4Linux::GetProtocol (int protocolNumber, int32_t interfaceIndex) const
{
  return 0;
}

Ptr<Socket>
Ipv4Linux::CreateRawSocket (void)
{
  return 0;
}

/**
 * Do nothing
 */
void
Ipv4Linux::DeleteRawSocket (Ptr<Socket> socket)
{

}

void
Ipv4Linux::PopulateRoutingTable ()
{
  // Only support Ipv4GlobalRouting
  Ptr<Node> node = this->GetObject<Node> ();
  Ptr<Ipv4GlobalRouting> globalRouting = DynamicCast<Ipv4GlobalRouting> (GetRoutingProtocol ());
  NS_ASSERT_MSG (globalRouting, "No global routing");

  for (uint32_t i = 0; i < globalRouting->GetNRoutes (); i++)
    {
      Ipv4RoutingTableEntry route = globalRouting->GetRoute (i);
      std::ostringstream oss;
      oss << "route add to " << route.GetDest () << '/' << route.GetDestNetworkMask () << " via " << route.GetGateway ();
      LinuxStackHelper::RunIp (node, NanoSeconds (++m_nanoSec), oss.str ());
    }

  std::ostringstream oss;
  oss.str ("");
  oss << "link set lo up";
  LinuxStackHelper::RunIp (node, NanoSeconds (++m_nanoSec), oss.str ());
}

}
