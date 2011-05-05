#include "ns3-socket-fd-factory.h"
#include "unix-fd.h"
#include "unix-socket-fd.h"
#include "unix-datagram-socket-fd.h"
#include "unix-stream-socket-fd.h"
#include "ns3/netlink-socket-factory.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(Ns3SocketFdFactory);

TypeId 
Ns3SocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ns3SocketFdFactory")
    .SetParent<SocketFdFactory> ()
    .AddConstructor<Ns3SocketFdFactory> ()
    ;
  return tid;
}

Ns3SocketFdFactory::Ns3SocketFdFactory ()
{
  m_netlink = CreateObject<NetlinkSocketFactory> ();
}
UnixFd *
Ns3SocketFdFactory::CreateSocket (int domain, int type, int protocol)
{
  UnixSocketFd *socket = 0;
  Ptr<Socket> sock;

  if (domain == PF_INET)
    {
      switch (type) {
      case SOCK_DGRAM: {
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
	sock = factory->CreateSocket ();
	socket = new UnixDatagramSocketFd (sock);
      } break;
      case SOCK_RAW: {
	TypeId tid = TypeId::LookupByName ("ns3::Ipv4RawSocketFactory");
	Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
	sock = factory->CreateSocket ();
	sock->SetAttribute ("Protocol", UintegerValue (protocol));
	socket = new UnixDatagramSocketFd (sock);
      } break;
      case SOCK_STREAM: {
	TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
	Ptr<SocketFactory> factory = GetObject<SocketFactory> (tid);
	sock = factory->CreateSocket ();
	socket = new UnixStreamSocketFd (sock);
      } break;
      default:
	NS_FATAL_ERROR ("missing socket type");
	break;
      }
    }
  else if (domain == PF_NETLINK)
    {
      switch (type) {
      case SOCK_DGRAM: {
	sock = m_netlink->CreateSocket ();
	socket = new UnixDatagramSocketFd (sock);
      } break;
      default:
	NS_FATAL_ERROR ("missing socket type");
	break;
      }
    }
  else
    {
      NS_FATAL_ERROR ("unsupported domain");
    }

  return socket;
}

} // namespace ns3
