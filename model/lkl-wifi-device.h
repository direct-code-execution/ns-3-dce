/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */
#ifndef LKL_WIFI_DEVICE_H
#define LKL_WIFI_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-phy.h"
#include <deque>
#include <map>
#include <set>
#include <vector>

namespace ns3 {

class WifiNetDevice;
class WifiPsdu;
class WifiMpdu;
class WifiMacHeader;
struct RxSignalInfo;

/**
 * The radio hardware of a Linux kernel (LKL) mac80211 radio, on an ns-3
 * WifiPhy: Linux's own frames go on the air, so that captures of the
 * channel show the frames of Linux's 802.11 stack, next to those of ns-3
 * devices on the same channel.
 *
 * Like a SoftMAC card, it contends for the medium (DCF), acknowledges the
 * frames it receives, waits for the acknowledgments of those it sends, and
 * retries them over the rate series chosen by Linux's rate control, then
 * reports their outcome. Everything else, from beacons to mesh peering,
 * is Linux's.
 *
 * KernelSocketFdFactory turns each of these devices of a node into a
 * kernel radio (wlan<n>), configured with iw. The PHY is attached with
 * LklWifiHelper.
 *
 * Legacy, HT, VHT and HE (single user) rates. Like the hardware, the device
 * aggregates the frames the kernel lets it aggregate (A-MPDU) and handles
 * the block acknowledgments of the Block Ack sessions mac80211 sets up: it
 * answers A-MPDUs and Block Ack requests with a compressed BlockAck from
 * its receive scoreboard, and reports the outcome of each MPDU of the
 * A-MPDUs it sends from the BlockAck it gets back. It answers RTS with CTS,
 * on the width of the RTS. Rate control is the kernel's (minstrel_ht),
 * except for HE peers, whose frames the kernel has sent as HE SU at the
 * MCS minstrel_ht chose, as firmware does.
 */
class LklWifiDevice : public NetDevice
{
public:
  /// What the device tells the kernel.
  struct Event
  {
    enum Type
    {
      RX = 1,        ///< a frame was received
      TX_STATUS = 2, ///< the outcome of the oldest frame sent with status
    } type;
    std::vector<uint8_t> frame; ///< RX: the 802.11 frame, without FCS
    uint16_t freq;              ///< RX: MHz
    uint16_t rate;              ///< RX: 100 kbps units (legacy)
    int32_t signal;             ///< RX: dBm
    bool acked;                 ///< TX_STATUS
    uint8_t attempts;           ///< TX_STATUS: in total
    uint8_t mode;               ///< RX: one of Mode
    uint8_t mcs;                ///< RX: HT/VHT/HE MCS (HT: 0-7, per stream)
    uint8_t nss;                ///< RX: spatial streams
    bool sgi;                   ///< RX: short guard interval
    uint16_t width;             ///< RX: MHz
    uint32_t ampduRef;          ///< RX: the A-MPDU of the frame, 0 for none
  };

  /// Modulation of a rate.
  enum Mode
  {
    LEGACY = 0,
    HT = 1,
    VHT = 2,
    HE = 3,
  };

  /// How the kernel wants a frame sent.
  struct TxInfo
  {
    uint16_t rate[4];  ///< legacy rate of each entry of the series, 100 kbps units
    uint8_t count[4];  ///< attempts at each rate; 0 ends the series
    uint8_t mode[4];   ///< Mode of each entry
    uint8_t mcs[4];    ///< MCS of each HT/VHT/HE entry (HT: 0-7, per stream)
    uint8_t nss[4];    ///< spatial streams of each HT/VHT/HE entry
    uint16_t width[4]; ///< MHz
    bool sgi[4];       ///< short guard interval
    uint16_t freq;     ///< MHz
    bool noAck;        ///< sent once, not acknowledged
    bool noStatus;     ///< no TX_STATUS event
    bool ampdu;        ///< may be aggregated: a Block Ack session is set up
  };

  /**
   * Get the type ID.
   * @return the object TypeId
   */
  static TypeId GetTypeId (void);
  LklWifiDevice ();
  ~LklWifiDevice () override;

  /**
   * Attach the PHY, which must be configured and on its channel.
   *
   * @param phy the PHY
   * @param carrier the WifiNetDevice the PHY is attached to, with this
   *        device's node: it carries the PHY's node and its traces (pcap),
   *        but has no MAC and is not one of the node's devices
   */
  void SetPhy (Ptr<WifiPhy> phy, Ptr<WifiNetDevice> carrier);
  /// @return the PHY
  Ptr<WifiPhy> GetPhy (void) const;
  /// @return the WifiNetDevice the PHY is attached to, for its traces
  Ptr<WifiNetDevice> GetCarrier (void) const;

