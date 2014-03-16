/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Hajime Tazaki
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.jp>
 */

#include "ipv6-linux.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "linux-stack-helper.h"
#include "ns3/ipv6-list-routing-helper.h"
#include "ns3/ipv6-static-routing-helper.h"
//#include "ns3/ipv6-global-routing-helper.h"
#include "ns3/ipv6-interface.h"
#include "linux-ipv6-raw-socket-factory-impl.h"
#include "linux-udp6-socket-factory-impl.h"
#include "linux-tcp6-socket-factory-impl.h"
#include "linux-dccp6-socket-factory-impl.h"
#include "linux-sctp6-socket-factory-impl.h"

NS_LOG_COMPONENT_DEFINE ("Ipv6Linux");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Ipv6Linux);

TypeId
Ipv6Linux::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv6Linux")
    .SetParent<Ipv6> ()
    .AddConstructor<Ipv6Linux> ();

  return tid;
}

Ipv6Linux::Ipv6Linux()
  : m_nanoSec (0)
{
  NS_LOG_FUNCTION (this);
}

Ipv6Linux::~Ipv6Linux ()
{
  NS_LOG_FUNCTION (this);
}

void
Ipv6Linux::SetRoutingProtocol (Ptr<Ipv6RoutingProtocol> routingProtocol)
{
  NS_LOG_FUNCTION (this);
  m_routingProtocol = routingProtocol;
  m_routingProtocol->SetIpv6 (this);
}

Ptr<Ipv6RoutingProtocol>
Ipv6Linux::GetRoutingProtocol (void) const
{
  NS_LOG_FUNCTION (this);
  return m_routingProtocol;
}

uint32_t
Ipv6Linux::AddInterface (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << &device);

  Ptr<Node> node = GetObject<Node> ();
  Ptr<Ipv6Interface> interface = CreateObject<Ipv6Interface> ();
  interface->SetNode (node);
  interface->SetDevice (device);
  interface->SetForwarding (m_ipForward);
  return AddIpv6Interface (interface);
}
uint32_t
Ipv6Linux::AddIpv6Interface (Ptr<Ipv6Interface>interface)
{
  NS_LOG_FUNCTION (this << interface);
  uint32_t index = m_interfaces.size ();
  m_interfaces.push_back (interface);
  return index;
}

uint32_t
Ipv6Linux::GetNInterfaces (void) const
{
  NS_LOG_FUNCTION (this);
  return m_interfaces.size ();
}

int32_t
Ipv6Linux::GetInterfaceForAddress (Ipv6Address address) const
{
  NS_LOG_FUNCTION (this);

  int32_t interface = 0;
  for (Ipv6InterfaceList::const_iterator i = m_interfaces.begin ();
       i != m_interfaces.end ();
       i++, interface++)
    {
      for (uint32_t j = 0; j < (*i)->GetNAddresses (); j++)
        {
          if ((*i)->GetAddress (j).GetAddress () == address)
            {
              return interface;
            }
        }
    }

  return -1;
}

void
Ipv6Linux::Send (Ptr<Packet> packet, Ipv6Address source,
                   Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

void
Ipv6Linux::SendWithHeader (Ptr<Packet> packet, Ipv6Header ipHeader, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << "empty method.");
}

#if 0
void
Ipv6Linux::Insert (Ptr<IpL4Protocol> protocol)
{
  NS_LOG_FUNCTION (this << "empty method.");
}
#endif


int32_t
Ipv6Linux::GetInterfaceForPrefix (Ipv6Address address, Ipv6Prefix mask) const
{
  NS_LOG_FUNCTION (this);
  int32_t interface = 0;
  for (Ipv6InterfaceList::const_iterator i = m_interfaces.begin ();
       i != m_interfaces.end ();
       i++, interface++)
    {
      for (uint32_t j = 0; j < (*i)->GetNAddresses (); j++)
        {
          if ((*i)->GetAddress (j).GetAddress ().CombinePrefix (mask) == address.CombinePrefix (mask))
            {
              return interface;
            }
        }
    }

  return -1;
}

Ptr<NetDevice>
Ipv6Linux::GetNetDevice (uint32_t i)
{
  NS_LOG_FUNCTION (this);
  return GetInterface (i)->GetDevice ();
}

