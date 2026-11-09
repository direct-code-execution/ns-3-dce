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
#include "ns3/boolean.h"
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
#include "ns3/ht-phy.h"
#include "ns3/vht-phy.h"
#include "ns3/he-phy.h"
#include "ns3/ctrl-headers.h"
#include "ns3/wifi-mpdu.h"

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
    .AddAttribute ("WmmFromEdca",
                   "Give the kernel the QoS (EDCA) parameters of the beacons, probe "
                   "responses and association responses of ns-3 APs also as the WMM "
                   "Parameter element, which mac80211 requires for QoS, HT, VHT and HE "
                   "while ns-3 sends only the EDCA Parameter Set element: as if the AP "
                   "sent both, as real APs do.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LklWifiDevice::m_wmmFromEdca),
                   MakeBooleanChecker ())
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
    m_sending (false),
    m_expectBlockAck (false),
    m_ampduRef (0),
    m_wmmFromEdca (false),
    m_rxAmpduOpen (false)
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
  m_inflight.clear ();
  m_scoreboards.clear ();
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
LklWifiDevice::GetTxVector (uint16_t rate, MHz_u width) const
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
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS
      || mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
    {
      width = MHz_u{22};
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
    {
      width = MHz_u{20}; // no non-HT duplicate at 2.4 GHz
    }
  else
    {
      width = std::min (width, m_phy->GetChannelWidth ());
    }
  return WifiTxVector (mode, WIFI_MIN_TX_PWR_LEVEL, WIFI_PREAMBLE_LONG, NanoSeconds (800), 1, 1, 0,
                       width, false);
}

WifiTxVector
LklWifiDevice::GetTxVector (const TxInfo &info, uint8_t series) const
{
  uint8_t modeType = info.mode[series];
  if (modeType == LEGACY)
    {
      return GetTxVector (info.rate[series]);
    }
  MHz_u width = std::min (MHz_u{info.width[series] ? info.width[series] : 20u},
                          m_phy->GetChannelWidth ());
  uint8_t nss = std::max<uint8_t> (1, std::min (info.nss[series],
                                                m_phy->GetMaxSupportedTxSpatialStreams ()));
  // The width or number of streams may have been clamped to the PHY's:
  // lower the MCS until the combination is allowed (e.g. VHT MCS 9 on one
  // stream at 20 MHz).
  for (int mcs = info.mcs[series]; ; mcs--)
    {
      WifiMode mode;
      WifiPreamble preamble;
      uint16_t gi = info.sgi[series] ? 400 : 800;
      switch (modeType)
        {
        case HT:
          mode = HtPhy::GetHtMcs (std::min (mcs, 7) + 8 * (nss - 1));
          preamble = WIFI_PREAMBLE_HT_MF;
          break;
        case VHT:
          mode = VhtPhy::GetVhtMcs (std::min (mcs, 9));
          preamble = WIFI_PREAMBLE_VHT_SU;
          break;
        default:
          mode = HePhy::GetHeMcs (std::min (mcs, 11));
          preamble = WIFI_PREAMBLE_HE_SU;
          gi = 800;
          break;
        }
      WifiTxVector txVector (mode, WIFI_MIN_TX_PWR_LEVEL, preamble, NanoSeconds (gi), nss, nss, 0,
                             width, false);
      if (txVector.IsValid (m_phy->GetPhyBand ()) || mcs == 0)
        {
          return txVector;
        }
    }
}

WifiTxVector
LklWifiDevice::GetResponseTxVector (const WifiTxVector &rxVector, MHz_u width) const
{
  WifiMode mode = rxVector.GetMode ();
  uint16_t rate;
  if (mode.GetModulationClass () == WIFI_MOD_CLASS_DSSS
      || mode.GetModulationClass () == WIFI_MOD_CLASS_HR_DSSS)
    {
      rate = mode.GetDataRate (MHz_u{22}) >= 2000000 ? 20 : 10;
    }
  else if (mode.GetModulationClass () == WIFI_MOD_CLASS_OFDM
           || mode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
    {
      uint64_t r = mode.GetDataRate (MHz_u{20}) / 100000;
      rate = r >= 240 ? 240 : r >= 120 ? 120 : 60;
    }
  else
    {
      // HT/VHT/HE: by the constellation (BPSK, QPSK, then 16-QAM or above).
      uint16_t constellation = mode.GetConstellationSize ();
      rate = constellation <= 2 ? 60 : constellation <= 4 ? 120 : 240;
    }
  return GetTxVector (rate, width);
}

Ptr<WifiMpdu>
LklWifiDevice::MakeMpdu (const std::vector<uint8_t> &frame) const
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
  return Create<WifiMpdu> (p, hdr);
}

