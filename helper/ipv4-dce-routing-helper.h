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

#ifndef IPV4_DCE_ROUTING_HELPER_H
#define IPV4_DCE_ROUTING_HELPER_H

#include "ns3/ipv4-static-routing-helper.h"

namespace ns3 {

/**
 * \brief Helper class that adds ns3::Ipv4DceRouting objects
 *
 * This class is expected to be used in conjunction with
 * ns3::InternetStackHelper::SetRoutingHelper
 */
class Ipv4DceRoutingHelper : public Ipv4StaticRoutingHelper
{
public:
  /*
   * Construct an Ipv4StaticRoutingHelper object, used to make configuration
   * of static routing easier.
   */
  Ipv4DceRoutingHelper ();

  /**
   * \brief Construct an Ipv4StaticRoutingHelper from another previously
   * initialized instance (Copy Constructor).
   */
  Ipv4DceRoutingHelper (const Ipv4DceRoutingHelper &);

  /**
   * \internal
   * \returns pointer to clone of this Ipv4StaticRoutingHelper
   *
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  Ipv4DceRoutingHelper* Copy (void) const;

  /**
   * \param node the node on which the routing protocol will run
   * \returns a newly-created routing protocol
   *
   * This method will be called by ns3::InternetStackHelper::Install
   */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

private:
  /**
   * \internal
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   */
  Ipv4DceRoutingHelper &operator = (const Ipv4DceRoutingHelper &o);
};

} // namespace ns3

#endif /* IPV4_DCE_ROUTING_HELPER_H */
