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

#ifndef NETLINK_MESSAGE_ROUTE_H
#define NETLINK_MESSAGE_ROUTE_H

#include "ns3/object-base.h"
#include "ns3/address.h"
#include "netlink-attribute.h"
#include <string>

namespace ns3 {

/*
* this file define some netlink message of NETLINK_ROUTE protocol,
* there are mainly three types:interface address, interface info, route entry
* just implemented them for quagga porting.
*/



/**
* \Types of messages,here we only define the route message types quagga used
*/
enum NetlinkRtmType_e
{
  NETLINK_RTM_BASE = 16,

  NETLINK_RTM_NEWLINK = 16,
  NETLINK_RTM_DELLINK,
  NETLINK_RTM_GETLINK,
  NETLINK_RTM_SETLINK,

  NETLINK_RTM_NEWADDR = 20,
  NETLINK_RTM_DELADDR,
  NETLINK_RTM_GETADDR,

  NETLINK_RTM_NEWROUTE = 24,
  NETLINK_RTM_DELROUTE,
  NETLINK_RTM_GETROUTE,

  NETLINK_RTM_MAX,
};

/**
* \Types of netlink groups,here we only define types quagga used
*/
enum NetlinkRtmGroup_e
{
  NETLINK_RTM_GRP_LINK = 1,
  NETLINK_RTM_GRP_IPV4_IFADDR = 0x10,
  NETLINK_RTM_GRP_IPV4_ROUTE = 0x40,
  RTMGRP_IPV6_IFADDR = 0x100,
  RTMGRP_IPV6_ROUTE = 0x400,
};

class NetlinkPayload : public ObjectBase
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const = 0;
  virtual void Serialize (Buffer::Iterator& start) const = 0;
  virtual void Print (std::ostream &os) const = 0;
  virtual uint32_t GetSerializedSize (void) const = 0;
};

/***
 General form of address family dependent message.

  struct rtgenmsg
  {
    unsigned char		rtgen_family;
  };
**/

class GeneralMessage : public NetlinkPayload
{
public:
  GeneralMessage ();
  virtual ~GeneralMessage ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (Buffer::Iterator& start) const;
  virtual uint32_t Deserialize (Buffer::Iterator& start);
  virtual uint32_t Deserialize (Buffer::Iterator& start, uint32_t len);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;


  virtual uint32_t GetNNetlinkAttribute (void) const;
  virtual NetlinkAttribute GetNetlinkAttribute (uint32_t index) const;
  virtual void AppendAttribute (NetlinkAttribute v);
  virtual void SerializeAttribute (Buffer::Iterator& start) const;
  virtual void PrintAttribute (std::ostream &os) const;
  virtual uint32_t GetAttributeSerializedSize (void) const;
  virtual bool GetAttributeByType (NetlinkAttribute& attr, uint16_t type);


  void SetFamily (uint8_t v);
  uint8_t GetFamily (void) const;

private:
  static const int NETLINK_GENMSG_SIZE = 1; /* size of the struct rtgenmsg */
protected:
  uint8_t m_family;   //always set to AF_UNSPEC
  //attribute can exist or not
  std::vector<NetlinkAttribute> m_attributes;
};



/**
* \brief Link layer specific messages
*
* struct ifinfomsg
* passes link level specific information, not dependent
* on network protocol.
*
  struct ifinfomsg
  {
    unsigned char	ifi_family;
    unsigned char	__ifi_pad;
    unsigned short	ifi_type;
    int		ifi_index;
    unsigned	ifi_flags;
    unsigned	ifi_change;
  };
*/
class InterfaceInfoMessage : public GeneralMessage
{
public:
  InterfaceInfoMessage ();
  virtual ~InterfaceInfoMessage ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (Buffer::Iterator& start) const;
  virtual uint32_t Deserialize (Buffer::Iterator& start, uint32_t len);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;

  enum IflAttr_e
  {
    IFL_A_UNSPEC,
    IFL_A_ADDRESS,
    IFL_A_BROADCAST,
    IFL_A_IFNAME,
    IFL_A_MTU,
    IFL_A_LINK,
    IFL_A_QDISC,
    IFL_A_STATS,
    IFL_A_COST,
    IFL_A_PRIORITY,
    IFL_A_MASTER,
    IFL_A_WIRELESS,
    IFL_A_PROTINFO,
    IFL_A_TXQLEN,
    IFL_A_MAP,
    IFL_A_WEIGHT,
    IFL_A_OPERSTATE,
    IFL_A_LINKMODE,
    IFL_A_MAX,
  };

  enum Type_e
  {
    UP = 1,
    BROADCAST = 2,
    DBG = 4,
  };

  void SetDeviceType (uint16_t type);
  void SetInterfaceIndex (int32_t index);
  void SetDeviceFlags (uint32_t index);
  void SetChangeMask (uint32_t mask);

  uint16_t GetDeviceType (void) const;
  int32_t GetInterfaceIndex (void) const;
  uint32_t GetDeviceFlags (void) const;
  uint32_t GetChangeMask (void) const;
private:
  static const int NETLINK_INTERFACE_SIZE = 16; /* size of the struct ifinfomsg */
  uint8_t m_reserved; //not used
  uint16_t m_deviceType;
  int32_t m_interfaceIndex;
  uint32_t m_deviceFlags;
  uint32_t m_changeMask;
  NetlinkAttributeValueType m_attributeTypes[IFL_A_MAX];
};




