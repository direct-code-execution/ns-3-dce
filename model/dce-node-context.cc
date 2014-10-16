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
#include "ns3/double.h"
#include "utils.h"
#include <string.h>
#include "process.h"
#include "dce-manager.h"

NS_LOG_COMPONENT_DEFINE ("DceNodeContext");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DceNodeContext);

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
  m_randomCtx = CreateObject<NormalRandomVariable> ();
  m_randomCtx->SetAttribute ("Mean", DoubleValue (0)); 
  m_randomCtx->SetAttribute ("Variance", DoubleValue (2 ^ 32 - 1)); 
  m_rndBuffer = m_randomCtx->GetInteger ();
  m_rndOffset = 0;
}

DceNodeContext::~DceNodeContext ()
{
}

int
DceNodeContext::UName (struct utsname *buf)
{
  if (0 == m_sysName.length ())
    {
      uint32_t nodeId = UtilsGetNodeId ();
      Ptr<Node> node = NodeList::GetNode (nodeId);
      NS_ASSERT (node != 0);
      std::string nn = Names::FindName (node);
      std::ostringstream oss;

      if (nn.length () == 0)
        {
          oss << "NODE_" << nodeId;
          nn = oss.str ();

          oss.str ("");
          oss.clear ();
        }
      m_sysName = nn + "'s OS";
      m_nodeName = nn;
      m_release = "3"; // XXX
      m_version = "12"; // XXX
      oss << nodeId;
      m_hardId = oss.str ();
    }
  memset (buf, 0, sizeof (struct utsname));

  memcpy (buf->sysname, m_sysName.c_str (), std::min ((int) m_sysName.length (), 64));
  memcpy (buf->nodename, m_nodeName.c_str (), std::min ((int) m_nodeName.length (), 64));
  memcpy (buf->release, m_release.c_str (), std::min ((int) m_release.length (), 64));
  memcpy (buf->version, m_version.c_str (), std::min ((int) m_version.length (), 64));
  memcpy (buf->machine, m_hardId.c_str (), std::min ((int) m_hardId.length (), 64));

  return 0;
}

Ptr<DceNodeContext>
DceNodeContext::GetNodeContext ()
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current);
  NS_ASSERT (current != 0);
  DceManager *manager = current->process->manager;
  NS_ASSERT (manager != 0);

  return manager->GetObject<DceNodeContext> ();
}

int
DceNodeContext::RandomRead (void *buf, size_t count)
{
  uint8_t *crsr = (uint8_t*)buf;
  for (uint32_t i = 0; i < count; i++)
    {
      *crsr++ = GetNextRnd ();
    }
  return count;
}

uint8_t
DceNodeContext::GetNextRnd ()
{
  uint8_t v = ((uint8_t *)  &m_rndBuffer) [ m_rndOffset++ ];
  if (m_rndOffset >= 4)
    {
      m_rndOffset = 0;
      m_rndBuffer = m_randomCtx->GetInteger ();
    }
  return v;
}

} // namespace ns3