void
LklWifiDevice::Transmit (const std::vector<uint8_t> &frame, const TxInfo &info)
{
  NS_LOG_FUNCTION (this << frame.size ());
  bool valid = info.count[0] != 0
    && info.freq == PrimaryFrequency ()
    && frame.size () >= MIN_HEADER;
  if (!valid || m_queue.size () >= m_maxQueue)
    {
      if (info.freq != PrimaryFrequency ())
        {
          NS_LOG_WARN ("the kernel sends on " << info.freq << " MHz, the PHY is on "
                       << PrimaryFrequency () << " MHz: frame dropped");
        }
      NS_LOG_DEBUG ("dropping a frame for " << info.freq << " MHz, queue " << m_queue.size ());
      if (!info.noStatus)
        {
          // Report it in order, after the frames before it.
          Pending dropped = {};
          dropped.info = info;
          dropped.done = true;
          dropped.acked = false;
          m_queue.push_back (dropped);
          if (!m_sending)
            {
              Flush ();
            }
        }
      return;
    }
  Pending pending = {};
  pending.frame = frame;
  pending.info = info;
  m_queue.push_back (pending);
  if (!m_sending && !m_access.IsPending ())
    {
      StartAccess ();
    }
}

uint16_t
LklWifiDevice::PrimaryFrequency (void) const
{
  // The kernel knows the channel by its primary 20 MHz channel.
  return static_cast<uint16_t> (
    m_phy->GetOperatingChannel ().GetPrimaryChannelCenterFrequency (MHz_u{20}));
}

std::size_t
LklWifiDevice::FirstPending (void) const
{
  for (std::size_t i = 0; i < m_queue.size (); i++)
    {
      if (!m_queue[i].done)
        {
          return i;
        }
    }
  return m_queue.size ();
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
  std::size_t first = FirstPending ();
  if (first == m_queue.size ())
    {
      return;
    }
  Pending &head = m_queue[first];
  WifiTxVector txVector = GetTxVector (head.info, head.series);
  Ptr<WifiMpdu> mpdu = MakeMpdu (head.frame);
  if (!mpdu)
    {
      NS_LOG_WARN ("cannot parse a frame of the kernel: dropped");
      head.done = true;
      head.acked = false;
      Flush ();
      return;
    }
  m_inflight.assign (1, first);
  std::vector<Ptr<WifiMpdu>> mpdus (1, mpdu);
  const WifiMacHeader &hdr = mpdu->GetHeader ();

  // Aggregate the frames that follow for the same receiver and TID, while
  // the kernel lets it (a Block Ack session) and the PPDU is short enough.
  if (head.info.ampdu && hdr.IsQosData () && !hdr.GetAddr1 ().IsGroup ()
      && txVector.GetModulationClass () >= WIFI_MOD_CLASS_HT)
    {
      const Time maxDuration = MicroSeconds (5484); // aPPDUMaxTime
      const uint32_t maxLength = 65535;
      for (std::size_t i = first + 1; i < m_queue.size () && mpdus.size () < 64; i++)
        {
          Pending &next = m_queue[i];
          if (next.done)
            {
              continue;
            }
          Ptr<WifiMpdu> candidate = next.info.ampdu ? MakeMpdu (next.frame) : nullptr;
          if (!candidate || !candidate->GetHeader ().IsQosData ()
              || candidate->GetHeader ().GetAddr1 () != hdr.GetAddr1 ()
              || candidate->GetHeader ().GetQosTid () != hdr.GetQosTid ())
            {
              break;
            }
          mpdus.push_back (candidate);
          Ptr<WifiPsdu> trial = Create<WifiPsdu> (mpdus);
          if (trial->GetSize () > maxLength
              || WifiPhy::CalculateTxDuration (trial->GetSize (), txVector, m_phy->GetPhyBand ())
                   > maxDuration)
            {
              mpdus.pop_back ();
              break;
            }
          m_inflight.push_back (i);
        }
    }

  for (std::size_t i = 0; i < m_inflight.size (); i++)
    {
      Pending &p = m_queue[m_inflight[i]];
      if (p.attempts > 0)
        {
          // Like the hardware: the Retry bit of the frame control field, by
          // which receivers drop the duplicates of frames they acknowledged.
          p.frame[1] |= 0x08;
          mpdus[i]->GetHeader ().SetRetry ();
        }
      p.tries++;
      p.attempts++;
    }

  Ptr<WifiPsdu> psdu;
  if (mpdus.size () > 1)
    {
      psdu = Create<WifiPsdu> (mpdus);
    }
  else if (txVector.GetModulationClass () >= WIFI_MOD_CLASS_VHT)
    {
      psdu = Create<WifiPsdu> (mpdu, true); // VHT and HE PPDUs carry an S-MPDU
    }
  else
    {
      psdu = Create<WifiPsdu> (mpdu->GetPacket (), hdr);
    }
  txVector.SetAggregation (psdu->IsAggregate ()); // S-MPDUs too
  NS_LOG_DEBUG ("tx " << *psdu << " at " << txVector << " attempt " << +head.attempts);
  m_sending = true;
  m_expectBlockAck = mpdus.size () > 1 || hdr.IsBlockAckReq ();
  m_phy->Send (psdu, txVector);

  bool needsAck = !head.info.noAck && !psdu->GetAddr1 ().IsGroup ()
    && (!hdr.IsCtl () || hdr.IsBlockAckReq ())
    && !(mpdus.size () == 1 && hdr.IsQosData () && hdr.IsQosNoAck ());
  Time duration = WifiPhy::CalculateTxDuration (psdu->GetSize (), txVector, m_phy->GetPhyBand ());
  if (!needsAck)
    {
      m_ackTimeout = Simulator::Schedule (duration, [this] () {
        for (std::size_t i : m_inflight)
          {
            m_queue[i].done = true;
            m_queue[i].acked = true;
          }
        m_inflight.clear ();
        m_sending = false;
        m_cw = m_cwMin;
        Flush ();
      });
      return;
    }
  // The response starts SIFS after the frame: allow a slot and the
  // detection of its preamble, then for its reception if it started.
  Time timeout = duration + m_phy->GetSifs () + m_phy->GetSlot ()
    + WifiPhy::GetPreambleDetectionDuration ();
  m_ackTimeout = Simulator::Schedule (timeout, &LklWifiDevice::AckTimeout, this);
}

