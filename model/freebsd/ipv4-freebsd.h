/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Hajime Tazaki
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
 * Author:  Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef IPV4_FREEBSD_H
#define IPV4_FREEBSD_H

#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ip-l4-protocol.h"

namespace ns3 {
class Ipv4Interface;

/**
 * \brief This implementation of Ipv4 for nodes using a real FreeBSD Stack.
 *
 * The main goal of this class is not to do the stack job which is done by the real code ....
 * but it is used to not redo the wheel and in particular to be able to reuse these 2 standards NS-3 Helpers for two tasks:
 *   1 - assign address to devices using Ipv4AddressHelper,
 *   2 - create the static routes using Ipv4GlobalRoutingHelper
 *
 */
class Ipv4FreeBSD : public Ipv4
{
public:
  static TypeId GetTypeId (void);
  Ipv4FreeBSD ();
  virtual ~Ipv4FreeBSD ();

  /**
   * \brief Register a new routing protocol to be used by this Ipv4 stack
   *
   * This call will replace any routing protocol that has been previously
   * registered.  If you want to add multiple routing protocols, you must
   * add them to a Ipv4ListRoutingProtocol directly.
   *
   * \param routingProtocol smart pointer to Ipv4RoutingProtocol object
   */
  virtual void SetRoutingProtocol (Ptr<Ipv4RoutingProtocol> routingProtocol);

  /**
   * \brief Get the routing protocol to be used by this Ipv4 stack
   *
   * \returns smart pointer to Ipv4RoutingProtocol object, or null pointer if none
   */
  virtual Ptr<Ipv4RoutingProtocol> GetRoutingProtocol (void) const;

  /**
   * \param device device to add to the list of Ipv4 interfaces
   *        which can be used as output interfaces during packet forwarding.
   * \returns the index of the Ipv4 interface added.
   *
   * Once a device has been added, it can never be removed: if you want
   * to disable it, you can invoke Ipv4::SetDown which will
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
   * \returns The interface number of the Ipv4 interface with the given
   *          address or -1 if not found.
   *
   * Each IP interface has one or more IP addresses associated with it.
   * This method searches the list of interfaces for one that holds a
   * particular address.  This call takes an IP address as a parameter and
   * returns the interface number of the first interface that has been assigned
   * that address, or -1 if not found.  There must be an exact match; this
   * method will not match broadcast or multicast addresses.
   */
  virtual int32_t GetInterfaceForAddress (Ipv4Address address) const;

