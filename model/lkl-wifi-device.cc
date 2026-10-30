/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */
#include "lkl-wifi-device.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-trailer.h"
#include "ns3/snr-tag.h"
#include "ns3/wifi-utils.h"
#include "ns3/wifi-types.h"
#include "ns3/wifi-standard-constants.h"
#include "ns3/dsss-phy.h"
#include "ns3/erp-ofdm-phy.h"
#include "ns3/ofdm-phy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LklWifiDevice");

NS_OBJECT_ENSURE_REGISTERED (LklWifiDevice);

namespace {

const uint32_t MIN_HEADER = 10; // frame control, duration and address 1

} // namespace

TypeId
LklWifiDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LklWifiDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("Dce")
    .AddConstructor<LklWifiDevice> ()
    .AddAttribute ("Mtu", "The MTU of the kernel radio.",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&LklWifiDevice::m_mtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("CwMin", "The minimum contention window (slots).",
                   UintegerValue (15),
                   MakeUintegerAccessor (&LklWifiDevice::m_cwMin),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("CwMax", "The maximum contention window (slots).",
                   UintegerValue (1023),
                   MakeUintegerAccessor (&LklWifiDevice::m_cwMax),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxQueue", "Frames the transmission queue holds; the "
                   "kernel learns of the others as not acknowledged.",
                   UintegerValue (256),
                   MakeUintegerAccessor (&LklWifiDevice::m_maxQueue),
                   MakeUintegerChecker<uint32_t> (1))
    .AddTraceSource ("HeaderMismatch",
                     "A frame of the kernel whose MAC header ns-3 does not "
                     "serialize back to the same bytes: the frame, and the "
                     "MPDU that went on the air in its place.",
                     MakeTraceSourceAccessor (&LklWifiDevice::m_headerMismatchTrace),
                     "ns3::LklWifiDevice::HeaderMismatchCallback")
  ;
  return tid;
}

LklWifiDevice::LklWifiDevice ()
  : m_ifIndex (0),
    m_cw (0),
    m_backoff (0),
    m_idleSlots (0),
    m_sending (false)
{
  NS_LOG_FUNCTION (this);
  m_rng = CreateObject<UniformRandomVariable> ();
}

LklWifiDevice::~LklWifiDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
LklWifiDevice::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  // The carrier is not one of the node's devices: nothing else does it.
  if (m_phy)
    {
      m_phy->Initialize ();
    }
  NetDevice::DoInitialize ();
}

void
LklWifiDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_access.Cancel ();
  m_ackTimeout.Cancel ();
  m_queue.clear ();
  m_events.clear ();
  m_eventCallback = MakeNullCallback<void> ();
  if (m_phy)
    {
      m_phy->SetReceiveOkCallback (MakeNullCallback<void, Ptr<const WifiPsdu>, RxSignalInfo,
                                                    const WifiTxVector &, const std::vector<bool> &> ());
    }
  if (m_carrier)
    {
      m_carrier->Dispose ();
    }
  m_phy = nullptr;
  m_carrier = nullptr;
  m_node = nullptr;
  NetDevice::DoDispose ();
}

void
LklWifiDevice::SetPhy (Ptr<WifiPhy> phy, Ptr<WifiNetDevice> carrier)
{
  NS_LOG_FUNCTION (this << phy << carrier);
  m_phy = phy;
  m_carrier = carrier;
  m_cw = m_cwMin;
  m_phy->SetReceiveOkCallback (MakeCallback (&LklWifiDevice::Receive, this));
}

Ptr<WifiPhy>
LklWifiDevice::GetPhy (void) const
{
  return m_phy;
}

Ptr<WifiNetDevice>
LklWifiDevice::GetCarrier (void) const
{
  return m_carrier;
}

int64_t
LklWifiDevice::AssignStreams (int64_t stream)
{
  m_rng->SetStream (stream);
  return 1;
}

void
LklWifiDevice::SetEventCallback (Callback<void> callback)
{
  m_eventCallback = callback;
}

bool
LklWifiDevice::PopEvent (Event &event)
{
  if (m_events.empty ())
    {
      return false;
    }
  event = m_events.front ();
  m_events.pop_front ();
  return true;
}

bool
LklWifiDevice::HasEvents (void) const
{
  return !m_events.empty ();
}

void
LklWifiDevice::Notify (const Event &event)
{
  m_events.push_back (event);
  if (!m_eventCallback.IsNull ())
    {
      m_eventCallback ();
    }
}

