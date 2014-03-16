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

#ifndef IPV6_LINUX_H
#define IPV6_LINUX_H

#include "ns3/ipv6.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/ipv6-pmtu-cache.h"
#include "ns3/ip-l4-protocol.h"

namespace ns3 {
class Ipv6Interface;

/**
 * \brief This implementation of Ipv6 for nodes using a real Linux Stack.
 *
 * The main goal of this class is not to do the stack job which is done by the real code ....
 * but it is used to not redo the wheel and in particular to be able to reuse these 2 standards NS-3 Helpers for two tasks:
 *   1 - assign address to devices using Ipv6AddressHelper,
 *   2 - create the static routes using Ipv6GlobalRoutingHelper
 *
 */
class Ipv6Linux : public Ipv6
{
public:
  static TypeId GetTypeId (void);
  Ipv6Linux ();
  virtual ~Ipv6Linux ();

  /**
   * \brief Register a new routing protocol to be used by this Ipv6 stack
   *
   * This call will replace any routing protocol that has been previously
   * registered.  If you want to add multiple routing protocols, you must
   * add them to a Ipv6ListRoutingProtocol directly.
   *
   * \param routingProtocol smart pointer to Ipv6RoutingProtocol object
   */
  virtual void SetRoutingProtocol (Ptr<Ipv6RoutingProtocol> routingProtocol);

  /**
   * \brief Get the routing protocol to be used by this Ipv6 stack
   *
   * \returns smart pointer to Ipv6RoutingProtocol object, or null pointer if none
   */
  virtual Ptr<Ipv6RoutingProtocol> GetRoutingProtocol (void) const;

  /**
   * \param device device to add to the list of Ipv6 interfaces
   *        which can be used as output interfaces during packet forwarding.
   * \returns the index of the Ipv6 interface added.
   *
   * Once a device has been added, it can never be removed: if you want
   * to disable it, you can invoke Ipv6::SetDown which will
   * make sure that it is never used during packet forwarding.
   */
  virtual uint32_t AddInterface (Ptr<NetDevice> device);

  /**
   * \returns the number of interfaces added by the user.
   */
  virtual uint32_t GetNInterfaces (void) const;

  /**
   * \brief Return the interface number of the interface that has been
   *        assigned the specified IP address.
   *
   * \param address The IP address being searched for
   * \returns The interface number of the Ipv6 interface with the given
   *          address or -1 if not found.
   *
   * Each IP interface has one or more IP addresses associated with it.
   * This method searches the list of interfaces for one that holds a
   * particular address.  This call takes an IP address as a parameter and
   * returns the interface number of the first interface that has been assigned
   * that address, or -1 if not found.  There must be an exact match; this
   * method will not match broadcast or multicast addresses.
   */
  virtual int32_t GetInterfaceForAddress (Ipv6Address address) const;

  /**
   * Do nothing
   */
  virtual void Send (Ptr<Packet> packet, Ipv6Address source,
                     Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route);

  /**
   * Do nothing
   */
  virtual void SendWithHeader (Ptr<Packet> packet, Ipv6Header ipHeader, Ptr<Ipv6Route> route);

  /**
   * Do nothing
   */
  //  virtual void Insert (Ptr<IpL4Protocol> protocol);

  /**
   * \brief Return the interface number of first interface found that
   *  has an Ipv6 address within the prefix specified by the input
   *  address and mask parameters
   *
   * \param address The IP address assigned to the interface of interest.
   * \param mask The IP prefix to use in the mask
   * \returns The interface number of the Ipv6 interface with the given
   *          address or -1 if not found.
   *
   * Each IP interface has one or more IP addresses associated with it.
   * This method searches the list of interfaces for the first one found
   * that holds an address that is included within the prefix
   * formed by the input address and mask parameters.  The value -1 is
   * returned if no match is found.
   */
  virtual int32_t GetInterfaceForPrefix (Ipv6Address address,
                                         Ipv6Prefix mask) const;

  /**
   * \param interface The interface number of an Ipv6 interface.
   * \returns The NetDevice associated with the Ipv6 interface number.
   */
  virtual Ptr<NetDevice> GetNetDevice (uint32_t interface);

  /**
   * \param device The NetDevice for an Ipv6Interface
   * \returns The interface number of an Ipv6 interface or -1 if not found.
   */
  virtual int32_t GetInterfaceForDevice (Ptr<const NetDevice> device) const;

  /**
   * \param interface Interface number of an Ipv6 interface
   * \param address Ipv6InterfaceAddress address to associate with the underlying Ipv6 interface
   * \returns true if the operation succeeded
   */
  virtual bool AddAddress (uint32_t interface, Ipv6InterfaceAddress address);

  /**
   * \param interface Interface number of an Ipv6 interface
   * \returns the number of Ipv6InterfaceAddress entries for the interface.
   */
  virtual uint32_t GetNAddresses (uint32_t interface) const;

  /**
   * Because addresses can be removed, the addressIndex is not guaranteed
   * to be static across calls to this method.
   *
   * \param interface Interface number of an Ipv6 interface
   * \param addressIndex index of Ipv6InterfaceAddress
   * \returns the Ipv6InterfaceAddress associated to the interface and addressIndex
   */
  virtual Ipv6InterfaceAddress GetAddress (uint32_t interface, uint32_t addressIndex) const;

