/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "dce-pcap-check.h"
#include "ns3/pcap-file.h"
#include "ns3/log.h"
#include <netinet/in.h>
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("DcePcapCheck");

namespace ns3 {

static const uint32_t DLT_EN10MB_ = 1;
static const uint32_t DLT_RAW_ = 101;
static const uint32_t DLT_IEEE802_11_ = 105;
static const uint32_t DLT_IEEE802_11_RADIO_ = 127;

DcePcapCheck::DcePcapCheck (const std::string &file)
  : m_ok (false)
{
  PcapFile pcap;
  pcap.Open (file, std::ios::in);
  if (pcap.Fail ())
    {
      NS_LOG_WARN ("cannot open " << file);
      return;
    }
  uint32_t linkType = pcap.GetDataLinkType ();
  uint8_t buffer[65536];
  while (true)
    {
      uint32_t tsSec, tsUsec, inclLen, origLen, readLen;
      pcap.Read (buffer, sizeof (buffer), tsSec, tsUsec, inclLen, origLen, readLen);
      if (pcap.Eof () || pcap.Fail ())
        {
          break;
        }
      ParseFrame (linkType, buffer, readLen, tsSec + tsUsec / 1e6);
    }
  m_ok = true;
}

static uint16_t
Get16 (const uint8_t *p)
{
  return (p[0] << 8) | p[1];
}

// ns-3 writes an aggregated 802.11n/ac/ax PSDU (A-MPDU) as the sequence of
// its subframes: a 4 byte delimiter (2 bytes with the MPDU length in the low
// 14 bits, a CRC byte, the signature 'N'), the MPDU, then padding to a
// multiple of 4 bytes except after the last one.
static bool
AmpduSubframe (const uint8_t *p, uint32_t left, uint32_t &mpduLen)
{
  if (left < 4 || p[3] != 0x4e)
    {
      return false;
    }
  uint32_t be = ((p[0] << 8) | p[1]) & 0x3fff;
  uint32_t le = ((p[1] << 8) | p[0]) & 0x3fff;
  if (be >= 24 && be <= left - 4)
    {
      mpduLen = be;
      return true;
    }
  if (le >= 24 && le <= left - 4)
    {
      mpduLen = le;
      return true;
    }
  return false;
}

void
DcePcapCheck::ParseFrame (uint32_t linkType, const uint8_t *data, uint32_t len, double time)
{
  const uint8_t *p = data;
  uint32_t left = len;
  if (linkType == DLT_IEEE802_11_RADIO_)
    {
      if (left < 4)
        {
          return;
        }
      uint16_t radiotapLen = p[2] | (p[3] << 8);
      if (radiotapLen > left)
        {
          return;
        }
      p += radiotapLen;
      left -= radiotapLen;
      linkType = DLT_IEEE802_11_;
    }
  if (linkType == DLT_IEEE802_11_)
    {
      uint32_t mpduLen;
      if (AmpduSubframe (p, left, mpduLen))
        {
          while (AmpduSubframe (p, left, mpduLen))
            {
              ParseFrame (DLT_IEEE802_11_, p + 4, mpduLen, time);
              uint32_t step = 4 + mpduLen;
              step += (4 - step % 4) % 4;
              if (step > left)
                {
                  break;
                }
              p += step;
              left -= step;
            }
          return;
        }
      if (left < 24)
        {
          return;
        }
      uint8_t fc0 = p[0];
      uint8_t fc1 = p[1];
      uint8_t type = (fc0 >> 2) & 3;
      uint8_t subtype = (fc0 >> 4) & 0xf;
      if (type != 2 || (subtype & 4)) // data frames carrying data only
        {
          return;
        }
      uint32_t hdr = 24;
      if ((fc1 & 3) == 3) // to-DS and from-DS: 4 addresses
        {
          hdr += 6;
        }
      if (subtype & 8) // QoS data
        {
          hdr += 2;
        }
      if (fc1 & 0x80) // order bit: HT control
        {
          hdr += 4;
        }
      if (left < hdr + 8)
        {
          return;
        }
      p += hdr;
      left -= hdr;
      // LLC/SNAP
      if (p[0] != 0xaa || p[1] != 0xaa || p[2] != 0x03)
        {
          return;
        }
      uint16_t etherType = Get16 (p + 6);
      p += 8;
      left -= 8;
      if (etherType != 0x0800)
        {
          return;
        }
    }
  else if (linkType == DLT_EN10MB_)
    {
      if (left < 14 || Get16 (p + 12) != 0x0800)
        {
          return;
        }
      p += 14;
      left -= 14;
    }
  else if (linkType != DLT_RAW_)
    {
      return;
    }
  // IPv4
  if (left < 20 || (p[0] >> 4) != 4)
    {
      return;
    }
  uint32_t ihl = (p[0] & 0xf) * 4;
  uint16_t totalLen = Get16 (p + 2);
  if (ihl < 20 || left < ihl || totalLen < ihl)
    {
      return;
    }
  if (totalLen < left)
    {
      left = totalLen;
    }
  Packet pkt;
  pkt.time = time;
  pkt.protocol = p[9];
  memcpy (&pkt.srcIp, p + 12, 4);
  memcpy (&pkt.dstIp, p + 16, 4);
  const uint8_t *t = p + ihl;
  uint32_t tleft = left - ihl;
  if (pkt.protocol == IPPROTO_TCP)
    {
      if (tleft < 20)
        {
          return;
        }
      uint32_t doff = (t[12] >> 4) * 4;
      if (doff < 20 || tleft < doff)
        {
          return;
        }
      pkt.srcPort = Get16 (t);
      pkt.dstPort = Get16 (t + 2);
      pkt.payload.assign ((const char *)t + doff, tleft - doff);
    }
  else if (pkt.protocol == IPPROTO_UDP)
    {
      if (tleft < 8)
        {
          return;
        }
      pkt.srcPort = Get16 (t);
      pkt.dstPort = Get16 (t + 2);
      pkt.payload.assign ((const char *)t + 8, tleft - 8);
    }
  else
    {
      return;
    }
  m_packets.push_back (pkt);
}

bool
DcePcapCheck::Ok (void) const
{
  return m_ok;
}

const std::vector<DcePcapCheck::Packet> &
DcePcapCheck::GetPackets (void) const
{
  return m_packets;
}

uint32_t
DcePcapCheck::Count (uint8_t protocol, uint16_t port) const
{
  return Count (protocol, port, "");
}

uint32_t
DcePcapCheck::Count (uint8_t protocol, uint16_t port, const std::string &prefix) const
{
  uint32_t n = 0;
  for (std::vector<Packet>::const_iterator i = m_packets.begin (); i != m_packets.end (); ++i)
    {
      if (i->protocol == protocol && (i->srcPort == port || i->dstPort == port)
          && i->payload.compare (0, prefix.size (), prefix) == 0)
        {
          n++;
        }
    }
  return n;
}

uint64_t
DcePcapCheck::PayloadBytes (uint8_t protocol, uint16_t port) const
{
  uint64_t n = 0;
  for (std::vector<Packet>::const_iterator i = m_packets.begin (); i != m_packets.end (); ++i)
    {
      if (i->protocol == protocol && (i->srcPort == port || i->dstPort == port))
        {
          n += i->payload.size ();
        }
    }
  return n;
}

std::map<std::string, uint32_t>
DcePcapCheck::RequestLines (uint16_t port, const std::string &prefix) const
{
  std::map<std::string, uint32_t> lines;
  for (std::vector<Packet>::const_iterator i = m_packets.begin (); i != m_packets.end (); ++i)
    {
      if (i->protocol == IPPROTO_TCP && i->dstPort == port
          && i->payload.compare (0, prefix.size (), prefix) == 0)
        {
          size_t eol = i->payload.find ("\r\n");
          lines[i->payload.substr (0, eol == std::string::npos ? i->payload.size () : eol)]++;
        }
    }
  return lines;
}

} // namespace ns3