WifiTxVector
LklWifiDevice::GetTxVector (uint16_t rate) const
{
  WifiMode mode;
  bool erp = m_phy->GetPhyBand () == WIFI_PHY_BAND_2_4GHZ
    && m_phy->GetStandard () != WIFI_STANDARD_80211b;
  switch (rate)
    {
    case 10: mode = DsssPhy::GetDsssRate1Mbps (); break;
    case 20: mode = DsssPhy::GetDsssRate2Mbps (); break;
    case 55: mode = DsssPhy::GetDsssRate5_5Mbps (); break;
    case 110: mode = DsssPhy::GetDsssRate11Mbps (); break;
    case 60: mode = erp ? ErpOfdmPhy::GetErpOfdmRate6Mbps () : OfdmPhy::GetOfdmRate6Mbps (); break;
    case 90: mode = erp ? ErpOfdmPhy::GetErpOfdmRate9Mbps () : OfdmPhy::GetOfdmRate9Mbps (); break;
    case 120: mode = erp ? ErpOfdmPhy::GetErpOfdmRate12Mbps () : OfdmPhy::GetOfdmRate12Mbps (); break;
    case 180: mode = erp ? ErpOfdmPhy::GetErpOfdmRate18Mbps () : OfdmPhy::GetOfdmRate18Mbps (); break;
    case 240: mode = erp ? ErpOfdmPhy::GetErpOfdmRate24Mbps () : OfdmPhy::GetOfdmRate24Mbps (); break;
    case 360: mode = erp ? ErpOfdmPhy::GetErpOfdmRate36Mbps () : OfdmPhy::GetOfdmRate36Mbps (); break;
    case 480: mode = erp ? ErpOfdmPhy::GetErpOfdmRate48Mbps () : OfdmPhy::GetOfdmRate48Mbps (); break;
    case 540: mode = erp ? ErpOfdmPhy::GetErpOfdmRate54Mbps () : OfdmPhy::GetOfdmRate54Mbps (); break;
    default: NS_FATAL_ERROR ("No legacy rate of " << rate * 100 << " kbps");
    }
  return WifiTxVector (mode, WIFI_MIN_TX_PWR_LEVEL, WIFI_PREAMBLE_LONG, NanoSeconds (800), 1, 1, 0,
                       MHz_u{20}, false);
}

Ptr<WifiPsdu>
LklWifiDevice::MakePsdu (const std::vector<uint8_t> &frame) const
{
  if (frame.size () < MIN_HEADER)
    {
      return nullptr;
    }
  Ptr<Packet> p = Create<Packet> (frame.data (), frame.size ());
  WifiMacHeader hdr;
  uint32_t size = p->RemoveHeader (hdr);
  if (size == 0 || size > frame.size ())
    {
      return nullptr;
    }
  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (p, hdr);

  // The frame on the air is ns-3's serialization of Linux's: check that
  // they are the same.
  Ptr<Packet> mpdu = p->Copy ();
  mpdu->AddHeader (hdr);
  std::vector<uint8_t> sent (mpdu->GetSize ());
  mpdu->CopyData (sent.data (), sent.size ());
  if (sent != frame)
    {
      NS_LOG_WARN ("ns-3 serializes the header of " << hdr << " differently");
      m_headerMismatchTrace (frame, mpdu);
    }
  return psdu;
}

void
LklWifiDevice::Transmit (const std::vector<uint8_t> &frame, const TxInfo &info)
{
  NS_LOG_FUNCTION (this << frame.size ());
  bool valid = info.rate[0] != 0
    && info.freq == static_cast<uint16_t> (m_phy->GetFrequency ())
    && frame.size () >= MIN_HEADER;
  if (!valid || m_queue.size () >= m_maxQueue)
    {
      if (info.freq != static_cast<uint16_t> (m_phy->GetFrequency ()))
        {
          NS_LOG_WARN ("the kernel sends on " << info.freq << " MHz, the PHY is on "
                       << m_phy->GetFrequency () << " MHz: frame dropped");
        }
      NS_LOG_DEBUG ("dropping a frame for " << info.freq << " MHz, queue " << m_queue.size ());
      if (!info.noStatus)
        {
          Event event = {};
          event.type = Event::TX_STATUS;
          event.acked = false;
          event.attempts = 0;
          Notify (event);
        }
      return;
    }
  Pending pending;
  pending.frame = frame;
  pending.info = info;
  pending.series = 0;
  pending.tries = 0;
  pending.attempts = 0;
  m_queue.push_back (pending);
  if (m_queue.size () == 1 && !m_sending)
    {
      StartAccess ();
    }
}

void
LklWifiDevice::StartAccess (void)
{
  NS_LOG_FUNCTION (this);
  m_backoff = m_rng->GetInteger (0, m_cw);
  m_idleSlots = 0;
  m_access.Cancel ();
  m_access = Simulator::Schedule (m_phy->GetSlot (), &LklWifiDevice::AccessSlot, this);
}