void
LklWifiDevice::Failed (Pending &pending)
{
  if (pending.tries >= pending.info.count[pending.series])
    {
      pending.series++;
      pending.tries = 0;
    }
  if (pending.series >= 4 || pending.info.count[pending.series] == 0)
    {
      pending.done = true;
      pending.acked = false;
    }
}

void
LklWifiDevice::AckTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (m_phy->IsStateRx () || m_phy->IsStateCcaBusy ())
    {
      // Something is being received (its preamble or header while CCA
      // busy), maybe the response: wait for its end.
      m_ackTimeout = Simulator::Schedule (m_phy->GetSlot (), &LklWifiDevice::AckTimeout, this);
      return;
    }
  for (std::size_t i : m_inflight)
    {
      Failed (m_queue[i]);
    }
  m_inflight.clear ();
  m_sending = false;
  m_cw = std::min (2 * m_cw + 1, m_cwMax);
  Flush ();
}

void
LklWifiDevice::FinishHead (bool acked)
{
  NS_LOG_FUNCTION (this << acked);
  m_ackTimeout.Cancel ();
  for (std::size_t i : m_inflight)
    {
      if (acked)
        {
          m_queue[i].done = true;
          m_queue[i].acked = true;
        }
      else
        {
          Failed (m_queue[i]);
        }
    }
  m_inflight.clear ();
  m_sending = false;
  m_cw = acked ? m_cwMin : std::min (2 * m_cw + 1, m_cwMax);
  Flush ();
}

void
LklWifiDevice::Flush (void)
{
  // The kernel takes the outcomes in the order it gave the frames.
  while (!m_queue.empty () && m_queue.front ().done)
    {
      Pending head = m_queue.front ();
      m_queue.pop_front ();
      if (!head.info.noStatus)
        {
          Event event = {};
          event.type = Event::TX_STATUS;
          event.acked = head.acked;
          event.attempts = head.attempts;
          Notify (event);
        }
    }
  if (FirstPending () < m_queue.size ())
    {
      // Post-backoff, or the next attempt.
      StartAccess ();
    }
}

