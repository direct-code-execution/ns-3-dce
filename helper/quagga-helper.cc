/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hajime Tazaki
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
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "ns3/object-factory.h"
#include "quagga-helper.h"
#include "ns3/names.h"
#include "ns3/ipv4-l3-protocol.h"
//#include "ns3/utils.h"
#include <fstream>
#include <map>
#include <sys/stat.h>
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("QuaggaHelper");

namespace ns3 {

class QuaggaConfig: public Object
{
private:
  static int index;
  std::string router_id;
  std::map<std::string, uint32_t> *networks;
public:
  QuaggaConfig ()
    : m_tdpenable (false),
      m_zebradebug (false),
      m_usemanualconf (false)
  {
    m_radvd_if = new std::vector<std::string> ();
    m_egress_if = new std::vector<std::string> ();
    m_haflag_if = new std::vector<std::string> ();
  }
  ~QuaggaConfig ()
  {
  }

  static TypeId 
  GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::QuaggaConfig")
      .SetParent<Object> ()
      .AddConstructor<QuaggaConfig> ()
      ;
    return tid;
  }
  TypeId 
  GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  void
  SetFilename (const std::string &filename)
  {
    m_filename = filename;
  }
  
  std::string
  GetFilename () const 
  {
    return m_filename;
  }

  bool m_tdpenable;
  bool m_zebradebug;
  bool m_usemanualconf;
  std::vector<std::string> *m_radvd_if;
  std::vector<std::string> *m_egress_if;
  std::vector<std::string> *m_haflag_if;

  std::string m_filename;

  std::vector<uint32_t> iflist;

  virtual void
  Print (std::ostream& os) const
  {
    os << "hostname zebra" << std::endl
       << "password zebra" << std::endl
       << "log stdout" << std::endl;
  }
};
std::ostream& operator << (std::ostream& os, QuaggaConfig const& config)
{
  config.Print (os);
  return os;
}

class OspfConfig : public Object
{
private:
  std::map<std::string, uint32_t> *networks;
public:
  OspfConfig ()
    : m_ospfdebug (false)
  {
    networks = new std::map<std::string, uint32_t> ();
    iflist = new std::vector<uint32_t> ();
  }
  ~OspfConfig ()
  {
    delete networks;
    delete iflist;
  }

  bool m_ospfdebug;

  static TypeId 
  GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::OspfConfig")
      .SetParent<Object> ()
      .AddConstructor<OspfConfig> ()
      ;
    return tid;
  }
  TypeId 
  GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  void
  addNetwork (Ipv4Address prefix, uint32_t plen, uint32_t area)
  {
    std::stringstream net;
    net << prefix << "/" << plen;
    networks->insert (std::map<std::string, uint32_t>::value_type (net.str (), area));
  }

  void
  SetFilename (const std::string &filename)
  {
    m_filename = filename;
  }
  
  std::string
  GetFilename () const 
  {
    return m_filename;
  }

  virtual void
  Print (std::ostream& os) const
  {
    os << "hostname zebra" << std::endl
       << "password zebra" << std::endl
       << "log stdout" << std::endl;
    if (m_ospfdebug)
      {
        //os << "log trap errors" << std::endl;
        os << "debug ospf event " << std::endl;
        os << "debug ospf nsm " << std::endl;
        os << "debug ospf ism " << std::endl;
        os << "debug ospf packet all " << std::endl;
      }

    for (std::vector<uint32_t>::iterator i = iflist->begin (); 
         i != iflist->end (); ++i)
      {
        os << "interface ns3-device" << (*i) << std::endl;
      }

    os << "router ospf " << std::endl; 
    //    os << "  ospf router-id " << m_routerId << std::endl;
    for (std::map<std::string, uint32_t>::iterator i = networks->begin (); 
         i != networks->end (); ++i)
      {
        os << "  network " << (*i).first << " area " << (*i).second << std::endl;
      }
    os << "!" << std::endl;
  }
  std::vector<uint32_t> *iflist;
  std::string m_filename;
  uint32_t m_routerId;
};


QuaggaHelper::QuaggaHelper ()
{
}

void 
QuaggaHelper::SetAttribute (std::string name, const AttributeValue &value)
{
}