int32_t
Ipv6Linux::GetInterfaceForDevice (Ptr<const NetDevice> device) const
{
  NS_LOG_FUNCTION (this);
  int32_t interface = 0;
  for (Ipv6InterfaceList::const_iterator i = m_interfaces.begin ();
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
Ipv6Linux::AddAddress (uint32_t i, Ipv6InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << i << address);
  Ptr<Ipv6Interface> interface = GetInterface (i);
  bool retVal = interface->AddAddress (address);
  if (m_routingProtocol != 0)
    {
      m_routingProtocol->NotifyAddAddress (i, address);
    }

  std::ostringstream oss;
  Ptr<Node> node = this->GetObject<Node> ();
  oss << "-f inet6 addr add ";
  address.GetAddress ().Print (oss);
  oss << address.GetPrefix () << " dev sim" << i;
  LinuxStackHelper::RunIp (node, NanoSeconds (++m_nanoSec), oss.str ());
  oss.str ("");
  oss << "link set sim" << i << " up arp "
      << ((interface->GetDevice ()->NeedsArp ()) ? "on" : "off");
  LinuxStackHelper::RunIp (node, NanoSeconds (++m_nanoSec), oss.str ());

  return retVal;
}

uint32_t
Ipv6Linux::GetNAddresses (uint32_t interface) const
{
  Ptr<Ipv6Interface> iface = GetInterface (interface);
  return iface->GetNAddresses ();
}

Ipv6InterfaceAddress
Ipv6Linux::GetAddress (uint32_t interfaceIndex, uint32_t addressIndex) const
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv6Interface> interface = GetInterface (interfaceIndex);
  return interface->GetAddress (addressIndex);
}

bool
Ipv6Linux::RemoveAddress (uint32_t i, uint32_t addressIndex)
{
  NS_LOG_FUNCTION (this << i << addressIndex);
  Ptr<Ipv6Interface> interface = GetInterface (i);
  Ipv6InterfaceAddress address = interface->RemoveAddress (addressIndex);
  if (address != Ipv6InterfaceAddress ())
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
Ipv6Linux::RemoveAddress (uint32_t i, Ipv6Address address)
{
  NS_LOG_FUNCTION (this << i << address);

  if (address == Ipv6Address::GetLoopback())
    {
      NS_LOG_WARN ("Cannot remove loopback address.");
      return false;
    }
  Ptr<Ipv6Interface> interface = GetInterface (i);
  Ipv6InterfaceAddress ifAddr = interface->RemoveAddress (address);
  if (ifAddr != Ipv6InterfaceAddress ())
  {
    if (m_routingProtocol != 0)
    {
      m_routingProtocol->NotifyRemoveAddress (i, ifAddr);
    }
    return true;
  }
  return false;
}

void
Ipv6Linux::SetMetric (uint32_t i, uint16_t metric)
{
  NS_LOG_FUNCTION (this << i << metric);
  Ptr<Ipv6Interface> interface = GetInterface (i);
  interface->SetMetric (metric);
}

uint16_t
Ipv6Linux::GetMetric (uint32_t i) const
{
  Ptr<Ipv6Interface> interface = GetInterface (i);
  return interface->GetMetric ();
}

uint16_t
Ipv6Linux::GetMtu (uint32_t i) const
{
  Ptr<Ipv6Interface> interface = GetInterface (i);
  return interface->GetDevice ()->GetMtu ();
}

void
Ipv6Linux::SetPmtu (Ipv6Address dst, uint32_t pmtu)
{
  NS_LOG_FUNCTION (this << dst << int(pmtu));
  m_pmtuCache->SetPmtu (dst, pmtu);
}

bool
Ipv6Linux::IsUp (uint32_t i) const
{
  Ptr<Ipv6Interface> interface = GetInterface (i);
  return interface->IsUp ();
}

void
Ipv6Linux::SetUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  Ptr<Ipv6Interface> interface = GetInterface (i);
  interface->SetUp ();

  if (m_routingProtocol != 0)
    {
      m_routingProtocol->NotifyInterfaceUp (i);
    }
}

void
Ipv6Linux::SetDown (uint32_t ifaceIndex)
{
  NS_LOG_FUNCTION (this << ifaceIndex);
  Ptr<Ipv6Interface> interface = GetInterface (ifaceIndex);
  interface->SetDown ();

  if (m_routingProtocol != 0)
    {
      m_routingProtocol->NotifyInterfaceDown (ifaceIndex);
    }
}

bool
Ipv6Linux::IsForwarding (uint32_t i) const
{
  NS_LOG_FUNCTION (this << i);
  Ptr<Ipv6Interface> interface = GetInterface (i);
  NS_LOG_LOGIC ("Forwarding state: " << interface->IsForwarding ());
  return interface->IsForwarding ();
}

void
Ipv6Linux::SetForwarding (uint32_t i, bool val)
{
  NS_LOG_FUNCTION (this << i);
  Ptr<Ipv6Interface> interface = GetInterface (i);
  interface->SetForwarding (val);
}