void
LklWifiDevice::ReceiveBlockAck (Ptr<const WifiMpdu> mpdu)
{
  NS_LOG_FUNCTION (this);
  m_ackTimeout.Cancel ();
  CtrlBAckResponseHeader ba;
  mpdu->GetPacket ()->Copy ()->RemoveHeader (ba);
  bool any = false;
  for (std::size_t i : m_inflight)
    {
      Pending &p = m_queue[i];
      WifiMacHeader hdr;
      Create<Packet> (p.frame.data (), p.frame.size ())->RemoveHeader (hdr);
      // The BlockAck that answers a Block Ack request is its acknowledgment.
      bool acked = hdr.IsBlockAckReq ()
        || (hdr.IsQosData () && ba.GetTidInfo () == hdr.GetQosTid ()
            && ba.IsPacketReceived (hdr.GetSequenceNumber ()));
      if (acked)
        {
          p.done = true;
          p.acked = true;
          any = true;
        }
      else
        {
          Failed (p);
        }
    }
  m_inflight.clear ();
  m_sending = false;
  m_cw = any ? m_cwMin : std::min (2 * m_cw + 1, m_cwMax);
  Flush ();
}

void
LklWifiDevice::Record (Mac48Address ta, uint8_t tid, uint16_t seq)
{
  auto it = m_scoreboards.find (std::make_pair (ta, tid));
  if (it == m_scoreboards.end ())
    {
      it = m_scoreboards.insert (std::make_pair (std::make_pair (ta, tid), Scoreboard {seq, {}})).first;
    }
  Scoreboard &sb = it->second;
  uint16_t distance = (seq - sb.winStart) & 0xfff;
  if (distance >= 2048)
    {
      return; // older than the window
    }
  if (distance >= 64)
    {
      // Move the window so that it ends with this frame.
      sb.winStart = (seq - 63) & 0xfff;
    }
  sb.received.insert (seq);
  for (auto i = sb.received.begin (); i != sb.received.end ();)
    {
      i = ((*i - sb.winStart) & 0xfff) >= 64 ? sb.received.erase (i) : std::next (i);
    }
}

void
LklWifiDevice::AddSnrTag (Ptr<Packet> packet, double snr)
{
  // Like ns-3 receivers, tell the SNR of the frame answered to the rate
  // managers of ns-3 transmitters that use it (e.g. IdealWifiManager).
  SnrTag tag;
  tag.Set (snr);
  packet->AddPacketTag (tag);
}

void
LklWifiDevice::SendBlockAck (Mac48Address to, uint8_t tid, WifiTxVector rxVector, double snr)
{
  if (m_phy->IsStateTx ())
    {
      return;
    }
  auto it = m_scoreboards.find (std::make_pair (to, tid));
  CtrlBAckResponseHeader ba;
  ba.SetType (BlockAckType (BlockAckType::COMPRESSED));
  ba.SetTidInfo (tid);
  ba.SetStartingSequence (it != m_scoreboards.end () ? it->second.winStart : 0);
  if (it != m_scoreboards.end ())
    {
      for (uint16_t seq : it->second.received)
        {
          ba.SetReceivedPacket (seq);
        }
    }
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKRESP);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_address);
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();
  hdr.SetDuration (Seconds (0));
  Ptr<Packet> p = Create<Packet> ();
  p->AddHeader (ba);
  AddSnrTag (p, snr);
  m_phy->Send (Create<WifiPsdu> (p, hdr), GetResponseTxVector (rxVector, MHz_u{20}));
}

void
LklWifiDevice::SendCts (Mac48Address to, Time duration, WifiTxVector rxVector, double snr)
{
  if (m_phy->IsStateTx ())
    {
      return;
    }
  // On the width of the RTS (non-HT duplicate), which tells the originator
  // the width it may use.
  WifiTxVector txVector = GetResponseTxVector (rxVector, rxVector.GetChannelWidth ());
  WifiMacHeader cts;
  cts.SetType (WIFI_MAC_CTL_CTS);
  cts.SetAddr1 (to);
  cts.SetDsNotFrom ();
  cts.SetDsNotTo ();
  cts.SetNoRetry ();
  cts.SetNoMoreFragments ();
  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (Create<Packet> (), cts);
  Time remaining = duration - m_phy->GetSifs ()
    - WifiPhy::CalculateTxDuration (psdu->GetSize (), txVector, m_phy->GetPhyBand ());
  cts.SetDuration (std::max (remaining, Seconds (0)));
  Ptr<Packet> packet = Create<Packet> ();
  AddSnrTag (packet, snr);
  m_phy->Send (Create<WifiPsdu> (packet, cts), txVector);
}

