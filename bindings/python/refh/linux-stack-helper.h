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
#ifndef LINUX_STACK_HELPER_H
#define LINUX_STACK_HELPER_H

//#include "ns3/object.h"
#include <string>
#include <stdint.h>
#include <vector>

namespace ns3 {

class Node;
class NodeContainer;
class Time;
class Ipv4RoutingHelper;

template <class T>
class Ptr;

/**
 * \brief aggregate Ipv4Linux to nodes
 *
 * Today the first goal of the Ipv4Linux is to be able to use the standards NS-3 Helpers for two tasks:
 *   1 - assign address to devices using Ipv4AddressHelper,
 *   2 - create the static routes using Ipv4GlobalRoutingHelper
 *
 *
 */
class LinuxStackHelper
{
public:

  LinuxStackHelper ();
  ~LinuxStackHelper ();

  /**
   * Aggregate ns3::Ipv4Linux classe onto the provided node.
   * This method will assert if called on a node that
   * already has an Ipv4 object aggregated to it.
   *
   * \param nodeName The name of the node on which to install the stack.
   */
  //void Install (std::string nodeName);

  /**
   * Aggregate ns3::Ipv4Linux classe onto the provided node.
   * This method will assert if called on a node that
   * already has an Ipv4 object aggregated to it.
   *
   * \param node The node on which to install the stack.
   */
  void Install (Ptr<Node> node);

  /**
   * Aggregate ns3::Ipv4Linux class onto the provided node.
   * This method will assert if called on a node that
   * already has an Ipv4 object aggregated to it.
   *
   * \param c NodeContainer that holds the set of nodes on which to install the
   * new stacks.
   */
  void Install (NodeContainer c);

  /**
   * Aggregate ns3::Ipv4Linux to all nodes in the simulation
   */
  void InstallAll (void);

  /**
   * \param routing a new routing helper
   *
   * Set the routing helper to use during Install. The routing
   * helper is really an object factory which is used to create 
   * an object of type ns3::Ipv4RoutingProtocol per node. This routing
   * object is then associated to a single ns3::Ipv4 object through its 
   * ns3::Ipv4::SetRoutingProtocol.
   */
  void SetRoutingHelper (const Ipv4RoutingHelper &routing);

  /**
   * Configure Linux kernel parameters with traditional 'sysctl' interface.
   *
   * \param c NodeContainer that holds the set of nodes to configure this parameter.
   * \param path a string value for sysctl parameter. it starts from '.' following the name of parameter.
   *             e.g., ".net.ipv4.conf.default.forwarding"
   * \param value a string value to set for sysctl parameter.
   */
  void SysctlSet (NodeContainer c, std::string path, std::string value);

  /**
   * Obtain Linux kernel state with traditional 'sysctl' interface.
   *
   * \param node The node pointer Ptr<Node> that will ask the status.
   * \param at the delta from the begining of simulation to ask this query.
   * \param path a string value for sysctl parameter. it starts from '.' following the name of parameter.
   *             e.g., ".net.ipv4.conf.default.forwarding"
   * \param callback a callback function to parse the result of sysctl query.
   */
  static void SysctlGet (Ptr<Node> node, Time at, std::string path,
                         void (*callback)(std::string, std::string));

  /**
   * Populate routing information to all the nodes in network from GlobalRoutingTable.
   *
   * Limitation:
   * 1) This method SHOULD call after Ipv4GlobalRoutingHelper::PopulateRoutingTables () so that
   * LinuxStackHelper can obtain the route information.
   * 2) This feature is only available for IPv4 route information. IPv6 is not implemented.
   *
   */
  static void PopulateRoutingTables ();

  /**
   * Execute "ip" command (of Linux) on a specific node to configure the ip address/route/etc information.
   *
   * \param node The node pointer Ptr<Node> to configure.
   * \param at the delta from the begining of simulation to execute this command.
   * \param str a string for the command line argument of ip command. e.g., "route add 10.0.1.0/24 via 10.0.0.1"
   */
  static void RunIp (Ptr<Node> node, Time at, std::string str);


};

} // namespace ns3

#endif // LINUX_STACK_HELPER_H