void
Ipv6Linux::SetIpForward (bool forward)
{
  NS_LOG_FUNCTION (this << forward);
  m_ipForward = forward;
  for (Ipv6InterfaceList::const_iterator i = m_interfaces.begin (); i != m_interfaces.end (); i++)
    {
      (*i)->SetForwarding (forward);
    }
}

Ipv6Address
Ipv6Linux::SourceAddressSelection (uint32_t interface, Ipv6Address dest)
{
  NS_LOG_FUNCTION (this << interface << dest);
  Ipv6Address ret;

  if (dest.IsLinkLocal () || dest.IsLinkLocalMulticast ())
    {
      for (uint32_t i = 0; i < GetNAddresses (interface); i++)
        {
          Ipv6InterfaceAddress test = GetAddress (interface, i);
          if (test.GetScope () == Ipv6InterfaceAddress::LINKLOCAL)
            {
              return test.GetAddress ();
            }
        }
      NS_ASSERT_MSG (false, "No link-local address found on interface " << interface);
    }

  for (uint32_t i = 0; i < GetNAddresses (interface); i++)
    {
      Ipv6InterfaceAddress test = GetAddress (interface, i);

      if (test.GetScope () == Ipv6InterfaceAddress::GLOBAL)
        {
          if (test.IsInSameSubnet (dest))
            {
              return test.GetAddress ();
            }
          else
            {
              ret = test.GetAddress ();
            }
        }
    }

  // no specific match found. Use a global address (any useful is fine).
  return ret;
}

bool
Ipv6Linux::GetIpForward (void) const
{
  return m_ipForward;
}

void
Ipv6Linux::SetMtuDiscover (bool mtuDiscover)
{
  NS_LOG_FUNCTION (this << int(mtuDiscover));
  m_mtuDiscover = mtuDiscover;
}

bool
Ipv6Linux::GetMtuDiscover () const
{
  NS_LOG_FUNCTION (this);
  return m_mtuDiscover;
}

void
Ipv6Linux::SetWeakEsModel (bool model)
{
  m_weakEsModel = model;
}

bool
Ipv6Linux::GetWeakEsModel (void) const
{
  return m_weakEsModel;
}

void
Ipv6Linux::InstallNode (Ptr<Node> node)
{
  ObjectFactory factory;
  factory.SetTypeId ("ns3::Ipv6Linux");
  Ptr<Object> protocol = factory.Create <Object> ();
  node->AggregateObject (protocol);
  // Set routing
  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
  Ipv6ListRoutingHelper listRoutingv6;
  Ipv6StaticRoutingHelper staticRoutingv6;
  listRoutingv6.Add (staticRoutingv6, 0);
  Ptr<Ipv6RoutingProtocol> ipv6Routing = listRoutingv6.Create (node);
  ipv6->SetRoutingProtocol (ipv6Routing);
  // Socket related stuff
  Ptr<LinuxIpv6RawSocketFactoryImpl> ipv6rawFactory = CreateObject<LinuxIpv6RawSocketFactoryImpl> ();
  node->AggregateObject (ipv6rawFactory);
  Ptr<LinuxUdp6SocketFactoryImpl> udp6Factory = CreateObject<LinuxUdp6SocketFactoryImpl> ();
  node->AggregateObject (udp6Factory);
  Ptr<LinuxTcp6SocketFactoryImpl> tcp6Factory = CreateObject<LinuxTcp6SocketFactoryImpl> ();
  node->AggregateObject (tcp6Factory);
  Ptr<LinuxDccp6SocketFactoryImpl> dccp6Factory = CreateObject<LinuxDccp6SocketFactoryImpl> ();
  node->AggregateObject (dccp6Factory);
  Ptr<LinuxSctp6SocketFactoryImpl> sctp6Factory = CreateObject<LinuxSctp6SocketFactoryImpl> ();
  node->AggregateObject (sctp6Factory);
}
Ptr<Ipv6Interface>
Ipv6Linux::GetInterface (uint32_t index) const
{
  if (index < m_interfaces.size ())
    {
      return m_interfaces[index];
    }
  return 0;
}

Ptr<IpL4Protocol>
Ipv6Linux::GetProtocol (int protocolNumber) const
{
  return 0;
}

Ptr<Socket>
Ipv6Linux::CreateRawSocket (void)
{
  return 0;
}

/**
 * Do nothing
 */
void
Ipv6Linux::DeleteRawSocket (Ptr<Socket> socket)
{

}

void
Ipv6Linux::RegisterExtensions ()
{
  NS_ASSERT (0);
}

void 
Ipv6Linux::RegisterOptions ()
{
  NS_ASSERT (0);
}

}
