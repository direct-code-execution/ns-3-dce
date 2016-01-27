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

#include "netlink-message.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("DceNetlinkMessage");

namespace ns3 {

/***********************************************************************************
* \ NetlinkMessageHeader
***********************************************************************************/
NS_OBJECT_ENSURE_REGISTERED (NetlinkMessageHeader);
NS_OBJECT_ENSURE_REGISTERED (NetlinkMessage);

NetlinkMessageHeader::NetlinkMessageHeader ()
  : m_nlmsgLen (16),
    m_nlmsgType (0),
    m_nlmsgFlags (0),
    m_nlmsgSeq (0),
    m_nlmsgPid (0)
{
}

NetlinkMessageHeader::NetlinkMessageHeader (uint16_t type, uint16_t flags, uint32_t seq, uint32_t pid)
  : m_nlmsgLen (16),
    m_nlmsgType (type),
    m_nlmsgFlags (flags),
    m_nlmsgSeq (seq),
    m_nlmsgPid (pid)
{
}

void
NetlinkMessageHeader::SetMsgLen (uint32_t v)
{
  m_nlmsgLen = v;
}
void
NetlinkMessageHeader::SetMsgFlags (uint16_t v)
{
  m_nlmsgFlags = v;
}
void
NetlinkMessageHeader::SetMsgType (uint16_t v)
{
  m_nlmsgType = v;
}
void
NetlinkMessageHeader::SetMsgSeq (uint32_t v)
{
  m_nlmsgSeq = v;
}
void
NetlinkMessageHeader::SetMsgPid (uint32_t v)
{
  m_nlmsgPid = v;
}
uint16_t
NetlinkMessageHeader::GetMsgFlags (void) const
{
  return m_nlmsgFlags;
}
uint32_t
NetlinkMessageHeader::GetMsgLen (void) const
{
  return m_nlmsgLen;
}
uint16_t
NetlinkMessageHeader::GetMsgType (void) const
{
  return m_nlmsgType;
}
uint32_t
NetlinkMessageHeader::GetMsgSeq (void) const
{
  return m_nlmsgSeq;
}
uint32_t
NetlinkMessageHeader::GetMsgPid (void) const
{
  return m_nlmsgPid;
}
uint32_t
NetlinkMessageHeader::GetHeaderSize ()
{
  return NETLINK_MSG_HEADER_SIZE;
}
uint32_t
NetlinkMessageHeader::GetPayloadSize (void) const
{
  return NETLINK_MSG_ALIGN (m_nlmsgLen - NETLINK_MSG_HEADER_SIZE);
}

TypeId
NetlinkMessageHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetlinkMessageHeader")
    .SetParent<Header> ()
    .AddConstructor<NetlinkMessageHeader> ()
  ;
  return tid;
}
TypeId
NetlinkMessageHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
NetlinkMessageHeader::Print (std::ostream &os) const
{
  os << "NetlinkMessageHeader "
     << "len: " << m_nlmsgLen << " "
     << "flags: " << m_nlmsgFlags << " "
     << "type: " << m_nlmsgType << " "
     << "seq: " << m_nlmsgSeq << " "
     << "pid: " << m_nlmsgPid;
}

uint32_t
NetlinkMessageHeader::GetSerializedSize (void) const
{
  /* this is the size of an nlmsghdr payload. */
  return NETLINK_MSG_HEADER_SIZE;
}

void
NetlinkMessageHeader::Serialize (Buffer::Iterator& start) const
{
  start.WriteU32 (m_nlmsgLen);
  start.WriteU16 (m_nlmsgType);
  start.WriteU16 (m_nlmsgFlags);
  start.WriteU32 (m_nlmsgSeq);
  start.WriteU32 (m_nlmsgPid);
}

uint32_t
NetlinkMessageHeader::Deserialize (Buffer::Iterator& start)
{
  m_nlmsgLen = start.ReadU32 ();
  m_nlmsgType = start.ReadU16 ();
  m_nlmsgFlags = start.ReadU16 ();
  m_nlmsgSeq = start.ReadU32 ();
  m_nlmsgPid = start.ReadU32 ();

  return GetSerializedSize ();
}




/***********************************************************************************
* \ NetlinkMessageError
***********************************************************************************/

NetlinkMessageError::NetlinkMessageError ()
  : m_error (0)
{
}
NetlinkMessageError::~NetlinkMessageError ()
{
}
void
NetlinkMessageError::SetError (int32_t v)
{
  m_error = v;
}
int32_t
NetlinkMessageError::GetError (void) const
{
  return m_error;
}
void
NetlinkMessageError::SetMsg (NetlinkMessageHeader v)
{
  m_msg = v;
}
NetlinkMessageHeader
NetlinkMessageError::GetMsg (void) const
{
  return m_msg;
}

