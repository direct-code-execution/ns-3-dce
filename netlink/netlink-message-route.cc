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

#include "netlink-message-route.h"
#include "netlink-message.h"

namespace ns3 {

std::string NetlinkRtmTypeToStr(int type)
{
  static const char* str[] = {
  "GETLINK",
  "SETLINK",

  "NEWADDR",
  "DELADDR",
  "GETADDR",

  "NEWROUTE",
  "DELROUTE",
  "GETROUTE"
  };

  NS_ASSERT(type > NETLINK_RTM_BASE && type < NETLINK_RTM_MAX);
  return str[type-NETLINK_RTM_GETLINK];

}

std::string NetlinkFamilyToStr(int family)
{
  static const char* str[] = {
  "BASE",
  "GETLINK",
  "SETLINK",

  "NEWADDR",
  "DELADDR",
  "GETADDR",

  "NEWROUTE",
  "DELROUTE",
  "GETROUTE"
  };

  NS_ASSERT(family >= 0 && family < 30);
  return str[family];

}


/***********************************************************************************
* \ NetlinkPayload
***********************************************************************************/
TypeId
NetlinkPayload::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetlinkPayload")
    .SetParent<ObjectBase> ()
    .SetGroupName ("Netlink")
  ;
  return tid;
}


/***********************************************************************************
* \ GeneralMessage
***********************************************************************************/

NS_OBJECT_ENSURE_REGISTERED (GeneralMessage);
NS_OBJECT_ENSURE_REGISTERED (InterfaceInfoMessage);
NS_OBJECT_ENSURE_REGISTERED (InterfaceAddressMessage);
NS_OBJECT_ENSURE_REGISTERED (RouteMessage);

GeneralMessage::GeneralMessage ()
  : m_family (0)
{
}
GeneralMessage::~GeneralMessage ()
{
}

void
GeneralMessage::SetFamily (uint8_t v)
{
  m_family = v;
}
uint8_t
GeneralMessage::GetFamily (void) const
{
  return m_family;
}

TypeId
GeneralMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GeneralMessage")
    .SetParent<NetlinkPayload> ()
    .SetGroupName ("Netlink")
    .AddConstructor<GeneralMessage> ()
  ;
  return tid;
}

TypeId
GeneralMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
GeneralMessage::Print (std::ostream &os) const
{
  os << " ----GeneralMessage ("
     << "family: " << (uint32_t)m_family << ")";
}

uint32_t
GeneralMessage::GetSerializedSize (void) const
{
  /* this is the size of an nlmsghdr payload. */
  return NETLINK_MSG_ALIGN (NETLINK_GENMSG_SIZE);
}


void
GeneralMessage::Serialize (Buffer::Iterator& start) const
{
  start.WriteU8 (m_family);
  start.WriteU8 (0, 3);
}

uint32_t
GeneralMessage::Deserialize (Buffer::Iterator& start)
{
  uint8_t buf[3];
  m_family = start.ReadU8 ();
  start.Read (buf, 3);
  return GetSerializedSize ();
}
uint32_t
GeneralMessage::Deserialize (Buffer::Iterator& start, uint32_t len)
{
  uint8_t buf[3];
  m_family = start.ReadU8 ();
  start.Read (buf, 3);
  return GetSerializedSize ();
}


uint32_t
GeneralMessage::GetNNetlinkAttribute (void) const
{
  return m_attributes.size ();
}
NetlinkAttribute
GeneralMessage::GetNetlinkAttribute (uint32_t index) const
{
  NS_ASSERT (index < GetNNetlinkAttribute ());
  return m_attributes[index];
}

uint32_t
GeneralMessage::GetAttributeSerializedSize (void) const
{
  uint32_t size = 0;

  for (uint32_t i = 0; i < m_attributes.size (); i++)
    {
      size += m_attributes[i].GetSerializedSize ();
    }
  return size;
}
bool
GeneralMessage::GetAttributeByType (NetlinkAttribute& attr, uint16_t type)
{
  for (uint32_t i = 0; i < m_attributes.size (); i++)
    {
      if (type == m_attributes[i].GetAttrType ())
        {
          attr = m_attributes[i];
          return true;
        }
    }
  return false;
}
void
GeneralMessage::AppendAttribute (NetlinkAttribute v)
{
  m_attributes.push_back (v);
}