  /**
   * Queue a frame for transmission.
   *
   * @param frame the 802.11 frame, without FCS
   * @param info how to send it
   */
  void Transmit (const std::vector<uint8_t> &frame, const TxInfo &info);
  /**
   * Take the next event for the kernel.
   *
   * @param event the event
   * @return false without event
   */
  bool PopEvent (Event &event);
  /// @return whether events wait for the kernel
  bool HasEvents (void) const;
  /**
   * Set the callback called when an event is queued for the kernel.
   *
   * @param callback the callback
   */
  void SetEventCallback (Callback<void> callback);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this device.
   *
   * @param stream first stream index to use
   * @return the number of stream indices assigned
   */
  int64_t AssignStreams (int64_t stream);

  // NetDevice: the kernel's frames do not go through these.
  void SetIfIndex (const uint32_t index) override;
  uint32_t GetIfIndex (void) const override;
  Ptr<Channel> GetChannel (void) const override;
  void SetAddress (Address address) override;
  Address GetAddress (void) const override;
  bool SetMtu (const uint16_t mtu) override;
  uint16_t GetMtu (void) const override;
  bool IsLinkUp (void) const override;
  void AddLinkChangeCallback (Callback<void> callback) override;
  bool IsBroadcast (void) const override;
  Address GetBroadcast (void) const override;
  bool IsMulticast (void) const override;
  Address GetMulticast (Ipv4Address multicastGroup) const override;
  Address GetMulticast (Ipv6Address addr) const override;
  bool IsBridge (void) const override;
  bool IsPointToPoint (void) const override;
  bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber) override;
  bool SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                 uint16_t protocolNumber) override;
  Ptr<Node> GetNode (void) const override;
  void SetNode (Ptr<Node> node) override;
  bool NeedsArp (void) const override;
  void SetReceiveCallback (NetDevice::ReceiveCallback cb) override;
  void SetPromiscReceiveCallback (PromiscReceiveCallback cb) override;
  bool SupportsSendFrom (void) const override;

protected:
  void DoInitialize (void) override;
  void DoDispose (void) override;

