/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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

#include <vector>
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/assert.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-routing-protocol.h"

#include "../model/ipv4-dce-routing.h"
#include "ipv4-dce-routing-helper.h"

NS_LOG_COMPONENT_DEFINE ("Ipv4DceRoutingHelper");

namespace ns3 {

Ipv4DceRoutingHelper::Ipv4DceRoutingHelper ()
{
}

Ipv4DceRoutingHelper::Ipv4DceRoutingHelper (const Ipv4DceRoutingHelper &o)
  : Ipv4StaticRoutingHelper (o)
{
}

Ipv4DceRoutingHelper*
Ipv4DceRoutingHelper::Copy (void) const
{
  NS_LOG_FUNCTION (this);
  return new Ipv4DceRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
Ipv4DceRoutingHelper::Create (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this);
  return CreateObject<Ipv4DceRouting> ();
}



} // namespace ns3