void
GeneralMessage::SerializeAttribute (Buffer::Iterator& start) const
{
  for (uint32_t i = 0; i < m_attributes.size (); i++)
    {
      m_attributes[i].Serialize (start);
    }
}

void
GeneralMessage::PrintAttribute (std::ostream &os) const
{
  for (uint32_t i = 0; i < m_attributes.size (); i++)
    {
      os << " ----Attribute (" << i << "):";
      m_attributes[i].Print (os);
    }
}

/***********************************************************************************
* \ InterfaceInfoMessage
***********************************************************************************/
InterfaceInfoMessage::InterfaceInfoMessage ()
  : m_reserved (0),
    m_deviceType (0),
    m_interfaceIndex (0),
    m_deviceFlags (0),
    m_changeMask (0)
{
  memset ((void*)m_attributeTypes, 0, sizeof (m_attributeTypes));
  m_attributeTypes[IFL_A_UNSPEC] = UNSPEC;
  m_attributeTypes[IFL_A_ADDRESS] = ADDRESS;
  m_attributeTypes[IFL_A_BROADCAST] = ADDRESS;
  m_attributeTypes[IFL_A_IFNAME] = STRING;
  m_attributeTypes[IFL_A_MTU] = U32;
  m_attributeTypes[IFL_A_LINK] = U32;
  m_attributeTypes[IFL_A_QDISC] = U8;
  m_attributeTypes[IFL_A_STATS] = UNSPEC;
  m_attributeTypes[IFL_A_COST] = UNSPEC;
}
InterfaceInfoMessage::~InterfaceInfoMessage ()
{
}
void
InterfaceInfoMessage::SetDeviceType (uint16_t type)
{
  m_deviceType = type;
}
void
InterfaceInfoMessage::SetInterfaceIndex (int32_t index)
{
  m_interfaceIndex = index;
}
void
InterfaceInfoMessage::SetDeviceFlags (uint32_t flags)
{
  m_deviceFlags = flags;
}
void
InterfaceInfoMessage::SetChangeMask (uint32_t mask)
{
  m_changeMask = mask;
}
uint16_t
InterfaceInfoMessage::GetDeviceType (void) const
{
  return m_deviceType;
}
int32_t
InterfaceInfoMessage::GetInterfaceIndex (void) const
{
  return m_interfaceIndex;
}
uint32_t
InterfaceInfoMessage::GetDeviceFlags (void) const
{
  return m_deviceFlags;
}
uint32_t
InterfaceInfoMessage::GetChangeMask (void) const
{
  return m_changeMask;
}
TypeId
InterfaceInfoMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::InterfaceInfoMessage")
    .SetParent<GeneralMessage> ()
    .AddConstructor<InterfaceInfoMessage> ()
  ;
  return tid;
}
TypeId
InterfaceInfoMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
InterfaceInfoMessage::Print (std::ostream &os) const
{
  os << " ----InterfaceInfoMessage ("
     << "deviceType: " << m_deviceType << " "
     << "interfaceIndex: " << m_interfaceIndex << " "
     << "deviceFlags: " << m_deviceFlags << " "
     << "changeMask: " << m_changeMask << ")" ;
  PrintAttribute (os);
}
uint32_t
InterfaceInfoMessage::GetSerializedSize (void) const
{
  return NETLINK_INTERFACE_SIZE + GetAttributeSerializedSize ();
}

void
InterfaceInfoMessage::Serialize (Buffer::Iterator& start) const
{
  start.WriteU8 (m_family);
  start.WriteU8 (m_reserved);
  start.WriteU16 (m_deviceType);
  start.WriteU32 (m_interfaceIndex);
  start.WriteU32 (m_deviceFlags);
  start.WriteU32 (m_changeMask);

  SerializeAttribute (start);
}
uint32_t
InterfaceInfoMessage::Deserialize (Buffer::Iterator& start, uint32_t len)
{
  m_family = start.ReadU8 ();
  m_reserved = start.ReadU8 ();
  m_deviceType = start.ReadU16 ();
  m_interfaceIndex = start.ReadU32 ();
  m_deviceFlags = start.ReadU32 ();
  m_changeMask = start.ReadU32 ();

  len -= NETLINK_INTERFACE_SIZE;

  while (len)
    {
      NetlinkAttribute attr;

      len -= attr.Deserialize (start, m_attributeTypes);
      m_attributes.push_back (attr);
    }

  return GetSerializedSize ();
}