TypeId
NetlinkMessageError::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetlinkMessageError")
    .SetParent<NetlinkPayload> ()
    .AddConstructor<NetlinkMessageError> ()
  ;
  return tid;
}

TypeId
NetlinkMessageError::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
NetlinkMessageError::Print (std::ostream &os) const
{
  os << "----NetlinkMessageError "
     << "error: " << m_error << " "
     << "msg:( ";
  m_msg.Print (os);
  os << " )";
}

uint32_t
NetlinkMessageError::GetSerializedSize (void) const
{
  /* this is the size of an nlmsgerr payload. */
  return NETLINK_MSG_ERROR_SIZE;
}

void
NetlinkMessageError::Serialize (Buffer::Iterator& start) const
{
  start.WriteU32 (m_error);
  m_msg.Serialize (start);
}

uint32_t
NetlinkMessageError::Deserialize (Buffer::Iterator& start)
{
  m_error = start.ReadU32 ();
  m_msg.Deserialize (start);

  return GetSerializedSize ();
}




/***********************************************************************************
* \ NetlinkMessage
***********************************************************************************/
NetlinkMessage::NetlinkMessage ()
{
}

void
NetlinkMessage::SetHeader (NetlinkMessageHeader hdr)
{
  m_hdr = hdr;
}
NetlinkMessageHeader
NetlinkMessage::GetHeader (void) const
{
  return m_hdr;
}
void
NetlinkMessage::SetGeneralMessage (GeneralMessage genmsg)
{
  m_genmsg = genmsg;
  m_hdr.SetMsgLen (m_hdr.GetSerializedSize () + genmsg.GetSerializedSize ());
}
void
NetlinkMessage::SetErrorMessage (NetlinkMessageError errmsg)
{
  m_errorMessage = errmsg;
  m_hdr.SetMsgLen (m_hdr.GetSerializedSize () + errmsg.GetSerializedSize ());
}
// the type is one of RTM_NEWLINK,RTM_DELLINK,RTM_GETLINK
void
NetlinkMessage::SetInterfaceInfoMessage (InterfaceInfoMessage v)
{
  m_hdr.SetMsgLen (m_hdr.GetSerializedSize () + v.GetSerializedSize ());
  m_interfaceTemplate = v;
}
// the type is one of RTM_NEWADDR,RTM_DELADDR,RTM_GETADDR
void NetlinkMessage::SetInterfaceAddressMessage (InterfaceAddressMessage v)
{
  m_hdr.SetMsgLen (m_hdr.GetSerializedSize () + v.GetSerializedSize ());
  m_addressTemplate = v;
}
// the type  is one of RTM_NEWROUTE,RTM_DELROUTE,RTM_GETROUTE
void NetlinkMessage::SetRouteMessage (RouteMessage v)
{
  m_hdr.SetMsgLen (m_hdr.GetSerializedSize () + v.GetSerializedSize ());
  m_routeTemplate = v;
}
GeneralMessage
NetlinkMessage::GetGeneralMessage (void) const
{
  return m_genmsg;
}
NetlinkMessageError
NetlinkMessage::GetErrorMessage (void) const
{
  return m_errorMessage;
}
InterfaceInfoMessage
NetlinkMessage::GetInterfaceInfoMessage (void) const
{
  return m_interfaceTemplate;
}
InterfaceAddressMessage
NetlinkMessage::GetInterfaceAddressMessage (void) const
{
  return m_addressTemplate;
}
RouteMessage
NetlinkMessage::GetRouteMessage (void) const
{
  return m_routeTemplate;
}
bool
NetlinkMessage::IsMessageNetlinkControl (uint16_t type)
{
  return (type < NETLINK_RTM_BASE);
}
bool
NetlinkMessage::IsMessageNetlinkRoute (uint16_t type)
{
  return (type >= NETLINK_RTM_BASE && type < NETLINK_RTM_MAX);
}
bool
NetlinkMessage::IsMessageAddress (uint16_t type)
{
  return (type >= NETLINK_RTM_NEWADDR && type <= NETLINK_RTM_GETADDR);
}
bool
NetlinkMessage::IsMessageInterface (uint16_t type)
{
  return (type >= NETLINK_RTM_NEWLINK && type <= NETLINK_RTM_SETLINK);
}
bool
NetlinkMessage::IsMessageRoute (uint16_t type)
{
  return (type >= NETLINK_RTM_NEWROUTE && type <= NETLINK_RTM_GETROUTE);
}
bool
NetlinkMessage::IsMessageTypeGet (uint16_t type)
{
  return (((type - NETLINK_RTM_BASE) & 3) == 2);
}
bool
NetlinkMessage::IsMessageFlagsAck (uint16_t flags)
{
  return (flags & NETLINK_MSG_F_ACK) ? true : false;
}
bool
NetlinkMessage::IsMessageFlagsRequest (uint16_t flags)
{
  return (flags & NETLINK_MSG_F_REQUEST) ? true : false;
}
bool
NetlinkMessage::IsMessageFlagsDump (uint16_t flags)
{
  return (flags & NETLINK_MSG_F_DUMP) ? true : false;
}

