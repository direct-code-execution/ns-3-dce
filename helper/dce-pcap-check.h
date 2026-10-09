/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DCE_PCAP_CHECK_H
#define DCE_PCAP_CHECK_H

#include <string>
#include <vector>
#include <map>
#include <stdint.h>

namespace ns3 {

/**
 * \brief Read back a pcap file written by ns-3 and count the TCP/UDP traffic
 * in it, so that an example can check that the traffic it expected did happen.
 *
 * 802.11 (data frames, with or without QoS control), Ethernet and raw IPv4
 * link types are understood; IPv4 only.
 */
class DcePcapCheck
{
public:
  struct Packet
  {
    double time;        // seconds
    uint8_t protocol;   // IPPROTO_TCP or IPPROTO_UDP
    uint32_t srcIp;
    uint32_t dstIp;
    uint16_t srcPort;
    uint16_t dstPort;
    std::string payload; // transport payload
  };

  /** Read the whole file; Ok() tells whether it could be parsed. */
  DcePcapCheck (const std::string &file);
  bool Ok (void) const;
  const std::vector<Packet> &GetPackets (void) const;

  /** Number of TCP segments or UDP datagrams with port as source or destination. */
  uint32_t Count (uint8_t protocol, uint16_t port) const;
  /** Same, keeping only the packets whose payload starts with prefix. */
  uint32_t Count (uint8_t protocol, uint16_t port, const std::string &prefix) const;
  /** Total transport payload bytes with port as source or destination. */
  uint64_t PayloadBytes (uint8_t protocol, uint16_t port) const;
  /** The first lines of the TCP payloads starting with prefix (e.g. HTTP request lines), with their counts. */
  std::map<std::string, uint32_t> RequestLines (uint16_t port, const std::string &prefix) const;

private:
  void ParseFrame (uint32_t linkType, const uint8_t *data, uint32_t len, double time);
  bool m_ok;
  std::vector<Packet> m_packets;
};

} // namespace ns3

#endif /* DCE_PCAP_CHECK_H */