/***********************************************************************************
* \InterfaceAddressMessage
***********************************************************************************/
InterfaceAddressMessage::InterfaceAddressMessage ()
  : m_length (0),
    m_flags (0),
    m_scope (0),
    m_index (0)
{
  memset ((void*)m_attributeTypes, 0, sizeof (m_attributeTypes));
  m_attributeTypes[IF_A_UNSPEC] = UNSPEC;
  m_attributeTypes[IF_A_ADDRESS] = ADDRESS;
  m_attributeTypes[IF_A_LOCAL] = ADDRESS;
  m_attributeTypes[IF_A_LABEL] = STRING;
  m_attributeTypes[IF_A_BROADCAST] = ADDRESS;
  m_attributeTypes[IF_A_ANYCAST] = ADDRESS;
  m_attributeTypes[IF_A_CACHEINFO] = UNSPEC;
  m_attributeTypes[IF_A_MULTICAST] = ADDRESS;
}
InterfaceAddressMessage::~InterfaceAddressMessage ()
{
}
void
InterfaceAddressMessage::SetFamily (uint8_t family)
{
  m_family = family;
}
void
InterfaceAddressMessage::SetLength (uint8_t length)
{
  m_length = length;
}
void
InterfaceAddressMessage::SetFlags (uint8_t flags)
{
  m_flags = flags;
}
void
InterfaceAddressMessage::SetScope (uint8_t scope)
{
  m_scope = scope;
}
void
InterfaceAddressMessage::SetInterfaceIndex (int32_t index)
{
  m_index = index;
}

uint8_t
InterfaceAddressMessage::GetFamily (void) const
{
  return m_family;
}
uint8_t
InterfaceAddressMessage::GetLength (void) const
{
  return m_length;
}
uint8_t
InterfaceAddressMessage::GetFlags (void) const
{
  return m_flags;
}
uint8_t
InterfaceAddressMessage::GetScope (void) const
{
  return m_scope;
}
int32_t
InterfaceAddressMessage::GetInterfaceIndex (void) const
{
  return m_index;
}

TypeId
InterfaceAddressMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::InterfaceAddressMessage")
    .SetParent<GeneralMessage> ()
    .AddConstructor<InterfaceAddressMessage> ()
  ;
  return tid;
}
TypeId
InterfaceAddressMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
InterfaceAddressMessage::Print (std::ostream &os) const
{
  os << " ----InterfaceAddressMessage ("
     << "family: " << (uint32_t)m_family << " "
     << "length: " << (uint32_t)m_length << " "
     << "flags: " << (uint32_t)m_flags << " "
     << "scope: " << (uint32_t)m_scope << " "
     << "index: " << m_index << ")";
  PrintAttribute (os);
}
uint32_t
InterfaceAddressMessage::GetSerializedSize (void) const
{
  return NETLINK_ADDRESS_SIZE + GetAttributeSerializedSize ();
}

void
InterfaceAddressMessage::Serialize (Buffer::Iterator& start) const
{
  start.WriteU8 (m_family);
  start.WriteU8 (m_length);
  start.WriteU8 (m_flags);
  start.WriteU8 (m_scope);
  start.WriteU32 (m_index);

  SerializeAttribute (start);
}

uint32_t
InterfaceAddressMessage::Deserialize (Buffer::Iterator& start, uint32_t len)
{
  m_family = start.ReadU8 ();
  m_length = start.ReadU8 ();
  m_flags = start.ReadU8 ();
  m_scope = start.ReadU8 ();
  m_index = start.ReadU32 ();

  len -= NETLINK_ADDRESS_SIZE;

  while (len)
    {
      NetlinkAttribute attr;

      len -= attr.Deserialize (start, m_attributeTypes);
      m_attributes.push_back (attr);
    }

  return GetSerializedSize ();
}



/***********************************************************************************
* \ RouteMessage
***********************************************************************************/
RouteMessage::RouteMessage ()
  : m_dstLen (0),
    m_srcLen (0),
    m_tos (0),
    m_tableId (0),
    m_protocol (0),
    m_scope (0),
    m_type (0),
    m_flags (0)
{
  memset ((void*)m_attributeTypes, 0, sizeof (m_attributeTypes));
  m_attributeTypes[RT_A_UNSPEC] = UNSPEC;
  m_attributeTypes[RT_A_DST] = ADDRESS;
  m_attributeTypes[RT_A_SRC] = ADDRESS;
  m_attributeTypes[RT_A_IIF] = U32;
  m_attributeTypes[RT_A_OIF] = U32;
  m_attributeTypes[RT_A_GATEWAY] = ADDRESS;
  m_attributeTypes[RT_A_PRIORITY] = U32;
  m_attributeTypes[RT_A_PREFSRC] = ADDRESS;
  m_attributeTypes[RT_A_METRICS] = UNSPEC;
  //others default value UNSPEC
}
RouteMessage::~RouteMessage ()
{
}