void
LklWifiDevice::Receive (Ptr<const WifiPsdu> psdu, RxSignalInfo rxSignalInfo,
                        const WifiTxVector &txVector, const std::vector<bool> &statusPerMpdu)
{
  NS_LOG_FUNCTION (this << *psdu << rxSignalInfo.rssi);
  // The PHY gives each MPDU of an A-MPDU received correctly on its own
  // (without status), then the A-MPDU with the status of each MPDU.
  bool aggregate = statusPerMpdu.empty ();
  if (aggregate && !m_rxAmpduOpen)
    {
      m_rxAmpduOpen = true;
      ++m_ampduRef;
    }
  if (!aggregate && psdu->GetNMpdus () > 1)
    {
      // The end of an A-MPDU, whose MPDUs were given: one BlockAck for those
      // for us that want an acknowledgment.
      m_rxAmpduOpen = false;
      for (std::size_t i = 0; i < psdu->GetNMpdus (); i++)
        {
          const WifiMacHeader &hdr = psdu->GetHeader (i);
          if (statusPerMpdu.at (i) && hdr.GetAddr1 () == m_address && hdr.IsQosData ()
              && !hdr.IsQosNoAck ())
            {
              Simulator::Schedule (m_phy->GetSifs (), &LklWifiDevice::SendBlockAck, this,
                                   hdr.GetAddr2 (), hdr.GetQosTid (), txVector,
                                   rxSignalInfo.snr);
              break;
            }
        }
      return;
    }
  uint32_t ampduRef = aggregate ? m_ampduRef : 0;
  for (std::size_t i = 0; i < psdu->GetNMpdus (); i++)
    {
      if (!aggregate && !statusPerMpdu.at (i))
        {
          continue;
        }
      Ptr<const WifiMpdu> mpdu = *std::next (psdu->begin (), i);
      const WifiMacHeader &hdr = mpdu->GetHeader ();
      bool forUs = hdr.GetAddr1 () == m_address;
      // Control frames go to the kernel too, after the device handled them, like
      // the frames of a card that passes them (FIF_CONTROL): its monitor
      // interfaces show them.
      if (hdr.IsAck ())
        {
          if (forUs && m_sending && m_ackTimeout.IsPending () && !m_expectBlockAck)
            {
              FinishHead (true);
            }
        }
      else if (hdr.IsBlockAck ())
        {
          if (forUs && m_sending && m_ackTimeout.IsPending () && m_expectBlockAck)
            {
              ReceiveBlockAck (mpdu);
            }
        }
      else if (hdr.IsRts ())
        {
          if (forUs)
            {
              Simulator::Schedule (m_phy->GetSifs (), &LklWifiDevice::SendCts, this,
                                   hdr.GetAddr2 (), hdr.GetDuration (), txVector,
                                   rxSignalInfo.snr);
            }
        }
      else if (hdr.IsBlockAckReq () && forUs)
        {
          CtrlBAckRequestHeader bar;
          mpdu->GetPacket ()->Copy ()->RemoveHeader (bar);
          auto key = std::make_pair (hdr.GetAddr2 (), bar.GetTidInfo ());
          Scoreboard &sb = m_scoreboards[key];
          uint16_t distance = (bar.GetStartingSequence () - sb.winStart) & 0xfff;
          if (sb.received.empty () || (distance > 0 && distance < 2048))
            {
              sb.winStart = bar.GetStartingSequence ();
              for (auto j = sb.received.begin (); j != sb.received.end ();)
                {
                  j = ((*j - sb.winStart) & 0xfff) >= 64 ? sb.received.erase (j) : std::next (j);
                }
            }
          Simulator::Schedule (m_phy->GetSifs (), &LklWifiDevice::SendBlockAck, this,
                               hdr.GetAddr2 (), bar.GetTidInfo (), txVector,
                               rxSignalInfo.snr);
          // and to the kernel, whose reorder buffer moves on
        }
      else if (!hdr.IsCtl () && forUs && !(hdr.IsQosData () && hdr.IsQosNoAck ()))
        {
          if (hdr.IsQosData () && aggregate)
            {
              // answered at the end of the A-MPDU
              Record (hdr.GetAddr2 (), hdr.GetQosTid (), hdr.GetSequenceNumber ());
            }
          else
            {
              if (hdr.IsQosData ())
                {
                  Record (hdr.GetAddr2 (), hdr.GetQosTid (), hdr.GetSequenceNumber ());
                }
              Simulator::Schedule (m_phy->GetSifs (), &LklWifiDevice::SendAck, this,
                                   hdr.GetAddr2 (), txVector, rxSignalInfo.snr);
            }
        }
      Ptr<Packet> frame = mpdu->GetProtocolDataUnit ();
      WifiMacTrailer fcs;
      frame->RemoveTrailer (fcs);
      Event event = {};
      event.type = Event::RX;
      event.frame.resize (frame->GetSize ());
      frame->CopyData (event.frame.data (), event.frame.size ());
      if (m_wmmFromEdca)
        {
          AddWmmFromEdca (event.frame, hdr);
        }
      event.freq = PrimaryFrequency ();
      event.signal = static_cast<int32_t> (rxSignalInfo.rssi);
      event.ampduRef = ampduRef;
      WifiModulationClass mc = txVector.GetModulationClass ();
      if (mc >= WIFI_MOD_CLASS_HT)
        {
          event.mode = mc == WIFI_MOD_CLASS_HT ? HT : mc == WIFI_MOD_CLASS_VHT ? VHT : HE;
          event.mcs = txVector.GetMode ().GetMcsValue () % 8;
          if (mc != WIFI_MOD_CLASS_HT)
            {
              event.mcs = txVector.GetMode ().GetMcsValue ();
            }
          event.nss = txVector.GetNss ();
          event.sgi = txVector.GetGuardInterval () == NanoSeconds (400);
          event.width = static_cast<uint16_t> (txVector.GetChannelWidth ());
        }
      else
        {
          event.mode = LEGACY;
          MHz_u width = (mc == WIFI_MOD_CLASS_DSSS || mc == WIFI_MOD_CLASS_HR_DSSS) ? MHz_u{22}
                                                                                    : MHz_u{20};
          event.rate = txVector.GetMode ().GetDataRate (width) / 100000;
          event.width = 20;
        }
      Notify (event);
    }
}

