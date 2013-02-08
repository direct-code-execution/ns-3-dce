/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 Liu Jian
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
 * Author: Liu Jian <liujatp@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef NETLINK_ATTRIBUTE_H
#define NETLINK_ATTRIBUTE_H


#include <stdint.h>
#include <string>
#include <ostream>
#include "ns3/address.h"
#include "ns3/buffer.h"

namespace ns3 {

/**
* \brief The Netlink Attribute
*/

typedef enum NetlinkAttributeValueType_e
{
  UNSPEC, // invalid initial value.
  U8,
  U16,
  U32,
  U64,
  STRING,
  ADDRESS,
}NetlinkAttributeValueType;

class NetlinkAttributeValue
{
public:
  NetlinkAttributeValue ();
  NetlinkAttributeValue (NetlinkAttributeValueType type, uint8_t v);
  NetlinkAttributeValue (NetlinkAttributeValueType type, uint16_t v);
  NetlinkAttributeValue (NetlinkAttributeValueType type, uint32_t v);
  NetlinkAttributeValue (NetlinkAttributeValueType type, uint64_t v);
  NetlinkAttributeValue (NetlinkAttributeValueType type, std::string v);
  NetlinkAttributeValue (NetlinkAttributeValueType type, Address v);

  void Serialize (Buffer::Iterator& start) const;
  uint32_t DeserializeWithType (Buffer::Iterator& start, NetlinkAttributeValueType type, uint16_t remaining);
  uint32_t GetSerializedSize (void) const;
  uint32_t GetSize (void) const;
  void Print (std::ostream &os) const;

  void SetType (NetlinkAttributeValueType type);
  NetlinkAttributeValueType GetType (void) const;
  void SetAddress (Address value);
  void SetString (std::string value);
  void SetU64 (uint64_t value);
  void SetU32 (uint32_t value);
  void SetU16 (uint16_t value);
  void SetU8 (uint8_t value);
  Address GetAddress (void) const;
  std::string GetString (void) const;
  uint64_t GetU64 (void) const;
  uint32_t GetU32 (void) const;
  uint16_t GetU16 (void) const;
  uint8_t GetU8 (void) const;

private:
  NetlinkAttributeValueType m_type;
  uint64_t m_u64;
  uint32_t m_u32;
  uint16_t m_u16;
  uint8_t m_u8;
  std::string m_string;
  Address m_address;
};

struct NetlinkAttribute
{
public:
  NetlinkAttribute ();
  NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint8_t payload);
  NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint16_t payload);
  NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint32_t payload);
  NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint64_t payload);
  NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  std::string payload);
  NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  Address payload);

  //static TypeId GetTypeId (void);
  //virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator& start) const;
  uint32_t Deserialize (Buffer::Iterator& start, NetlinkAttributeValueType vtypes[]);

  void SetAttrLen (uint16_t v);
  void SetAttrType (uint16_t v);
  void SetAttrPayload (NetlinkAttributeValue v);
  uint16_t GetAttrLen () const;
  uint16_t GetAttrType () const;
  NetlinkAttributeValue GetAttrPayload () const;

private:
  static const int NETLINK_MSG_ATTR_SIZE = 4; /* size of the nlattr field*/
  uint16_t m_len;
  uint16_t m_type;
  NetlinkAttributeValue m_payload;
};

}; // namespace ns3

#endif /* NETLINK_ATTRIBUTE_H */
