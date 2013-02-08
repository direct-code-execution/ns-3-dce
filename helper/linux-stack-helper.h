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

#include "ns3/object.h"

namespace ns3 {

class Node;
class NodeContainer;
class Time;

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
  /**
   * Aggregate ns3::Ipv4Linux classe onto the provided node.
   * This method will assert if called on a node that
   * already has an Ipv4 object aggregated to it.
   *
   * \param nodeName The name of the node on which to install the stack.
   */
  static void Install (std::string nodeName);

  /**
   * Aggregate ns3::Ipv4Linux classe onto the provided node.
   * This method will assert if called on a node that
   * already has an Ipv4 object aggregated to it.
   *
   * \param node The node on which to install the stack.
   */
  static void Install (Ptr<Node> node);

  /**
   * Aggregate ns3::Ipv4Linux class onto the provided node.
   * This method will assert if called on a node that
   * already has an Ipv4 object aggregated to it.
   *
   * \param c NodeContainer that holds the set of nodes on which to install the
   * new stacks.
   */
  static void Install (NodeContainer c);

  /**
   * Aggregate ns3::Ipv4Linux to all nodes in the simulation
   */
  static void InstallAll (void);

  void SysctlSet (NodeContainer c, std::string path, std::string value);
  static void SysctlGet (Ptr<Node> node, Time at, std::string path,
                         void (*callback)(std::string, std::string));
private:
  static void SysctlGetCallback (Ptr<Node> node, std::string path,
                                 void (*callback)(std::string, std::string));

};

} // namespace ns3

#endif // LINUX_STACK_HELPER_H
