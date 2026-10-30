/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */
#include "lkl-wifi-helper.h"
#include "ns3/ht-configuration.h"
#include "ns3/vht-configuration.h"
#include "ns3/he-configuration.h"
#include "ns3/lkl-wifi-device.h"
#include "ns3/abort.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"
#include <sstream>

namespace ns3 {

LklWifiHelper::LklWifiHelper ()
  : m_standard (WIFI_STANDARD_80211a)
{
}

void
LklWifiHelper::SetStandard (WifiStandard standard)
{
  NS_ABORT_MSG_IF (standard != WIFI_STANDARD_80211a && standard != WIFI_STANDARD_80211b
                   && standard != WIFI_STANDARD_80211g && standard != WIFI_STANDARD_80211n
                   && standard != WIFI_STANDARD_80211ac && standard != WIFI_STANDARD_80211ax,
                   "The kernel radios have legacy, HT, VHT and HE (single user) rates");
  m_standard = standard;
}

NetDeviceContainer
LklWifiHelper::Install (const WifiPhyHelper &phyHelper, NodeContainer nodes) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<WifiNetDevice> carrier = CreateObject<WifiNetDevice> ();
      carrier->SetStandard (m_standard);
      carrier->SetNode (node);
      // The PHYs of these standards look up their configuration on their
      // device; the kernel radios have two spatial streams.
      if (m_standard >= WIFI_STANDARD_80211n)
        {
          carrier->SetHtConfiguration (CreateObject<HtConfiguration> ());
        }
      if (m_standard >= WIFI_STANDARD_80211ac)
        {
          carrier->SetVhtConfiguration (CreateObject<VhtConfiguration> ());
        }
      if (m_standard >= WIFI_STANDARD_80211ax)
        {
          carrier->SetHeConfiguration (CreateObject<HeConfiguration> ());
        }
      std::vector<Ptr<WifiPhy> > phys = phyHelper.Create (node, carrier);
      NS_ABORT_MSG_IF (phys.size () != 1, "One PHY per radio");
      carrier->SetPhy (phys[0]);
      phys[0]->ConfigureStandard (m_standard);
      if (m_standard >= WIFI_STANDARD_80211n)
        {
          phys[0]->SetNumberOfAntennas (2);
          phys[0]->SetMaxSupportedTxSpatialStreams (2);
          phys[0]->SetMaxSupportedRxSpatialStreams (2);
        }

      Ptr<LklWifiDevice> device = CreateObject<LklWifiDevice> ();
      device->SetAddress (Mac48Address::Allocate ());
      device->SetPhy (phys[0], carrier);
      node->AddDevice (device);
      devices.Add (device);
    }
  return devices;
}

void
LklWifiHelper::EnablePcap (WifiPhyHelper &phy, std::string prefix, NetDeviceContainer devices)
{
  for (NetDeviceContainer::Iterator i = devices.Begin (); i != devices.End (); ++i)
    {
      Ptr<LklWifiDevice> device = DynamicCast<LklWifiDevice> (*i);
      NS_ABORT_MSG_IF (!device, "Not an LklWifiDevice");
      std::ostringstream name;
      name << prefix << "-" << device->GetNode ()->GetId () << "-" << device->GetIfIndex () << ".pcap";
      // The PHY's traces are those of its carrier.
      phy.EnablePcap (name.str (), device->GetCarrier (), true, true);
    }
}

int64_t
LklWifiHelper::AssignStreams (NetDeviceContainer devices, int64_t stream)
{
  int64_t current = stream;
  for (NetDeviceContainer::Iterator i = devices.Begin (); i != devices.End (); ++i)
    {
      Ptr<LklWifiDevice> device = DynamicCast<LklWifiDevice> (*i);
      if (device)
        {
          current += device->AssignStreams (current);
          current += device->GetPhy ()->AssignStreams (current);
        }
    }
  return current - stream;
}

} // namespace ns3
