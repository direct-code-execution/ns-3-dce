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

#include "netlink-attribute.h"
#include "netlink-message.h"
#include "netlink-message-route.h"
#include "ns3/address-utils.h"

namespace ns3 {


/***********************************************************************************
* \ NetlinkAttributeValue
***********************************************************************************/
NetlinkAttributeValue::NetlinkAttributeValue ()
{
  m_type = UNSPEC;
  m_u32 = 0;
}

NetlinkAttributeValue:: NetlinkAttributeValue (NetlinkAttributeValueType type, uint8_t v)
{
  NS_ASSERT (type == U8);
  m_type = U8;
  m_u8 = v;
}
NetlinkAttributeValue:: NetlinkAttributeValue (NetlinkAttributeValueType type, uint16_t v)
{
  NS_ASSERT (type == U16);
  m_type = U16;
  m_u16 = v;
}
NetlinkAttributeValue:: NetlinkAttributeValue (NetlinkAttributeValueType type, uint32_t v)
{
  NS_ASSERT (type == U32);
  m_type = U32;
  m_u32 = v;
}
NetlinkAttributeValue:: NetlinkAttributeValue (NetlinkAttributeValueType type, uint64_t v)
{
  NS_ASSERT (type == U64);
  m_type = U64;
  m_u64 = v;
}
NetlinkAttributeValue:: NetlinkAttributeValue (NetlinkAttributeValueType type, std::string v)
{
  NS_ASSERT (type == STRING);
  m_type = STRING;
  m_string = v;
}
NetlinkAttributeValue:: NetlinkAttributeValue (NetlinkAttributeValueType type, Address v)
{
  NS_ASSERT (type == ADDRESS);
  m_type = ADDRESS;
  m_address = v;
}

void
NetlinkAttributeValue::SetType (NetlinkAttributeValueType type)
{
  m_type = type;
}
NetlinkAttributeValueType
NetlinkAttributeValue::GetType (void) const
{
  return m_type;
}
void
NetlinkAttributeValue::SetAddress (Address value)
{
  m_address = value;
}
void
NetlinkAttributeValue::SetString (std::string value)
{
  m_string = value;
}
void
NetlinkAttributeValue::SetU64 (uint64_t value)
{
  m_u64 = value;
}
void
NetlinkAttributeValue::SetU32 (uint32_t value)
{
  m_u32 = value;
}
void
NetlinkAttributeValue::SetU16 (uint16_t value)
{
  m_u16 = value;
}
void
NetlinkAttributeValue::SetU8 (uint8_t value)
{
  m_u8 = value;
}
Address
NetlinkAttributeValue::GetAddress (void) const
{
  return m_address;
}
std::string
NetlinkAttributeValue::GetString (void) const
{
  return m_string;
}
uint64_t
NetlinkAttributeValue::GetU64 (void) const
{
  return m_u64;
}
uint32_t
NetlinkAttributeValue::GetU32 (void) const
{
  return m_u32;
}
uint16_t
NetlinkAttributeValue::GetU16 (void) const
{
  return m_u16;
}
uint8_t
NetlinkAttributeValue::GetU8 (void) const
{
  return m_u8;
}

void
NetlinkAttributeValue::Serialize (Buffer::Iterator& start) const
{
  uint32_t len;

  if (m_type == U8)
    {
      start.WriteU8 (m_u8);
      len = 1;
    }
  else if (m_type == U16)
    {
      start.WriteU16 (m_u16);
      len = 2;
    }
  else if (m_type == U32)
    {
      start.WriteU32 (m_u32);
      len = 4;
    }
  else if (m_type == STRING)
    {
      start.Write ((const uint8_t *)m_string.c_str (), (uint32_t)m_string.size () + 1);
      len = (uint32_t)m_string.size () + 1;
    }
  else if (m_type == ADDRESS)
    {
      WriteTo (start, m_address);
      len = m_address.GetLength ();
    }
  else
    {
      len = 0;
    }
  //netlink align
  start.WriteU8 (0, NETLINK_MSG_ALIGN (len) - len);
}

uint32_t
NetlinkAttributeValue::DeserializeWithType (Buffer::Iterator& start,
                                            NetlinkAttributeValueType_e type, uint16_t remaining)
{
  uint32_t len = 0;
  m_type = type;

  if (m_type == U8)
    {
      m_u8 = start.ReadU8 ();
      len = 1;
    }
  else if (m_type == U16)
    {
      m_u16 = start.ReadU16 ();
      len = 2;
    }
  else if (m_type == U32)
    {
      m_u32 = start.ReadU32 ();
      len = 4;
    }
  else if (m_type == U64)
    {
      m_u64 = start.ReadU64 ();
    }
  else if (m_type == STRING)
    {
      char buf[512];
      uint32_t i = 0;
      do
        {
          buf[i] = start.ReadU8 ();
        }
      while (buf[i++]);

      m_string = std::string (buf);
      len = (uint32_t)m_string.size () + 1;
    }
  else if (m_type == ADDRESS)
    {
      ReadFrom (start, m_address, remaining);
      len = m_address.GetLength ();
    }
  else
    {
      len = 0;
    }

  //netlink align
  uint8_t buf[4];
  start.Read (buf, NETLINK_MSG_ALIGN (len) - len);

  return NETLINK_MSG_ALIGN (len);
}

uint32_t
NetlinkAttributeValue::GetSerializedSize () const
{
  return NETLINK_MSG_ALIGN (GetSize ());
}

uint32_t
NetlinkAttributeValue::GetSize () const
{
  uint32_t len;

  if (m_type == U8)
    {
      len = 1;
    }
  else if (m_type == U16)
    {
      len =  2;
    }
  else if (m_type == U32)
    {
      len =  4;
    }
  else if (m_type == STRING)
    {
      len =  uint32_t (m_string.size () + 1);
    }
  else if (m_type == ADDRESS)
    {
      len =  m_address.GetLength ();
    }
  else
    {
      len = 0;
    }

  return len;
}

std::string
NetlinkAttribute::TypeToStr(NetlinkAttributeValueType type)
{
  static const char* str[] = {
    "UNSPEC", // invalid initial value.
  "U8",
  "U16",
  "U32",
  "U64",
  "STRING",
  "ADDRESS"
  };

  NS_ASSERT(type >= UNSPEC && type <= ADDRESS);
  return str[type];
}

void
NetlinkAttributeValue::Print (std::ostream &os) const
{
  os << "NetlinkAttributeValue (type= " << NetlinkAttribute::TypeToStr(m_type) << ", v= ";
  if (m_type == U8)
    {
      os << m_u8;
    }
  else if (m_type == U16)
    {
      os << m_u16;
    }
  else if (m_type == U32)
    {
      os << m_u32;
    }
  else if (m_type == U64)
    {
      os << m_u64;
    }
  else if (m_type == STRING)
    {
      os << m_string;
    }
  else if (m_type == ADDRESS)
    {
      os << "address(" << m_address << ")";
    }
  else
    {
      os << "NULL";
    }
  os << ")";
}


/***********************************************************************************
* \ NetlinkAttribute
***********************************************************************************/

NetlinkAttribute::NetlinkAttribute ()
  : m_len (4),
    m_type (0)
{
}

NetlinkAttribute::NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint8_t payload)
{
  m_payload = NetlinkAttributeValue (payloadtype, payload);
  m_len = NETLINK_MSG_ATTR_SIZE + m_payload.GetSize ();
  m_type = type;
}