private:
  /// A frame waiting for its turn, or being sent.
  struct Pending
  {
    std::vector<uint8_t> frame; ///< the 802.11 frame
    TxInfo info;                ///< how to send it
    uint8_t series;             ///< current entry of the rate series
    uint8_t tries;              ///< attempts at the current rate
    uint8_t attempts;           ///< attempts in total
    bool done;                  ///< its outcome is known
    bool acked;                 ///< done: it was acknowledged
  };

  /// What a recipient of a Block Ack session has received.
  struct Scoreboard
  {
    uint16_t winStart;          ///< first sequence number of the window
    std::set<uint16_t> received; ///< in the window
  };

  /**
   * Build the MPDU of a frame.
   *
   * @param frame the 802.11 frame, without FCS
   * @return the MPDU, or 0 if the header cannot be parsed
   */
  Ptr<WifiMpdu> MakeMpdu (const std::vector<uint8_t> &frame) const;
  /**
   * The TX vector of a legacy rate.
   *
   * @param rate the rate, in 100 kbps units
   * @param width the channel width (non-HT duplicate above 20 MHz)
   * @return the TX vector
   */
  WifiTxVector GetTxVector (uint16_t rate, MHz_u width = MHz_u{20}) const;
  /**
   * The TX vector of an entry of a rate series.
   *
   * @param info the rate series
   * @param series the entry
   * @return the TX vector
   */
  WifiTxVector GetTxVector (const TxInfo &info, uint8_t series) const;
  /**
   * The TX vector of a control response (ACK, BlockAck, CTS): the highest
   * mandatory legacy rate of the modulation of the frame it answers.
   *
   * @param rxVector the TX vector of the frame it answers
   * @param width the channel width
   * @return the TX vector
   */
  WifiTxVector GetResponseTxVector (const WifiTxVector &rxVector, MHz_u width) const;
  /**
   * Add the WMM Parameter element equivalent to the EDCA Parameter Set
   * element of a beacon, probe response or association response.
   *
   * @param frame the frame, without FCS
   * @param hdr its MAC header
   */
  void AddWmmFromEdca (std::vector<uint8_t> &frame, const WifiMacHeader &hdr) const;
  /// @return the center frequency of the primary 20 MHz channel (MHz)
  uint16_t PrimaryFrequency (void) const;
  /// @return the index in the queue of the first frame without outcome
  std::size_t FirstPending (void) const;
  /**
   * Account a failed attempt of a frame: move along its rate series, or
   * give up.
   *
   * @param pending the frame
   */
  void Failed (Pending &pending);
  /// Report the outcomes known at the head of the queue, then contend again.
  void Flush (void);
  /**
   * A BlockAck for the A-MPDU in flight was received.
   *
   * @param ba the BlockAck
   */
  void ReceiveBlockAck (Ptr<const WifiMpdu> ba);
  /**
   * Record a frame of a Block Ack session in the scoreboard.
   *
   * @param ta its transmitter
   * @param tid its TID
   * @param seq its sequence number
   */
  void Record (Mac48Address ta, uint8_t tid, uint16_t seq);
  /**
   * Answer with a compressed BlockAck after SIFS.
   *
   * @param to the originator
   * @param tid the TID
   * @param rxVector the TX vector of the frame answered
   * @param snr the SNR of the frame answered
   */
  void SendBlockAck (Mac48Address to, uint8_t tid, WifiTxVector rxVector, double snr);
  /**
   * Add the SNR tag of a response, as ns-3 receivers do.
   *
   * @param packet the response
   * @param snr the SNR of the frame answered
   */
  static void AddSnrTag (Ptr<Packet> packet, double snr);
  /**
   * Answer an RTS with a CTS after SIFS.
   *
   * @param to the transmitter of the RTS
   * @param duration the Duration of the RTS
   * @param rxVector its TX vector
   * @param snr its SNR
   */
  void SendCts (Mac48Address to, Time duration, WifiTxVector rxVector, double snr);
  /// Start the contention for the frame at the head of the queue.
  void StartAccess (void);
  /// One slot of contention: count it down if the medium is idle.
  void AccessSlot (void);
  /// Send the frame at the head of the queue, its contention won.
  void SendHead (void);
  /// No acknowledgment came for the frame at the head of the queue.
  void AckTimeout (void);
  /**
   * Done with the frame at the head of the queue.
   *
   * @param acked whether it was acknowledged
   */
  void FinishHead (bool acked);
  /**
   * Queue an event for the kernel.
   *
   * @param event the event
   */
  void Notify (const Event &event);
  /**
   * Called by the PHY for each PSDU received.
   *
   * @param psdu the PSDU
   * @param rxSignalInfo the signal of the PSDU
   * @param txVector its TX vector
   * @param statusPerMpdu reception status of each MPDU
   */
  void Receive (Ptr<const WifiPsdu> psdu, RxSignalInfo rxSignalInfo,
                const WifiTxVector &txVector, const std::vector<bool> &statusPerMpdu);
  /**
   * Acknowledge a frame after SIFS.
   *
   * @param to the frame's transmitter
   * @param txVector the TX vector of the frame
   * @param snr the SNR of the frame
   */
  void SendAck (Mac48Address to, WifiTxVector txVector, double snr);

  Ptr<Node> m_node;                   ///< the node
  Ptr<WifiPhy> m_phy;                 ///< the PHY
  Ptr<WifiNetDevice> m_carrier;       ///< the PHY's WifiNetDevice
  Mac48Address m_address;             ///< the radio's address
  uint32_t m_ifIndex;                 ///< interface index
  uint16_t m_mtu;                     ///< MTU
  uint32_t m_cwMin;                   ///< minimum contention window
  uint32_t m_cwMax;                   ///< maximum contention window
  uint32_t m_maxQueue;                ///< frames the queue can hold
  uint32_t m_cw;                      ///< current contention window
  uint32_t m_backoff;                 ///< slots of backoff left
  uint32_t m_idleSlots;               ///< idle slots since the medium was busy
  bool m_sending;                     ///< a frame is on the air or awaits its ACK
  bool m_expectBlockAck;              ///< the frame on the air is answered with a BlockAck
  std::vector<std::size_t> m_inflight; ///< queue indexes of the MPDUs on the air
  std::map<std::pair<Mac48Address, uint8_t>, Scoreboard> m_scoreboards; ///< per TA and TID
  uint32_t m_ampduRef;                ///< A-MPDUs received
  bool m_rxAmpduOpen;                 ///< MPDUs of an A-MPDU are being received
  bool m_wmmFromEdca;                 ///< add WMM elements from EDCA Parameter Sets
  EventId m_access;                   ///< next contention slot
  EventId m_ackTimeout;               ///< acknowledgment timeout
  std::deque<Pending> m_queue;        ///< frames to send, head first
  std::deque<Event> m_events;         ///< events for the kernel
  Callback<void> m_eventCallback;     ///< called when an event is queued
  Ptr<UniformRandomVariable> m_rng;   ///< backoff draws
  /// Frames whose header ns-3 does not serialize back to the same bytes
  TracedCallback<const std::vector<uint8_t> &, Ptr<const Packet>> m_headerMismatchTrace;
};

} // namespace ns3

#endif /* LKL_WIFI_DEVICE_H */