// OSPF
void
QuaggaHelper::EnableOspf (NodeContainer nodes)
{
  //  for (uint32_t i = 0; i < nodes.GetN (); i++)
  for (uint32_t i = 0; i < nodes.GetN (); i ++)
    {
      Ptr<OspfConfig> ospf_conf = CreateObject<OspfConfig> ();
      nodes.Get (i)->AggregateObject (ospf_conf);

      for (uint32_t j = 0; j < nodes.Get (i)->GetNDevices (); j ++)
        {
          // Use primary address, if multiple
          //      Ipv4Address addr = interfaces->GetAddress (i, 0).GetLocal ();
          // uint32_t ifindex = interfaces.Get (i).second;
          // g_ospf_conf->iflist->insert (std::map<Ptr<Node>, uint32_t>::value_type (nodes.Get (i), ifindex));
        }
    }

  return;
}

void
QuaggaHelper::EnableOspfDebug (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i ++)
    {
      Ptr<OspfConfig> ospf_conf = nodes.Get (i)->GetObject<OspfConfig>();
      if (!ospf_conf)
        {
          ospf_conf = new OspfConfig ();
          nodes.Get (i)->AggregateObject (ospf_conf);
        }
      ospf_conf->m_ospfdebug = true;
    }
  return;
}

void
QuaggaHelper::EnableTdpNina (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i ++)
    {
      Ptr<QuaggaConfig> zebra_conf = nodes.Get (i)->GetObject<QuaggaConfig>();
      if (!zebra_conf)
        {
          zebra_conf = new QuaggaConfig ();
          nodes.Get (i)->AggregateObject (zebra_conf);
        }
      zebra_conf->m_tdpenable = true;
    }
  return;
}

void
QuaggaHelper::EnableZebraDebug (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i ++)
    {
      Ptr<QuaggaConfig> zebra_conf = nodes.Get (i)->GetObject<QuaggaConfig>();
      if (!zebra_conf)
        {
          zebra_conf = new QuaggaConfig ();
          nodes.Get (i)->AggregateObject (zebra_conf);
        }
      zebra_conf->m_zebradebug = true;
    }
  return;
}

void
QuaggaHelper::EnableRadvd (Ptr<Node> node, const char *ifname)
{
  Ptr<QuaggaConfig> zebra_conf = node->GetObject<QuaggaConfig>();
  if (!zebra_conf)
    {
      zebra_conf = new QuaggaConfig ();
      node->AggregateObject (zebra_conf);
    }

  zebra_conf->m_radvd_if->push_back (std::string(ifname));

  return;
}

void
QuaggaHelper::EnableEgressIF (Ptr<Node> node, const char *ifname)
{
  Ptr<QuaggaConfig> zebra_conf = node->GetObject<QuaggaConfig>();
  if (!zebra_conf)
    {
      zebra_conf = new QuaggaConfig ();
      node->AggregateObject (zebra_conf);
    }

  zebra_conf->m_egress_if->push_back (std::string(ifname));

  return;
}

void
QuaggaHelper::EnableHomeAgentFlag (Ptr<Node> node, const char *ifname)
{
  Ptr<QuaggaConfig> zebra_conf = node->GetObject<QuaggaConfig>();
  if (!zebra_conf)
    {
      zebra_conf = new QuaggaConfig ();
      node->AggregateObject (zebra_conf);
    }

  zebra_conf->m_haflag_if->push_back (std::string(ifname));

  return;
}

void
QuaggaHelper::UseManualConfig (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i ++)
    {
      Ptr<QuaggaConfig> zebra_conf = nodes.Get (i)->GetObject<QuaggaConfig>();
      if (!zebra_conf)
        {
          zebra_conf = new QuaggaConfig ();
          nodes.Get (i)->AggregateObject (zebra_conf);
        }
      zebra_conf->m_usemanualconf = true;
    }
  return;
}