void
LklWifiDevice::AccessSlot (void)
{
  // DIFS (SIFS and two slots, counted as three slots: close enough at the
  // granularity of contention), then the backoff, over idle slots only.
  const uint32_t difs = 3;
  if (!m_phy->IsStateIdle ())
    {
      m_idleSlots = 0;
    }
  else if (++m_idleSlots > difs)
    {
      if (m_backoff == 0)
        {
          SendHead ();
          return;
        }
      m_backoff--;
    }
  m_access = Simulator::Schedule (m_phy->GetSlot (), &LklWifiDevice::AccessSlot, this);
}

void
LklWifiDevice::SendHead (void)
{
  Pending &head = m_queue.front ();
  if (head.attempts > 0)
    {
      // Like the hardware: the Retry bit of the frame control field, by
      // which receivers drop the duplicates of frames they acknowledged.
      head.frame[1] |= 0x08;
    }
  WifiTxVector txVector = GetTxVector (head.info.rate[head.series]);
  Ptr<WifiPsdu> psdu = MakePsdu (head.frame);
  if (!psdu)
    {
      NS_LOG_WARN ("cannot parse a frame of the kernel: dropped");
      FinishHead (false);
      return;
    }
  head.tries++;
  head.attempts++;
  NS_LOG_DEBUG ("tx " << *psdu << " at " << txVector.GetMode () << " attempt " << +head.attempts);
  m_sending = true;
  m_phy->Send (psdu, txVector);

  bool needsAck = !head.info.noAck && !psdu->GetAddr1 ().IsGroup ()
    && !psdu->GetHeader (0).IsCtl ()
    && !(psdu->GetHeader (0).IsQosData () && psdu->GetHeader (0).IsQosNoAck ());
  Time duration = WifiPhy::CalculateTxDuration (psdu->GetSize (), txVector, m_phy->GetPhyBand ());
  if (!needsAck)
    {
      m_ackTimeout = Simulator::Schedule (duration, &LklWifiDevice::FinishHead, this, true);
      return;
    }
  // The ACK starts SIFS after the frame: allow a slot and the detection of
  // its preamble, then for its reception if it started.
  Time timeout = duration + m_phy->GetSifs () + m_phy->GetSlot ()
    + WifiPhy::GetPreambleDetectionDuration ();
  m_ackTimeout = Simulator::Schedule (timeout, &LklWifiDevice::AckTimeout, this);
}

void
LklWifiDevice::AckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (m_phy->IsStateRx () || m_phy->IsStateCcaBusy ())
    {
      // Something is being received (its preamble or header while CCA
      // busy), maybe the ACK: wait for its end.
      m_ackTimeout = Simulator::Schedule (m_phy->GetSlot (), &LklWifiDevice::AckTimeout, this);
      return;
    }
  Pending &head = m_queue.front ();
  if (head.tries >= head.info.count[head.series])
    {
      head.series++;
      head.tries = 0;
    }
  if (head.series >= 4 || head.info.rate[head.series] == 0)
    {
      FinishHead (false);
      return;
    }
  m_sending = false;
  m_cw = std::min (2 * m_cw + 1, m_cwMax);
  StartAccess ();
}

void
LklWifiDevice::FinishHead (bool acked)
{
  NS_LOG_FUNCTION (this << acked);
  m_ackTimeout.Cancel ();
  Pending head = m_queue.front ();
  m_queue.pop_front ();
  m_sending = false;
  m_cw = m_cwMin;
  if (!head.info.noStatus)
    {
      Event event = {};
      event.type = Event::TX_STATUS;
      event.acked = acked;
      event.attempts = head.attempts;
      Notify (event);
    }
  if (!m_queue.empty ())
    {
      // Post-backoff.
      StartAccess ();
    }
}