void
RouteMessage::SetFamily (uint8_t v)
{
  m_family = v;
}
void
RouteMessage::SetDstLength (uint8_t v)
{
  m_dstLen = v;
}
void
RouteMessage::SetSrcLength (uint8_t v)
{
  m_srcLen = v;
}
void
RouteMessage::SetTos (uint8_t v)
{
  m_tos = v;
}
void
RouteMessage::SetTableId (uint8_t v)
{
  m_tableId = v;
}
void
RouteMessage::SetProtocol (uint8_t v)
{
  m_protocol = v;
}
void
RouteMessage::SetScope (uint8_t v)
{
  m_scope = v;
}
void
RouteMessage::SetType (uint8_t v)
{
  m_type = v;
}
void
RouteMessage::SetFlags (uint32_t v)
{
  m_flags = v;
}
uint8_t
RouteMessage::GetFamily (void) const
{
  return m_family;
}
uint8_t
RouteMessage::GetDstLength (void) const
{
  return m_dstLen;
}
uint8_t
RouteMessage::GetSrcLength (void) const
{
  return m_srcLen;
}
uint8_t
RouteMessage::GetTos (void) const
{
  return m_tos;
}
uint8_t
RouteMessage::GetTableId (void) const
{
  return m_tableId;
}
uint8_t
RouteMessage::GetProtocol (void) const
{
  return m_protocol;
}
uint8_t
RouteMessage::GetType (void) const
{
  return m_type;
}
uint8_t
RouteMessage::GetScope (void) const
{
  return m_scope;
}
uint32_t
RouteMessage::GetFlags (void) const
{
  return m_flags;
}

TypeId
RouteMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RouteMessage")
    .SetParent<GeneralMessage> ()
    .AddConstructor<RouteMessage> ()
  ;
  return tid;
}
TypeId
RouteMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
RouteMessage::Print (std::ostream &os) const
{
  os << " ----RouteMessage ("
     << "family: " << (uint32_t)m_family << " "
     << "dstLen: " << (uint32_t)m_dstLen << " "
     << "srcLen: " << (uint32_t)m_srcLen << " "
     << "tos: " << (uint32_t)m_tos << " "
     << "tableId: " << (uint32_t)m_tableId << " "
     << "protocol: " << (uint32_t)m_protocol << " "
     << "scope: " << (uint32_t)m_scope << " "
     << "type: " << (uint32_t)m_type << " "
     << "flags: " << m_flags << ")" ;
  PrintAttribute (os);
}
uint32_t
RouteMessage::GetSerializedSize (void) const
{
  return NETLINK_ROUTE_SIZE + GetAttributeSerializedSize ();
}

void
RouteMessage::Serialize (Buffer::Iterator& start) const
{
  start.WriteU8 (m_family);
  start.WriteU8 (m_dstLen);
  start.WriteU8 (m_srcLen);
  start.WriteU8 (m_tos);
  start.WriteU8 (m_tableId);
  start.WriteU8 (m_protocol);
  start.WriteU8 (m_scope);
  start.WriteU8 (m_type);
  start.WriteU32 (m_flags);

  SerializeAttribute (start);
}
uint32_t
RouteMessage::Deserialize (Buffer::Iterator& start, uint32_t len)
{
  m_family = start.ReadU8 ();
  m_dstLen = start.ReadU8 ();
  m_srcLen = start.ReadU8 ();
  m_tos = start.ReadU8 ();
  m_tableId = start.ReadU8 ();
  m_protocol = start.ReadU8 ();
  m_scope = start.ReadU8 ();
  m_type = start.ReadU8 ();
  m_flags = start.ReadU32 ();

  len -= NETLINK_ROUTE_SIZE;

  while (len)
    {
      NetlinkAttribute attr;

      len -= attr.Deserialize (start, m_attributeTypes);
      m_attributes.push_back (attr);
    }

  return GetSerializedSize ();
}
}; // namespace ns3