void
QuaggaHelper::GenerateConfigZebra (Ptr<Node> node)
{
  Ptr<QuaggaConfig> zebra_conf = node->GetObject<QuaggaConfig>();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  if (zebra_conf->m_usemanualconf)
    {
      return;
    }

  NS_LOG_DEBUG ("ipv4->GetNInterfaces () = " << ipv4->GetNInterfaces ());

//   for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i ++)
//     {
//       Ipv4Address addr = ipv4->GetAddress (i, 0).GetLocal ();
//       Ipv4Mask mask = ipv4->GetAddress (i, 0).GetMask ();
//       Ipv4Address prefix = addr.CombineMask (mask);
// 
//       NS_LOG_DEBUG ("address: " << addr);
// 
//       if (addr.IsEqual (Ipv4Address::GetLoopback()))
//         {
//           continue;
//         }
//       
//       zebra_conf->iflist.push_back (i);
//     }

  // config generation
  std::stringstream conf_dir, conf_file;
  // FIXME XXX
  conf_dir << "files-" << node->GetId () << "";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);
  conf_dir << "/usr/";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);
  conf_dir << "/local/";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);
  conf_dir << "/etc/";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);

  conf_file << conf_dir.str() << "/zebra.conf";
  zebra_conf->SetFilename ("/usr/local/etc/zebra.conf");

  std::ofstream conf;
  conf.open (conf_file.str ().c_str ());
  conf << *zebra_conf;
  if (zebra_conf->m_zebradebug)
    {
      conf << "debug zebra kernel" << std::endl;
      conf << "debug zebra events" << std::endl;
      // conf << "debug zebra route" << std::endl;
      conf << "debug zebra packet" << std::endl;
    }

  // radvd
  for (std::vector<std::string>::iterator i = zebra_conf->m_radvd_if->begin (); 
       i != zebra_conf->m_radvd_if->end (); ++i)
    {
      conf << "interface " << (*i) << std::endl;
      conf << " ipv6 nd ra-interval 5" << std::endl;
      conf << " ipv6 nd prefix-advertisement" << std::endl;
      conf << " no ipv6 nd suppress-ra" << std::endl;
      conf << "!" << std::endl;
    }

  // ha flag
  for (std::vector<std::string>::iterator i = zebra_conf->m_haflag_if->begin (); 
       i != zebra_conf->m_haflag_if->end (); ++i)
    {
      conf << "interface " << (*i) << std::endl;
      conf << " ipv6 nd home-agent-config-flag" << std::endl;
      conf << "!" << std::endl;
    }

  // egress IF
  for (std::vector<std::string>::iterator i = zebra_conf->m_egress_if->begin (); 
       i != zebra_conf->m_egress_if->end (); ++i)
    {
      conf << "interface " << (*i) << std::endl;
      conf << " ipv6 nd td egress" << std::endl;
      conf << "!" << std::endl;
    }

  for (std::vector<uint32_t>::iterator i = zebra_conf->iflist.begin ();
       i != zebra_conf->iflist.end ();
       i++)
    {
      conf << "interface eth" << *i << std::endl;
    }

#if 0
  if (zebra_conf->m_tdpenable)
    {
      conf << "interface sim0" << std::endl;
      conf << " ipv6 nd ra-interval 5" << std::endl;
      conf << " ipv6 nd prefix-advertisement" << std::endl;
      conf << " no ipv6 nd suppress-ra" << std::endl;
      conf << " ipv6 nd home-agent-config-flag" << std::endl;
      conf << "!" << std::endl;

      conf << "interface sim1" << std::endl;
      //      conf << " ipv6 nd td ingress" << std::endl;
      conf << " ipv6 nd ra-interval 5" << std::endl;
      conf << " ipv6 nd prefix-advertisement" << std::endl;
      conf << " no ipv6 nd suppress-ra" << std::endl;
      conf << " ipv6 nd home-agent-config-flag" << std::endl;
      conf << "!" << std::endl;

      conf << "interface sim2" << std::endl;
      //      conf << " ipv6 nd td ingress" << std::endl;
      conf << " ipv6 nd ra-interval 5" << std::endl;
      conf << " ipv6 nd prefix-advertisement" << std::endl;
      conf << " no ipv6 nd suppress-ra" << std::endl;
      conf << " ipv6 nd home-agent-config-flag" << std::endl;
      conf << "!" << std::endl;

      conf << "interface sim3" << std::endl;
      conf << " ipv6 nd ra-interval 5" << std::endl;
      conf << " ipv6 nd prefix-advertisement" << std::endl;
      conf << " no ipv6 nd suppress-ra" << std::endl;
      conf << " ipv6 nd home-agent-config-flag" << std::endl;
      conf << "!" << std::endl;

      conf << "interface sim4" << std::endl;
      conf << " ipv6 nd ra-interval 5" << std::endl;
      conf << " ipv6 nd prefix-advertisement" << std::endl;
      conf << " no ipv6 nd suppress-ra" << std::endl;
      conf << "!" << std::endl;

      conf << "interface sim5" << std::endl;
      conf << " ipv6 nd ra-interval 5" << std::endl;
      conf << " ipv6 nd prefix-advertisement" << std::endl;
      conf << " no ipv6 nd suppress-ra" << std::endl;
      conf << "!" << std::endl;
    }
