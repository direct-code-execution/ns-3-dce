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

#ifndef NETLINK_MESSAGE_H
#define NETLINK_MESSAGE_H

#include "ns3/header.h"
#include "ns3/address.h"
#include "netlink-message-route.h"
#include "netlink-attribute.h"


namespace ns3 {
class NetlinkPayload;
class GeneralMessage;
class InterfaceAddressMessage;
class InterfaceInfoMessage;
class RouteMessage;
class MultipartNetlinkMessage;

/**
* \brief The Netlink message structure for an netlink packet
*
There are three levels to a Netlink message: The general Netlink
message header, the IP service specific template, and the IP service
specific data.

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                   Netlink message header                      |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                  IP Service Template                          |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                  IP Service specific data in TLVs             |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

enum NetlinkMessageFlag
{
  NETLINK_MSG_F_REQUEST = 1,    // It is request message.
  NETLINK_MSG_F_MULTI = 2,      // Multipart message, terminated by NETLINK_MSG_DONE
  NETLINK_MSG_F_ACK = 4,        // Reply with ack, with zero or error code
  NETLINK_MSG_F_ECHO = 8,       // Echo this request

  /* Modifiers to Get request */
  NETLINK_MSG_F_ROOT = 0x100,          // specify tree root
  NETLINK_MSG_F_MATCH = 0x200,         // return all matching
  NETLINK_MSG_F_ATOMIC = 0x400,        // atomic Get =
  NETLINK_MSG_F_DUMP = (NETLINK_MSG_F_ROOT | NETLINK_MSG_F_MATCH),

  /* Modifiers to NEW request */
  NETLINK_MSG_F_REPLACE = 0x100,   // Override existing =
  NETLINK_MSG_F_EXCL = 0x200,     // Do not touch, if it exists
  NETLINK_MSG_F_CREATE = 0x400,    // Create, if it does not exist
  NETLINK_MSG_F_APPEND = 0x800,    // Add to end of list =
};

enum NetlinkMessageType
{
  NETLINK_MSG_NOOP = 0x1,            // Nothing.
  NETLINK_MSG_ERROR = 0x2,           // Error
  NETLINK_MSG_DONE = 0x3,            // End of a dump
  NETLINK_MSG_OVERRUN = 0x4,         // Data lost
  NETLINK_MSG_MIN_TYPE = 0x10,       // < 0x10: reserved control messages
};

#define NETLINK_MSG_ALIGNTO 4
#define NETLINK_MSG_ALIGN(X)    (((X) + NETLINK_MSG_ALIGNTO - 1) & ~(NETLINK_MSG_ALIGNTO - 1))

class NetlinkMessageHeader : public ObjectBase
{
public:
  NetlinkMessageHeader ();
  NetlinkMessageHeader (uint16_t type, uint16_t flags, uint32_t seq, uint32_t pid);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator& start) const;
  uint32_t Deserialize (Buffer::Iterator& start);

  void SetMsgLen (uint32_t v);
  void SetMsgFlags (uint16_t v);
  void SetMsgType (uint16_t v);
  void SetMsgSeq (uint32_t v);
  void SetMsgPid (uint32_t v);
  uint32_t GetMsgLen (void) const;
  uint16_t GetMsgFlags (void) const;
  uint16_t GetMsgType (void) const;
  uint32_t GetMsgSeq (void) const;
  uint32_t GetMsgPid (void) const;

  static uint32_t GetHeaderSize ();
  uint32_t GetPayloadSize (void) const;

private:
  static const uint32_t NETLINK_MSG_HEADER_SIZE = 16; /* size of the nlmsghdr field*/
  uint32_t m_nlmsgLen;  /* Length of message including header */
  uint16_t m_nlmsgType; /* Message content */
  uint16_t m_nlmsgFlags;        /* Additional flags */
  uint32_t m_nlmsgSeq;  /* Sequence number */
  uint32_t m_nlmsgPid;  /* Sending process PID */
};

/**
* \brief The struct nlmsgerr
*/
class NetlinkMessageError : public NetlinkPayload
{
public:
  NetlinkMessageError ();
  virtual ~NetlinkMessageError ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (Buffer::Iterator& start) const;
  virtual uint32_t Deserialize (Buffer::Iterator& start);
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;

  void SetError (int32_t v);
  int32_t GetError (void) const;
  void SetMsg (NetlinkMessageHeader v);
  NetlinkMessageHeader GetMsg (void) const;

private:
  static const int NETLINK_MSG_ERROR_SIZE = 20; /* size of the nlmsgerror field*/
  int32_t m_error;
  NetlinkMessageHeader m_msg;
};


class NetlinkMessage : public ObjectBase
{
public:
  NetlinkMessage ();

  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator& start) const;
  uint32_t Deserialize (Buffer::Iterator& start);

  operator MultipartNetlinkMessage (void) const;

  uint32_t GetTotalSize (void) const;  //length of netlink message including padding
  uint32_t GetMsgSize (void) const;    //length of netlink message not including padding
  uint32_t GetPayloadSize (void) const; //length of message payload
  uint16_t GetMsgType (void) const;
  uint8_t GetFamily (void) const;

  void SetHeader (NetlinkMessageHeader hdr);
  NetlinkMessageHeader GetHeader (void) const;

  //before set message body, should set header first
  void SetErrorMessage (NetlinkMessageError errmsg);
  void SetGeneralMessage (GeneralMessage genmsg);
  void SetInterfaceInfoMessage (InterfaceInfoMessage v);
  void SetInterfaceAddressMessage (InterfaceAddressMessage v);
  void SetRouteMessage (RouteMessage v);
  NetlinkMessageError GetErrorMessage (void) const;
  GeneralMessage GetGeneralMessage (void) const;
  InterfaceInfoMessage GetInterfaceInfoMessage (void) const;
  InterfaceAddressMessage GetInterfaceAddressMessage (void) const;
  RouteMessage GetRouteMessage (void) const;

  /**
  * \returns true if type was control type, false otherwise.
  */
  static bool IsMessageNetlinkControl (uint16_t type);
  /**
  * \returns true if type was netlink route, false otherwise.
  */
  static bool IsMessageNetlinkRoute (uint16_t type);
  static bool IsMessageAddress (uint16_t type);
  static bool IsMessageInterface (uint16_t type);
  static bool IsMessageRoute (uint16_t type);
  /**
  * \returns true if type was GETxxx , false otherwise.
  */
  static bool IsMessageTypeGet (uint16_t type);
  /**
  * \returns true if flag has ack , false otherwise.
  */
  static bool IsMessageFlagsAck (uint16_t flags);
  /**
  * \returns true if flag has request , false otherwise.
  */
  static bool IsMessageFlagsRequest (uint16_t flags);
  /**
  * \returns true if flag has dump , false otherwise.
  */
  static bool IsMessageFlagsDump (uint16_t flags);

private:
  NetlinkMessageHeader m_hdr;

  //only one type of messages below exists in real world application
  NetlinkMessageError m_errorMessage;
  GeneralMessage m_genmsg;
  InterfaceInfoMessage m_interfaceTemplate;
  InterfaceAddressMessage m_addressTemplate;
  RouteMessage m_routeTemplate;
};

class MultipartNetlinkMessage : public Header
{
public:
  MultipartNetlinkMessage ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void AppendMessage (NetlinkMessage nlmsg);
  void Clear ();
  uint32_t GetNMessages (void) const;
  NetlinkMessage GetMessage (uint32_t index) const;

private:
  std::vector<NetlinkMessage> m_netlinkMessages;
};

}; // namespace ns3

#endif /* NETLINK_MESSAGE_H */