NetlinkMessage::operator MultipartNetlinkMessage (void) const
{
  MultipartNetlinkMessage multi_nlmsg;
  multi_nlmsg.AppendMessage (*this);
  return multi_nlmsg;
}

TypeId
NetlinkMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetlinkMessage")
    .SetParent<Header> ()
    .AddConstructor<NetlinkMessage> ()
  ;
  return tid;
}
TypeId
NetlinkMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
NetlinkMessage::GetTotalSize (void) const
{
  return NETLINK_MSG_ALIGN (m_hdr.GetMsgLen ());
}

uint32_t
NetlinkMessage::GetMsgSize (void) const
{
  return m_hdr.GetMsgLen ();
}

uint32_t
NetlinkMessage::GetPayloadSize (void) const
{
  return m_hdr.GetPayloadSize ();
}
uint16_t
NetlinkMessage::GetMsgType (void) const
{
  return m_hdr.GetMsgType ();
}

uint8_t
NetlinkMessage::GetFamily (void) const
{
  if (IsMessageTypeGet (GetMsgType ()))
    {
      NS_LOG_DEBUG ("TypeGetMsg");
    }
  if (IsMessageAddress (m_hdr.GetMsgType ()))
    {
      return m_addressTemplate.GetFamily ();
    }
  else if (IsMessageInterface (m_hdr.GetMsgType ()))
    {
      return m_interfaceTemplate.GetFamily ();
    }
  else if (IsMessageRoute (m_hdr.GetMsgType ()))
    {
      return m_routeTemplate.GetFamily ();
    }
  else if (IsMessageFlagsDump (m_hdr.GetMsgFlags ()))
    {
      return m_genmsg.GetFamily (); //value is said to be always set to AF_UNSPEC
    }
  else
    {
      NS_LOG_WARN ("Netlink message type not supported, return AF_UNSPEC");
      return 0;
    }
}

void
NetlinkMessage::Print (std::ostream &os) const
{
  uint16_t type = m_hdr.GetMsgType ();

  os << "NetlinkMessage  ";
  os << " ----Header:(";
  m_hdr.Print (os);
  os << ")";

  if (type == NETLINK_MSG_DONE)
    {
      os << "\nmultipart message ends here";
    }
  else if (type == NETLINK_MSG_ERROR)
    {
      m_errorMessage.Print (os);
    }
  else if (type == NETLINK_RTM_GETROUTE || type == NETLINK_RTM_GETADDR || type == NETLINK_RTM_GETLINK)
    {
      m_genmsg.Print (os);
    }
  else if (type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE)
    {
      m_routeTemplate.Print (os);
    }
  else if (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR)
    {
      m_addressTemplate.Print (os);
    }
  else if (type == NETLINK_RTM_NEWLINK || type == NETLINK_RTM_DELLINK)
    {
      m_interfaceTemplate.Print (os);
    }
  else
    {
      os << "service not supported yet( " << type << ")";
    }
}
uint32_t
NetlinkMessage::GetSerializedSize (void) const
{
  return NETLINK_MSG_ALIGN (m_hdr.GetMsgLen ());
}

void
NetlinkMessage::Serialize (Buffer::Iterator& start) const
{
  NS_LOG_FUNCTION (this);
  //  Print (std::cout);
  uint16_t type = m_hdr.GetMsgType ();

  m_hdr.Serialize (start);

  if (type == NETLINK_MSG_DONE)
    {
      //nothing done
    }
  else if (type == NETLINK_MSG_ERROR)
    {
      m_errorMessage.Serialize (start);
    }
  else if (NetlinkMessage::IsMessageFlagsDump (m_hdr.GetMsgFlags ())
           && (type == NETLINK_RTM_GETROUTE || type == NETLINK_RTM_GETADDR || type == NETLINK_RTM_GETLINK))
    {
      m_genmsg.Serialize (start);
    }
  else if (type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE || type == NETLINK_RTM_GETROUTE)
    {
      m_routeTemplate.Serialize (start);
    }
  else if (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR)
    {
      m_addressTemplate.Serialize (start);
    }
  else if (type == NETLINK_RTM_NEWLINK || type == NETLINK_RTM_SETLINK)
    {
      m_interfaceTemplate.Serialize (start);
    }
  else
    {
    }
}