  /**
   * Remove the address at addressIndex on named interface.  The addressIndex
   * for all higher indices will decrement by one after this method is called;
   * so, for example, to remove 5 addresses from an interface i, one could
   * call RemoveAddress (i, 0); 5 times.
   *
   * \param interface Interface number of an Ipv6 interface
   * \param addressIndex index of Ipv6InterfaceAddress to remove
   * \returns true if the operation succeeded
   */
  virtual bool RemoveAddress (uint32_t interface, uint32_t addressIndex);

  /**
   * \brief Remove the given address on named Ipv6 interface
   *
   * \param interface Interface number of an IPv6 interface
   * \param address the address to remove
   * \returns true if the operation succeeded
   */
  virtual bool RemoveAddress (uint32_t interface, Ipv6Address address);

  /**
   * \param interface The interface number of an Ipv6 interface
   * \param metric routing metric (cost) associated to the underlying
   *          Ipv6 interface
   */
  virtual void SetMetric (uint32_t interface, uint16_t metric);

  /**
   * \param interface The interface number of an Ipv6 interface
   * \returns routing metric (cost) associated to the underlying
   *          Ipv6 interface
   */
  virtual uint16_t GetMetric (uint32_t interface) const;

  /**
   * \param interface Interface number of Ipv6 interface
   * \returns the Maximum Transmission Unit (in bytes) associated
   *          to the underlying Ipv6 interface
   */
  virtual uint16_t GetMtu (uint32_t interface) const;

  /**
   * \brief Set the Path MTU for the specified IPv6 destination address.
   * \param dst Ipv6 destination address
   * \param pmtu the Path MTU
   */
  virtual void SetPmtu (Ipv6Address dst, uint32_t pmtu);

  /**
   * \param interface Interface number of Ipv6 interface
   * \returns true if the underlying interface is in the "up" state,
   *          false otherwise.
   */
  virtual bool IsUp (uint32_t interface) const;

  /**
   * \param interface Interface number of Ipv6 interface
   *
   * Set the interface into the "up" state. In this state, it is
   * considered valid during Ipv6 forwarding.
   */
  virtual void SetUp (uint32_t interface);

  /**
   * \param interface Interface number of Ipv6 interface
   *
   * Set the interface into the "down" state. In this state, it is
   * ignored during Ipv6 forwarding.
   */
  virtual void SetDown (uint32_t interface);

  /**
   * \param interface Interface number of Ipv6 interface
   * \returns true if IP forwarding enabled for input datagrams on this device
   */
  virtual bool IsForwarding (uint32_t interface) const;

  /**
   * \param interface Interface number of Ipv6 interface
   * \param val Value to set the forwarding flag
   *
   * If set to true, IP forwarding is enabled for input datagrams on this device
   */
  virtual void SetForwarding (uint32_t interface, bool val);

  /**
   * \brief Choose the source address to use with destination address.
   * \param interface interface index
   * \param dest IPv6 destination address
   * \return IPv6 source address to use
   */
  virtual Ipv6Address SourceAddressSelection (uint32_t interface, Ipv6Address dest);

  /**
   * Do nothing
   */
  virtual Ptr<IpL4Protocol> GetProtocol (int protocolNumber) const ;

  /**
   * Do nothing
   */
  virtual Ptr<Socket> CreateRawSocket (void);

  /**
   * Do nothing
   */
  virtual void DeleteRawSocket (Ptr<Socket> socket);

  static void InstallNode (Ptr<Node> node);

  /**
   * \brief Register the IPv6 Extensions.
   */
  virtual void RegisterExtensions ();

  /**
   * \brief Register the IPv6 Options.
   */
  virtual void RegisterOptions ();

private:
  // Indirect the Ipv6 attributes through private pure virtual methods
  virtual void SetIpForward (bool forward);
  virtual bool GetIpForward (void) const;
  /**
   * \brief Set IPv6 MTU discover state.
   * \param mtuDiscover IPv6 MTU discover enabled or not
   */
  virtual void SetMtuDiscover (bool mtuDiscover);

  /**
   * \brief Get IPv6 MTU discover state.
   * \return MTU discover state (enabled or not)
   */
  virtual bool GetMtuDiscover (void) const;

  virtual void SetWeakEsModel (bool model);
  virtual bool GetWeakEsModel (void) const;
  uint32_t AddIpv6Interface (Ptr<Ipv6Interface> interface);
  Ptr<Ipv6Interface> GetInterface (uint32_t i) const;

  typedef std::vector<Ptr<Ipv6Interface> > Ipv6InterfaceList;
  Ptr<Ipv6RoutingProtocol> m_routingProtocol;
  bool m_ipForward;
  bool m_mtuDiscover;
  Ptr<Ipv6PmtuCache> m_pmtuCache;
  bool m_weakEsModel;
  Ipv6InterfaceList m_interfaces;
  uint64_t m_nanoSec;
};
}
#endif // IPV6_LINUX_H