  /**
   * Do nothing
   */
  virtual void Send (Ptr<Packet> packet, Ipv4Address source,
                     Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

  /**
   * Do nothing
   */
  virtual void SendWithHeader (Ptr<Packet> packet, Ipv4Header ipHeader, Ptr<Ipv4Route> route);

  /**
   * Do nothing
   */
  virtual void Insert (Ptr<IpL4Protocol> protocol);

  /**
   * \brief Determine whether address and interface corresponding to
   *        received packet can be accepted for local delivery
   *
   * \param address The IP address being considered
   * \param iif The incoming Ipv4 interface index
   *
   * This method can be used to determine whether a received packet has
   * an acceptable address for local delivery on the host.  The address
   * may be a unicast, multicast, or broadcast address.  This method will
   * return true if address is an exact match of a unicast address on
   * one of the host's interfaces (see below), if address corresponds to
   * a multicast group that the host has joined (and the incoming device
   * is acceptable), or if address corresponds to a broadcast address.
   *
   * If the Ipv4 attribute WeakEsModel is true, the unicast address may
   * match any of the Ipv4 addresses on any interface.  If the attribute is
   * false, the address must match one assigned to the incoming device.
   */
  virtual bool IsDestinationAddress (Ipv4Address address, uint32_t iif) const;

  /**
   * \brief Return the interface number of first interface found that
   *  has an Ipv4 address within the prefix specified by the input
   *  address and mask parameters
   *
   * \param address The IP address assigned to the interface of interest.
   * \param mask The IP prefix to use in the mask
   * \returns The interface number of the Ipv4 interface with the given
   *          address or -1 if not found.
   *
   * Each IP interface has one or more IP addresses associated with it.
   * This method searches the list of interfaces for the first one found
   * that holds an address that is included within the prefix
   * formed by the input address and mask parameters.  The value -1 is
   * returned if no match is found.
   */
  virtual int32_t GetInterfaceForPrefix (Ipv4Address address,
                                         Ipv4Mask mask) const;

  /**
   * \param interface The interface number of an Ipv4 interface.
   * \returns The NetDevice associated with the Ipv4 interface number.
   */
  virtual Ptr<NetDevice> GetNetDevice (uint32_t interface);

  /**
   * \param device The NetDevice for an Ipv4Interface
   * \returns The interface number of an Ipv4 interface or -1 if not found.
   */
  virtual int32_t GetInterfaceForDevice (Ptr<const NetDevice> device) const;

  /**
   * \param interface Interface number of an Ipv4 interface
   * \param address Ipv4InterfaceAddress address to associate with the underlying Ipv4 interface
   * \returns true if the operation succeeded
   */
  virtual bool AddAddress (uint32_t interface, Ipv4InterfaceAddress address);

  /**
   * \param interface Interface number of an Ipv4 interface
   * \returns the number of Ipv4InterfaceAddress entries for the interface.
   */
  virtual uint32_t GetNAddresses (uint32_t interface) const;

  /**
   * Because addresses can be removed, the addressIndex is not guaranteed
   * to be static across calls to this method.
   *
   * \param interface Interface number of an Ipv4 interface
   * \param addressIndex index of Ipv4InterfaceAddress
   * \returns the Ipv4InterfaceAddress associated to the interface and addressIndex
   */
  virtual Ipv4InterfaceAddress GetAddress (uint32_t interface, uint32_t addressIndex) const;

  /**
   * Remove the address at addressIndex on named interface.  The addressIndex
   * for all higher indices will decrement by one after this method is called;
   * so, for example, to remove 5 addresses from an interface i, one could
   * call RemoveAddress (i, 0); 5 times.
   *
   * \param interface Interface number of an Ipv4 interface
   * \param addressIndex index of Ipv4InterfaceAddress to remove
   * \returns true if the operation succeeded
   */
  virtual bool RemoveAddress (uint32_t interface, uint32_t addressIndex);

   /**
   * \brief Remove the given address on named Ipv4 interface
   *
   * \param interface Interface number of an Ipv4 interface
   * \param address The address to remove
   * \returns true if the operation succeeded
   */
  virtual bool RemoveAddress (uint32_t interface, Ipv4Address address);

  /**
   * \brief Return the first primary source address with scope less than
   * or equal to the requested scope, to use in sending a packet to
   * destination dst out of the specified device.
   *
   * This method mirrors the behavior of FreeBSD inet_select_addr() and is
   * provided because interfaces may have multiple IP addresses configured
   * on them with different scopes, and with a primary and secondary status.
   * Secondary addresses are never returned.
   * \see Ipv4InterfaceAddress
   *
   * If a non-zero device pointer is provided, the method first tries to
   * return a primary address that is configured on that device, and whose
   * subnet matches that of dst and whose scope is less than or equal to
   * the requested scope.  If a primary address does not match the
   * subnet of dst but otherwise matches the scope, it is returned.
   * If no such address on the device is found, the other devices are
   * searched in order of their interface index, but not considering dst
   * as a factor in the search.  Because a loopback interface is typically
   * the first one configured on a node, it will be the first alternate
   * device to be tried.  Addresses scoped at LINK scope are not returned
   * in this phase.
   *
   * If no device pointer is provided, the same logic as above applies, only
   * that there is no preferred device that is consulted first.  This means
   * that if the device pointer is null, input parameter dst will be ignored.
   *
   * If there are no possible addresses to return, a warning log message
   * is issued and the all-zeroes address is returned.
   *
   * \param device output NetDevice (optionally provided, only to constrain the search)
   * \param dst Destination address to match, if device is provided
   * \param scope Scope of returned address must be less than or equal to this
   * \returns the first primary Ipv4Address that meets the search criteria
   */
  virtual Ipv4Address SelectSourceAddress (Ptr<const NetDevice> device,
                                           Ipv4Address dst, Ipv4InterfaceAddress::InterfaceAddressScope_e scope);

  /**
   * \param interface The interface number of an Ipv4 interface
   * \param metric routing metric (cost) associated to the underlying
   *          Ipv4 interface
   */
  virtual void SetMetric (uint32_t interface, uint16_t metric);

  /**
   * \param interface The interface number of an Ipv4 interface
   * \returns routing metric (cost) associated to the underlying
   *          Ipv4 interface
   */
  virtual uint16_t GetMetric (uint32_t interface) const;

  /**
   * \param interface Interface number of Ipv4 interface
   * \returns the Maximum Transmission Unit (in bytes) associated
   *          to the underlying Ipv4 interface
   */
  virtual uint16_t GetMtu (uint32_t interface) const;

  /**
   * \param interface Interface number of Ipv4 interface
   * \returns true if the underlying interface is in the "up" state,
   *          false otherwise.
   */
  virtual bool IsUp (uint32_t interface) const;

  /**
   * \param interface Interface number of Ipv4 interface
   *
   * Set the interface into the "up" state. In this state, it is
   * considered valid during Ipv4 forwarding.
   */
  virtual void SetUp (uint32_t interface);

  /**
   * \param interface Interface number of Ipv4 interface
   *
   * Set the interface into the "down" state. In this state, it is
   * ignored during Ipv4 forwarding.
   */
  virtual void SetDown (uint32_t interface);

  /**
   * \param interface Interface number of Ipv4 interface
   * \returns true if IP forwarding enabled for input datagrams on this device
   */
  virtual bool IsForwarding (uint32_t interface) const;

  /**
   * \param interface Interface number of Ipv4 interface
   * \param val Value to set the forwarding flag
   *
   * If set to true, IP forwarding is enabled for input datagrams on this device
   */
  virtual void SetForwarding (uint32_t interface, bool val);

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
  void PopulateRoutingTable ();

private:
  // Indirect the Ipv4 attributes through private pure virtual methods
  virtual void SetIpForward (bool forward);
  virtual bool GetIpForward (void) const;
  virtual void SetWeakEsModel (bool model);
  virtual bool GetWeakEsModel (void) const;
  uint32_t AddIpv4Interface (Ptr<Ipv4Interface> interface);
  Ptr<Ipv4Interface> GetInterface (uint32_t i) const;

  typedef std::vector<Ptr<Ipv4Interface> > Ipv4InterfaceList;
  Ptr<Ipv4RoutingProtocol> m_routingProtocol;
  bool m_ipForward;
  bool m_weakEsModel;
  Ipv4InterfaceList m_interfaces;
  uint64_t m_nanoSec;
};
}
#endif // IPV4_FREEBSD_H