NetlinkAttribute::NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint16_t payload)
{
  m_payload = NetlinkAttributeValue (payloadtype, payload);
  m_len = NETLINK_MSG_ATTR_SIZE + m_payload.GetSize ();
  m_type = type;
}
NetlinkAttribute::NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint32_t payload)
{
  m_payload = NetlinkAttributeValue (payloadtype, payload);
  m_len = NETLINK_MSG_ATTR_SIZE + m_payload.GetSize ();
  m_type = type;
}
NetlinkAttribute::NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  uint64_t payload)
{
  m_payload = NetlinkAttributeValue (payloadtype, payload);
  m_len = NETLINK_MSG_ATTR_SIZE + m_payload.GetSize ();
  m_type = type;
}
NetlinkAttribute::NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  std::string payload)
{
  m_payload = NetlinkAttributeValue (payloadtype, payload);
  m_len = NETLINK_MSG_ATTR_SIZE + m_payload.GetSize ();
  m_type = type;
}
NetlinkAttribute::NetlinkAttribute (uint16_t type, NetlinkAttributeValueType payloadtype,  Address payload)
{
  m_payload = NetlinkAttributeValue (payloadtype, payload);
  m_len = NETLINK_MSG_ATTR_SIZE + m_payload.GetSize ();
  m_type = type;
}

void
NetlinkAttribute::SetAttrLen (uint16_t v)
{
  m_len = v;
}
void
NetlinkAttribute::SetAttrType (uint16_t v)
{
  m_type = v;
}
void
NetlinkAttribute::SetAttrPayload (NetlinkAttributeValue v)
{
  m_payload = v;
}
uint16_t
NetlinkAttribute::GetAttrLen () const
{
  return m_len;
}
uint16_t
NetlinkAttribute::GetAttrType () const
{
  return m_type;
}
NetlinkAttributeValue
NetlinkAttribute::GetAttrPayload () const
{
  return m_payload;
}

void
NetlinkAttribute::Print (std::ostream &os) const
{
  os << "NetlinkAttribute "
     << "len: " << m_len << " "
     << "type: " << m_type << " "
     << "payload:[";
  m_payload.Print (os);
  os << "]" << std::endl;
}

uint32_t
NetlinkAttribute::GetSerializedSize (void) const
{
  /* this is the size of an nlattr payload. */
  return NETLINK_MSG_ATTR_SIZE + m_payload.GetSerializedSize ();
}

void
NetlinkAttribute::Serialize (Buffer::Iterator& start) const
{
  start.WriteU16 (m_len);
  start.WriteU16 (m_type);
  m_payload.Serialize (start);
}

uint32_t
NetlinkAttribute::Deserialize (Buffer::Iterator& start, NetlinkAttributeValueType vtypes[])
{
  NetlinkAttributeValueType type;

  m_len = start.ReadU16 ();
  m_type = start.ReadU16 ();
  type = vtypes[m_type];
  m_payload.DeserializeWithType (start, type, m_len - 4);

  return GetSerializedSize ();
}

}; // namespace ns3
