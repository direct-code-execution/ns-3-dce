/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */
#ifndef LKL_WIFI_HELPER_H
#define LKL_WIFI_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/wifi-standards.h"
#include <string>

namespace ns3 {

class WifiPhyHelper;

/**
 * Give nodes with a Linux kernel stack (KernelSocketFdFactory) a kernel
 * mac80211 radio (LklWifiDevice) on an ns-3 wifi PHY, which can share its
 * channel with ns-3 wifi devices.
 *
 * Install before the kernel boots or after: the radio is wlan<n> in the
 * kernel either way. 802.11a/b/g, n, ac and ax (single user).
 */
class LklWifiHelper
{
public:
  LklWifiHelper ();

  /**
   * Set the standard the PHYs are configured with (WIFI_STANDARD_80211a by
   * default, as for MeshHelper).
   *
   * @param standard the standard
   */
  void SetStandard (WifiStandard standard);

  /**
   * Give each node a radio.
   *
   * @param phy the helper of the PHYs, with their channel
   * @param nodes the nodes
   * @return the LklWifiDevices
   */
  NetDeviceContainer Install (const WifiPhyHelper &phy, NodeContainer nodes) const;

  /**
   * Capture the frames of the radios, as WifiPhyHelper::EnablePcap does for
   * ns-3 wifi devices, in <prefix>-<node>-<device>.pcap.
   *
   * @param phy the helper the PHYs were created with, with its pcap data
   *        link type
   * @param prefix the file name prefix
   * @param devices the LklWifiDevices
   */
  static void EnablePcap (WifiPhyHelper &phy, std::string prefix, NetDeviceContainer devices);

  /**
   * Assign fixed random variable streams to the radios and their PHYs.
   *
   * @param devices the LklWifiDevices
   * @param stream first stream index to use
   * @return the number of stream indices assigned
   */
  static int64_t AssignStreams (NetDeviceContainer devices, int64_t stream);

private:
  WifiStandard m_standard; ///< standard of the PHYs
};

} // namespace ns3

#endif /* LKL_WIFI_HELPER_H */