void
LklWifiDevice::Receive (Ptr<const WifiPsdu> psdu, RxSignalInfo rxSignalInfo,
                        const WifiTxVector &txVector, const std::vector<bool> &statusPerMpdu)
{
  NS_LOG_FUNCTION (this << *psdu << rxSignalInfo.rssi);
  for (std::size_t i = 0; i < psdu->GetNMpdus (); i++)
    {
      if (!statusPerMpdu.at (i))
        {
          continue;
        }
      const WifiMacHeader &hdr = psdu->GetHeader (i);
      if (hdr.IsAck () && m_sending && m_ackTimeout.IsPending () && hdr.GetAddr1 () == m_address)
        {
          FinishHead (true);
        }
      // Control frames go to the kernel too, like the frames of a card that
      // passes them (FIF_CONTROL): its monitor interfaces show them.
      if (hdr.GetAddr1 () == m_address && !hdr.IsCtl ()
          && !(hdr.IsQosData () && hdr.IsQosNoAck ()))
        {
          Simulator::Schedule (m_phy->GetSifs (), &LklWifiDevice::SendAck, this,
                               hdr.GetAddr2 (), txVector, rxSignalInfo.snr);
        }
      if (psdu->IsAggregate ())
        {
          continue; // only legacy rates: never aggregated
        }
      Ptr<Packet> mpdu = psdu->GetPacket ()->Copy ();
      WifiMacTrailer fcs;
      mpdu->RemoveTrailer (fcs);
      Event event = {};
      event.type = Event::RX;
      event.frame.resize (mpdu->GetSize ());
      mpdu->CopyData (event.frame.data (), event.frame.size ());
      event.freq = static_cast<uint16_t> (m_phy->GetFrequency ());
      event.rate = txVector.GetMode ().GetDataRate (MHz_u{20}) / 100000;
      event.signal = static_cast<int32_t> (rxSignalInfo.rssi);
      Notify (event);
    }
}

void
LklWifiDevice::SendAck (Mac48Address to, WifiTxVector rxVector, double snr)
{
  if (m_phy->IsStateTx ())
    {
      return;
    }
  WifiMacHeader ack;
  ack.SetType (WIFI_MAC_CTL_ACK);
  ack.SetAddr1 (to);
  ack.SetDsNotFrom ();
  ack.SetDsNotTo ();
  ack.SetNoRetry ();
  ack.SetNoMoreFragments ();
  ack.SetDuration (Seconds (0));
  // The highest basic rate of the frame's modulation not above its rate.
  uint16_t rate = rxVector.GetMode ().GetDataRate (MHz_u{20}) / 100000;
  uint16_t ackRate;
  if (rxVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_DSSS
      || rxVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
    {
      ackRate = rate >= 20 ? 20 : 10;
    }
  else
    {
      ackRate = rate >= 240 ? 240 : rate >= 120 ? 120 : 60;
    }
  // Like ns-3 receivers, tell the SNR of the frame to the rate managers
  // of ns-3 transmitters that use it (e.g. IdealWifiManager).
  Ptr<Packet> packet = Create<Packet> ();
  SnrTag tag;
  tag.Set (snr);
  packet->AddPacketTag (tag);
  m_phy->Send (Create<WifiPsdu> (packet, ack), GetTxVector (ackRate));
}

void
LklWifiDevice::SetIfIndex (const uint32_t index)
{
  m_ifIndex = index;
}

uint32_t
LklWifiDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
LklWifiDevice::GetChannel (void) const
{
  return m_phy ? m_phy->GetChannel () : nullptr;
}

void
LklWifiDevice::SetAddress (Address address)
{
  m_address = Mac48Address::ConvertFrom (address);
}

Address
LklWifiDevice::GetAddress (void) const
{
  return m_address;
}

bool
LklWifiDevice::SetMtu (const uint16_t mtu)
{
  m_mtu = mtu;
  return true;
}

uint16_t
LklWifiDevice::GetMtu (void) const
{
  return m_mtu;
}

bool
LklWifiDevice::IsLinkUp (void) const
{
  return bool (m_phy);
}

void
LklWifiDevice::AddLinkChangeCallback (Callback<void> callback)
{
}

bool
LklWifiDevice::IsBroadcast (void) const
{
  return true;
}

Address
LklWifiDevice::GetBroadcast (void) const
{
  return Mac48Address::GetBroadcast ();
}

bool
LklWifiDevice::IsMulticast (void) const
{
  return true;
}

Address
LklWifiDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  return Mac48Address::GetMulticast (multicastGroup);
}

Address
LklWifiDevice::GetMulticast (Ipv6Address addr) const
{
  return Mac48Address::GetMulticast (addr);
}

bool
LklWifiDevice::IsBridge (void) const
{
  return false;
}

bool
LklWifiDevice::IsPointToPoint (void) const
{
  return false;
}

bool
LklWifiDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_WARN ("the kernel's frames do not go through Send");
  return false;
}

bool
LklWifiDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                         uint16_t protocolNumber)
{
  NS_LOG_WARN ("the kernel's frames do not go through SendFrom");
  return false;
}

Ptr<Node>
LklWifiDevice::GetNode (void) const
{
  return m_node;
}

void
LklWifiDevice::SetNode (Ptr<Node> node)
{
  m_node = node;
}

bool
LklWifiDevice::NeedsArp (void) const
{
  return true;
}

void
LklWifiDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
}

void
LklWifiDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
}

bool
LklWifiDevice::SupportsSendFrom (void) const
{
  return false;
}

} // namespace ns3
