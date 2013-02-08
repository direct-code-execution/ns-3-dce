/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
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
 * Author: George F. Riley<riley@ece.gatech.edu>
 *         Gustavo Carneiro <gjc@inescporto.pt>
 */

#ifndef IPV4_DCE_ROUTING_H
#define IPV4_DCE_ROUTING_H

#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ptr.h"

namespace ns3 {

class NetlinkSocket;

/**
 * \ingroup dce
 *
 * \brief DCE managed routing protocol for IP version 4 stacks.
 *
 * This class provides a basic set of methods to manage routing
 * entries from DCE and notify DCE about interface changes.
 *
 * The Ipv4DceRouting class inherits from Ipv4StaticRouting class.
 *
 * \see Ipv4RoutingProtocol
 * \see Ipv4StaticRouting
 * \see Ipv4ListRouting
 * \see Ipv4ListRouting::AddRoutingProtocol
 */
class Ipv4DceRouting : public Ipv4StaticRouting
{
public:
  static TypeId GetTypeId (void);

  Ipv4DceRouting ();
  virtual ~Ipv4DceRouting ();

  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);

  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;

  virtual void SetIpv4 (Ptr<Ipv4> ipv4);

  template<class T>
  static Ptr<T> GetRouting (Ptr<Ipv4RoutingProtocol> ipv4rp, T*);

private:
  Ptr<Ipv4> m_ipv4;
  Ptr<NetlinkSocket> m_netlink;
};
// This function does a recursive search for a requested routing protocol.
// Strictly speaking this recursion is not necessary, but why not?
template<class T>
Ptr<T> Ipv4DceRouting::GetRouting (Ptr<Ipv4RoutingProtocol> ipv4rp, T* type)
{
  if (ipv4rp == 0)
    {
      return 0;
    }

  if (DynamicCast<T> (ipv4rp))
    {
      return DynamicCast<T> (ipv4rp);
    }
  else if (DynamicCast<Ipv4ListRouting> (ipv4rp))
    {
      Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
      for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
        {
          int16_t priority;
          Ptr<T> ret = GetRouting (lrp->GetRoutingProtocol (i, priority), type);
          if (ret != 0)
            {
              return ret;
            }
        }
    }
  return 0;
}
} // Namespace ns3

#endif /* IPV4_DCE_ROUTING_H */