uint32_t
NetlinkMessage::Deserialize (Buffer::Iterator&start)
{
  uint32_t remaining;

  m_hdr.Deserialize (start);
  remaining = NETLINK_MSG_ALIGN (m_hdr.GetMsgLen ()) - m_hdr.GetSerializedSize ();

  //Deserialize service module
  uint16_t type = GetMsgType ();
  if (remaining)
    {
      if (type == NETLINK_MSG_DONE)
        {
          //do nothing
        }
      else if (type == NETLINK_MSG_ERROR)
        {
          remaining -= m_errorMessage.Deserialize (start);
        }
      else if (NetlinkMessage::IsMessageFlagsDump (m_hdr.GetMsgFlags ())
               && (type == NETLINK_RTM_GETROUTE || type == NETLINK_RTM_GETADDR || type == NETLINK_RTM_GETLINK))
        {
          remaining -= m_genmsg.Deserialize (start, remaining);
        }
      else if (type == NETLINK_RTM_NEWROUTE || type == NETLINK_RTM_DELROUTE || type == NETLINK_RTM_GETROUTE)
        {
          remaining -= m_routeTemplate.Deserialize (start, remaining);
        }
      else if (type == NETLINK_RTM_NEWADDR || type == NETLINK_RTM_DELADDR)
        {
          remaining -= m_addressTemplate.Deserialize (start, remaining);
        }
      else if (type == NETLINK_RTM_NEWLINK || type == NETLINK_RTM_SETLINK)
        {
          remaining -= m_interfaceTemplate.Deserialize (start, remaining);
        }
      else
        {
          //do nothing
        }
    }

  return GetSerializedSize ();
}


/***********************************************************************************
* \ MultipartNetlinkMessage
***********************************************************************************/
MultipartNetlinkMessage::MultipartNetlinkMessage ()
{
}

TypeId
MultipartNetlinkMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MultipartNetlinkMessage")
    .SetParent<Header> ()
    .AddConstructor<MultipartNetlinkMessage> ()
  ;
  return tid;
}
TypeId
MultipartNetlinkMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
MultipartNetlinkMessage::Print (std::ostream &os) const
{
  for (uint32_t i = 0; i <  m_netlinkMessages.size (); i++)
    {
      m_netlinkMessages[i].Print (os);
      os << std::endl;
    }
}
uint32_t
MultipartNetlinkMessage::GetSerializedSize (void) const
{
  uint32_t len = 0;

  for (uint32_t i = 0; i <  m_netlinkMessages.size (); i++)
    {
      len +=  m_netlinkMessages[i].GetSerializedSize ();
    }
  return len;
}
void
MultipartNetlinkMessage::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION ("Multi" << this);
  for (uint32_t i = 0; i <  m_netlinkMessages.size (); i++)
    {
      m_netlinkMessages[i].Serialize (start);
    }
}
uint32_t
MultipartNetlinkMessage::Deserialize (Buffer::Iterator start)
{
  while (1)
    {
      NetlinkMessage nlmsg;
      nlmsg.Deserialize (start);
      AppendMessage (nlmsg);

      if (!(nlmsg.GetHeader ().GetMsgFlags () & NETLINK_MSG_F_MULTI))
        {
          break;
        }

      if (nlmsg.GetHeader ().GetMsgType () == NETLINK_MSG_DONE)
        {
          break;
        }
    }
  return GetSerializedSize ();
}

void
MultipartNetlinkMessage::AppendMessage (NetlinkMessage nlmsg)
{
  m_netlinkMessages.push_back (nlmsg);
}

void
MultipartNetlinkMessage::Clear ()
{
  m_netlinkMessages.clear ();
}

uint32_t
MultipartNetlinkMessage::GetNMessages (void) const
{
  return m_netlinkMessages.size ();
}
NetlinkMessage
MultipartNetlinkMessage::GetMessage (uint32_t index) const
{
  NS_ASSERT (index < m_netlinkMessages.size ());
  return m_netlinkMessages[index];
}

}; // namespace ns3