#endif

  conf.close ();

}

void
QuaggaHelper::GenerateConfigOspf (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);

  Ptr<OspfConfig> ospf_conf = node->GetObject<OspfConfig>();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  
  ospf_conf->m_routerId = 1+node->GetId ();

  NS_LOG_DEBUG ("ipv4->GetNInterfaces () = " << ipv4->GetNInterfaces ());

  for (uint32_t i = 0; i < ipv4->GetNInterfaces (); i ++)
    {
      Ipv4Address addr = ipv4->GetAddress (i, 0).GetLocal ();
      Ipv4Mask mask = ipv4->GetAddress (i, 0).GetMask ();
      Ipv4Address prefix = addr.CombineMask (mask);
      
      NS_LOG_DEBUG ("address: " << addr);

      if (addr.IsEqual (Ipv4Address::GetLoopback()))
        {
          continue;
        }

      ospf_conf->iflist->push_back (i);
      ospf_conf->addNetwork (prefix, mask.GetPrefixLength (), 0);
    }

  // config generation
  std::stringstream conf_dir, conf_file;
  // FIXME XXX
  conf_dir << "files-" << node->GetId () << "";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);
  conf_dir << "/usr/";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);
  conf_dir << "/local/";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);
  conf_dir << "/etc/";
  ::mkdir (conf_dir.str ().c_str (), S_IRWXU | S_IRWXG);

  conf_file << conf_dir.str() << "/ospfd.conf";
  ospf_conf->SetFilename ("/usr/local/etc/ospfd.conf");

  std::ofstream conf;
  conf.open (conf_file.str ().c_str ());
  ospf_conf->Print (conf);
  conf.close ();

}


ApplicationContainer
QuaggaHelper::Install (Ptr<Node> node)
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
QuaggaHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
QuaggaHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

ApplicationContainer
QuaggaHelper::InstallPriv (Ptr<Node> node) 
{
  DceApplicationHelper process;
  ApplicationContainer apps;

  Ptr<QuaggaConfig> zebra_conf = node->GetObject<QuaggaConfig>();
  if (!zebra_conf)
    {
      zebra_conf = new QuaggaConfig ();
      node->AggregateObject (zebra_conf);
    }
  GenerateConfigZebra (node);
  process.SetBinary ("zebra");
  process.AddArguments ("-f", zebra_conf->GetFilename ());
  process.AddArguments ("-i", "/usr/local/etc/zebra.pid");
  process.SetStackSize (1<<16);
  apps.Add (process.Install (node));
  apps.Get(0)->SetStartTime (Seconds (1.0 + 0.01 * node->GetId ()));
  node->AddApplication (apps.Get (0));

  Ptr<OspfConfig> ospf_conf = node->GetObject<OspfConfig>();
  // OSPF
  if (ospf_conf)
    {
      GenerateConfigOspf (node);
      process.ResetArguments ();

      process.SetBinary ("ospfd");
      process.AddArguments ("-f", ospf_conf->GetFilename ());
      process.AddArguments ("-i", "/usr/local/etc/ospfd.pid");
      apps.Add (process.Install (node));
      apps.Get(1)->SetStartTime (Seconds (2.0 + 0.1 * node->GetId ()));
      node->AddApplication (apps.Get (1));
    }

  return apps;
}

} // namespace ns3
