/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hajime Tazaki
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */
#ifndef QUAGGA_HELPER_H
#define QUAGGA_HELPER_H

#include "dce-manager-helper.h"
#include "dce-application-helper.h"

namespace ns3 {

/**
 * \brief create a quagga routing daemon as an application and associate it to a node
 *
 * This class creates one or multiple instances of ns3::Quagga and associates
 * it/them to one/multiple node(s).
 */
class QuaggaHelper
{
public:
  /**
   * Create a QuaggaHelper which is used to make life easier for people wanting
   * to use quagga Applications.
   *
   */
  QuaggaHelper ();

  /**
   * Install a quagga application on each Node in the provided NodeContainer.
   *
   * \param nodes The NodeContainer containing all of the nodes to get a quagga
   *              application via ProcessManager.
   *
   * \returns A list of quagga applications, one for each input node
   */
  ApplicationContainer Install (NodeContainer nodes);

  /**
   * Install a quagga application on the provided Node.  The Node is specified
   * directly by a Ptr<Node>
   *
   * \param node The node to install the QuaggaApplication on.
   *
   * \returns An ApplicationContainer holding the quagga application created.
   */
  ApplicationContainer Install (Ptr<Node> node);

  /**
   * Install a quagga application on the provided Node.  The Node is specified
   * by a string that must have previosly been associated with a Node using the
   * Object Name Service.
   *
   * \param nodeName The node to install the ProcessApplication on.
   *
   * \returns An ApplicationContainer holding the quagga application created.
   */
  ApplicationContainer Install (std::string nodeName);

  /**
   * \brief Configure ping applications attribute 
   * \param name   attribute's name
   * \param value  attribute's value
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  void EnableOspf (NodeContainer nodes);
  void EnableOspfDebug (NodeContainer nodes);
  void EnableTdpNina (NodeContainer nodes);
  void EnableZebraDebug (NodeContainer nodes);
  void EnableRadvd (Ptr<Node> node, const char *ifname);
  void EnableEgressIF (Ptr<Node> node, const char *ifname);
  void EnableHomeAgentFlag (Ptr<Node> node, const char *ifname);
  void UseManualConfig (NodeContainer nodes);
private:
  /**
   * \internal
   */
  ApplicationContainer InstallPriv (Ptr<Node> node);
  void GenerateConfigZebra (Ptr<Node> node);
  void GenerateConfigOspf (Ptr<Node> node);
};

} // namespace ns3

#endif /* QUAGGA_HELPER_H */