/**
* \brief Interface address.
*
  struct ifaddrmsg
  {
  unsigned char	ifa_family;
  unsigned char	ifa_prefixlen;
  unsigned char	ifa_flags;
  unsigned char	ifa_scope;
  int		ifa_index;
  };
*/

class InterfaceAddressMessage : public GeneralMessage
{
public:
  InterfaceAddressMessage ();
  virtual ~InterfaceAddressMessage ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (Buffer::Iterator& start) const;
  virtual uint32_t Deserialize (Buffer::Iterator& start, uint32_t len);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;

  enum IfAttr_e
  {
    IF_A_UNSPEC,
    IF_A_ADDRESS,
    IF_A_LOCAL,
    IF_A_LABEL,
    IF_A_BROADCAST,
    IF_A_ANYCAST,
    IF_A_CACHEINFO,
    IF_A_MULTICAST,
    IF_A_MAX
  };

  enum
  {
    F_SECONDARY = 0x01,
    F_PERMANENT = 0x80,
    F_DEPRECATED = 0x20,
    F_TENTATIVE = 0x40
  };
  enum
  {
    SCOPE_UNIVERSE = 0,
    SCOPE_SITE = 200,
    SCOPE_LINK = 253,
    SCOPE_HOST = 254
  };


  void SetFamily (uint8_t family);
  void SetLength (uint8_t length);
  void SetFlags (uint8_t flags);
  void SetScope (uint8_t scope);
  void SetInterfaceIndex (int32_t index);

  uint8_t GetFamily (void) const;
  uint8_t GetLength (void) const;
  uint8_t GetFlags (void) const;
  uint8_t GetScope (void) const;
  int32_t GetInterfaceIndex (void) const;

private:
  static const int NETLINK_ADDRESS_SIZE = 8; /* size of the struct ifaddrmsg */
  uint8_t m_length;
  uint8_t m_flags;
  uint8_t m_scope;
  int32_t m_index;
  NetlinkAttributeValueType m_attributeTypes[IF_A_MAX];
};


/**
* \brief Definitions used in routing table administration.
*
  struct rtmsg
  {
    unsigned char		rtm_family;
    unsigned char		rtm_dst_len;
    unsigned char		rtm_src_len;
    unsigned char		rtm_tos;

    unsigned char		rtm_table;	// Routing table id
    unsigned char		rtm_protocol;	//Routing protocol;
    unsigned char		rtm_scope;
    unsigned char		rtm_type;

    unsigned		rtm_flags;
  };
*/

class RouteMessage : public GeneralMessage
{
public:
  RouteMessage ();
  virtual ~RouteMessage ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (Buffer::Iterator& start) const;
  virtual uint32_t Deserialize (Buffer::Iterator& start, uint32_t len);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;

  uint8_t GetFamily (void) const;
  uint8_t GetDstLength (void) const;
  uint8_t GetSrcLength (void) const;
  uint8_t GetTos (void) const;
  uint8_t GetTableId (void) const;
  uint8_t GetProtocol (void) const;
  uint8_t GetType (void) const;
  uint8_t GetScope (void) const;
  uint32_t GetFlags (void) const;
  void SetFamily (uint8_t v);
  void SetDstLength (uint8_t v);
  void SetSrcLength (uint8_t v);
  void SetTos (uint8_t v);
  void SetTableId (uint8_t v);
  void SetProtocol (uint8_t v);
  void SetScope (uint8_t v);
  void SetType (uint8_t v);
  void SetFlags (uint32_t v);

  enum RtProtocol_e
  {
    RT_PROT_UNSPEC = 0,
  };

  enum RtFlags_e
  {
    RT_F_CLONED = 0x200,
  };

  enum RtScope_e
  {
    RT_SCOPE_UNIVERSE = 0,
    RT_SCOPE_LINK = 253,
  };

  enum RtClass_e
  {
    RT_TABLE_UNSPEC = 0,
    RT_TABLE_MAIN = 254,
  };

  enum RtAttr_e
  {
    RT_A_UNSPEC,
    RT_A_DST,
    RT_A_SRC,
    RT_A_IIF,
    RT_A_OIF,
    RT_A_GATEWAY,
    RT_A_PRIORITY,
    RT_A_PREFSRC,
    RT_A_METRICS,
    RT_A_MULTIPATH,
    RT_A_PROTOINFO,
    RT_A_FLOW,
    RT_A_CACHEINFO,
    RT_A_SESSION,
    RT_A_MP_ALGO,
    RT_A_TABLE,
    RT_A_MAX
  };


private:
  static const int NETLINK_ROUTE_SIZE = 12; /* size of the struct rtmsg */
  uint8_t m_dstLen;
  uint8_t m_srcLen;
  uint8_t m_tos;
  uint8_t m_tableId;
  uint8_t m_protocol;
  uint8_t m_scope;
  uint8_t m_type;
  uint32_t m_flags;
  NetlinkAttributeValueType m_attributeTypes[RT_A_MAX];
};

}; // namespace ns3

#endif /* NETLINK_MESSAGE_ROUTE_H */