void
LklWifiDevice::AddWmmFromEdca (std::vector<uint8_t> &frame, const WifiMacHeader &hdr) const
{
  std::size_t fixed;
  if (hdr.IsBeacon () || hdr.IsProbeResp ())
    {
      fixed = 12; // timestamp, beacon interval, capabilities
    }
  else if (hdr.IsAssocResp () || hdr.IsReassocResp ())
    {
      fixed = 6; // capabilities, status, AID
    }
  else
    {
      return;
    }
  std::size_t pos = hdr.GetSize () + fixed;
  const uint8_t *edca = nullptr;
  while (pos + 2 <= frame.size () && pos + 2 + frame[pos + 1] <= frame.size ())
    {
      if (frame[pos] == 221 && frame[pos + 1] >= 6 && frame[pos + 2] == 0x00
          && frame[pos + 3] == 0x50 && frame[pos + 4] == 0xf2 && frame[pos + 5] == 2)
        {
          return; // already has WMM
        }
      if (frame[pos] == 12 && frame[pos + 1] == 18)
        {
          edca = &frame[pos + 2];
        }
      pos += 2 + frame[pos + 1];
    }
  if (!edca)
    {
      return;
    }
  // The WMM Parameter element: the body of the EDCA Parameter Set element
  // (QoS Info, reserved, then the AC_BE, AC_BK, AC_VI and AC_VO records).
  std::vector<uint8_t> wmm = {221, 24, 0x00, 0x50, 0xf2, 2, 1, 1};
  wmm.insert (wmm.end (), edca, edca + 18);
  frame.insert (frame.end (), wmm.begin (), wmm.end ());
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
  Ptr<Packet> packet = Create<Packet> ();
  AddSnrTag (packet, snr);
  m_phy->Send (Create<WifiPsdu> (packet, ack), GetResponseTxVector (rxVector, MHz_u{20}));
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
