/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
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
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 */
#include "dce-node-context.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "utils.h"
#include <string.h>

NS_LOG_COMPONENT_DEFINE("DceNodeContext");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(DceNodeContext);

TypeId
DceNodeContext::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DceNodeContext").SetParent<Object> ()
      .AddConstructor<DceNodeContext> ();

  return tid;
}
TypeId
DceNodeContext::GetInstanceTypeId (void) const
{
  return DceNodeContext::GetTypeId ();
}
DceNodeContext::DceNodeContext ()
{
}

DceNodeContext::~DceNodeContext ()
{
}
int
DceNodeContext::UName (struct utsname *buf)
{
  if ( 0 == sysName.length () )
    {

      uint32_t nodeId = UtilsGetNodeId ();
      Ptr<Node> node = NodeList::GetNode (nodeId);
      NS_ASSERT (node != 0);
      std::string nn = Names::FindName (node);
      std::ostringstream oss;

      if (nn.length() == 0)
        {
          oss << "NODE_" << nodeId;
          nn = oss.str ();

          oss.str ("");
          oss.clear ();
        }
      sysName = nn +"'s OS";
      nodeName = nn;
      release = "3"; // XXX
      version = "11"; // XXX
      oss << nodeId;
      hardId = oss.str ();
    }
  memset (buf, 0 , sizeof(struct utsname) );

  memcpy (buf->sysname, sysName.c_str (), std::min ( (int) sysName.length (), 64) );
  memcpy (buf->nodename, nodeName.c_str (), std::min ( (int) nodeName.length (), 64) );
  memcpy (buf->release, release.c_str (), std::min ( (int) release.length (), 64) );
  memcpy (buf->version, version.c_str (), std::min ( (int) version.length (), 64) );
  memcpy (buf->machine, hardId.c_str (), std::min ( (int) hardId.length (), 64) );

  return 0;
}
} // namespace ns3
