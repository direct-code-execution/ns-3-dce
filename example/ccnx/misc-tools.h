#ifndef MISC_TOOLS_H
#define MISC_TOOLS_H
#include "ns3/network-module.h"
#include "ns3/core-module.h"

namespace ns3 {

void setPos (Ptr<Node> n, int x, int y, int z);

void RunIp (Ptr<Node> node, Time at, std::string str);

void AddAddress (Ptr<Node> node, Time at, const char *name, const char *address);

std::string Ipv4AddressToString (Ipv4Address ad);
}

#endif // MISC_TOOLS_H
