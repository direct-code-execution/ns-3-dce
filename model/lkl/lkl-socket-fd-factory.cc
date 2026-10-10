/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "lkl-socket-fd-factory.h"
#include "lkl-socket-fd.h"
#include "lkl-kernel.h"
#include "task-manager.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/mac48-address.h"
#include "ns3/loopback-net-device.h"
#include "ns3/make-event.h"
#include <lkl/asm/unistd.h>
#include <deque>
#include <cstring>
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

NS_LOG_COMPONENT_DEFINE ("DceLklSocketFdFactory");

/*
 * LKL's network device interface (tools/lkl/include/lkl_host.h and lkl.h).
 * Those headers are not valid C++, so the few definitions needed here are
 * repeated; they must match the LKL version built by utils/build_lkl.sh.
 */
extern "C" {
struct lkl_netdev;
struct lkl_dev_net_ops
{
  int (*tx)(struct lkl_netdev *nd, struct iovec *iov, int cnt);
  int (*rx)(struct lkl_netdev *nd, struct iovec *iov, int cnt);
  int (*poll)(struct lkl_netdev *nd);
  void (*poll_hup)(struct lkl_netdev *nd);
  void (*free)(struct lkl_netdev *nd);
};
struct lkl_netdev
{
  struct lkl_dev_net_ops *ops;
  int id;
  uint8_t has_vnet_hdr : 1;
  uint8_t mac[6];
};
struct lkl_netdev_args
{
  void *mac;
  unsigned int offload;
};
}
#define LKL_DEV_NET_POLL_RX  1
#define LKL_DEV_NET_POLL_HUP 4

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LklSocketFdFactory);

/**
 * A kernel network device backed by an ns-3 NetDevice. LKL drives it
 * through the network device host operations below.
 */
struct LklNetDevice
{
  struct lkl_netdev nd; // must stay first: LKL passes &nd to the operations
  LklSocketFdFactory *factory;
  Ptr<NetDevice> device;
  Mac48Address mac; // the kernel device's address
  std::deque<std::vector<uint8_t> > rx; // received Ethernet frames
  Task *poller;
  bool hup;
  // The kernel had no receive buffers for the queued frames: it takes them
  // when it adds buffers, so do not report them again.
  bool stalled;
};

namespace {

const uint32_t LKL_TASK_STACK = 1 << 18;
const uint32_t ETH_HEADER = 14;
// Received frames waiting for the kernel, like the ring of a network card.
const uint32_t RX_BACKLOG = 1024;

LklNetDevice *
DeviceOf (struct lkl_netdev *nd)
{
  return (LklNetDevice *)nd;
}

void
SendMain (Ptr<NetDevice> device, Ptr<Packet> p, Mac48Address dest, uint16_t protocol)
{
  device->Send (p, dest, protocol);
}

// The kernel sends a frame: hand it to the ns-3 device.
int
NetTx (struct lkl_netdev *nd, struct iovec *iov, int cnt)
{
  LklNetDevice *dev = DeviceOf (nd);
  std::vector<uint8_t> frame;
  for (int i = 0; i < cnt; i++)
    {
      const uint8_t *base = (const uint8_t *)iov[i].iov_base;
      frame.insert (frame.end (), base, base + iov[i].iov_len);
    }
  if (frame.size () < ETH_HEADER)
    {
      return frame.size ();
    }
  Mac48Address dest;
  dest.CopyFrom (&frame[0]);
  uint16_t protocol = (frame[12] << 8) | frame[13];
  NS_LOG_DEBUG ("tx " << dev->device << " " << dest << " protocol " << protocol
                << " " << frame.size () << " bytes");
  Ptr<Packet> p = Create<Packet> (&frame[ETH_HEADER], frame.size () - ETH_HEADER);
  TaskManager::Current ()->ExecOnMain (MakeEvent (&SendMain, dev->device, p, dest, protocol));
  return frame.size ();
}

// The kernel reads the next received frame, if any.
int
NetRx (struct lkl_netdev *nd, struct iovec *iov, int cnt)
{
  LklNetDevice *dev = DeviceOf (nd);
  if (dev->rx.empty ())
    {
      return -1;
    }
  std::vector<uint8_t> &frame = dev->rx.front ();
  size_t done = 0;
  for (int i = 0; i < cnt && done < frame.size (); i++)
    {
      size_t n = std::min (iov[i].iov_len, frame.size () - done);
      memcpy (iov[i].iov_base, &frame[done], n);
      done += n;
    }
  dev->rx.pop_front ();
  dev->stalled = false;
  return done;
}

// Blocks the device's poll task until the kernel can take a received frame.
// Sending never blocks, so transmission readiness is never reported.
int
NetPoll (struct lkl_netdev *nd)
{
  LklNetDevice *dev = DeviceOf (nd);
  TaskManager *manager = TaskManager::Current ();
  while ((dev->rx.empty () || dev->stalled) && !dev->hup)
    {
      dev->poller = manager->RunningTask ();
      manager->Sleep ();
    }
  dev->poller = 0;
  if (dev->hup)
    {
      return LKL_DEV_NET_POLL_HUP;
    }
  // Until the kernel takes a frame or another one arrives.
  dev->stalled = true;
  return LKL_DEV_NET_POLL_RX;
}

void
NetPollHup (struct lkl_netdev *nd)
{
  LklNetDevice *dev = DeviceOf (nd);
  dev->hup = true;
  if (dev->poller)
    {
      TaskManager::Current ()->Wakeup (dev->poller);
    }
}

void
NetFree (struct lkl_netdev *nd)
{
  // The factory owns the device.
}

struct lkl_dev_net_ops g_netOps = {
  &NetTx, &NetRx, &NetPoll, &NetPollHup, &NetFree
};

} // namespace

TypeId
LklSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LklSocketFdFactory")
    .SetParent<SocketFdFactory> ()
    .AddConstructor<LklSocketFdFactory> ()
    .AddAttribute ("Library", "The LKL shared library to load.",
                   StringValue ("liblkl.so"),
                   MakeStringAccessor (&LklSocketFdFactory::m_library),
                   MakeStringChecker ())
    .AddAttribute ("CommandLine", "The kernel command line.",
                   StringValue ("mem=64M loglevel=4"),
                   MakeStringAccessor (&LklSocketFdFactory::m_cmdline),
                   MakeStringChecker ())
    .AddAttribute ("Mptcp",
                   "Create MPTCP sockets for the TCP sockets of applications, "
                   "like mptcpize: Linux uses MPTCP only when asked for.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LklSocketFdFactory::m_mptcp),
                   MakeBooleanChecker ())
  ;
  return tid;
}

LklSocketFdFactory::LklSocketFdFactory ()
  : m_mptcp (false),
    m_booted (false),
    m_nextDevice (0),
    m_epollFd (-1)
{
}

LklSocketFdFactory::~LklSocketFdFactory ()
{
  for (std::vector<LklNetDevice *>::iterator i = m_devices.begin (); i != m_devices.end (); ++i)
    {
      delete *i;
    }
}

void
LklSocketFdFactory::DoDispose (void)
{
  // The devices refer to their node, which refers to this factory.
  for (std::vector<LklNetDevice *>::iterator i = m_devices.begin (); i != m_devices.end (); ++i)
    {
      (*i)->device = 0;
    }
  m_kernel = 0;
  SocketFdFactory::DoDispose ();
}

Ptr<LklKernel>
LklSocketFdFactory::GetKernel (void) const
{
  return m_kernel;
}

void
LklSocketFdFactory::NotifyNewAggregate (void)
{
  Ptr<Node> node = GetObject<Node> ();
  Ptr<TaskManager> manager = GetObject<TaskManager> ();
  if (node && manager && !m_kernel)
    {
      m_kernel = CreateObject<LklKernel> ();
      // Scripts written for libos name its library; LKL replaces it.
      std::string library = m_library == "liblinux.so" ? "liblkl.so" : m_library;
      m_kernel->SetAttribute ("Library", StringValue (library));
      m_kernel->SetAttribute ("CommandLine", StringValue (m_cmdline));
      node->AggregateObject (m_kernel);
      Simulator::ScheduleWithContext (node->GetId (), Seconds (0),
                                      &TaskManager::Start, PeekPointer (manager),
                                      &LklSocketFdFactory::BootTrampoline, (void *)this,
                                      LKL_TASK_STACK);
    }
  SocketFdFactory::NotifyNewAggregate ();
}

void
LklSocketFdFactory::BootTrampoline (void *context)
{
  ((LklSocketFdFactory *)context)->Boot ();
  TaskManager::Current ()->Exit ();
}

void
LklSocketFdFactory::Boot (void)
{
  NS_LOG_FUNCTION (this);
  m_kernel->Boot ();
  // procfs, for sysctls.
  long ret = Call (__lkl__NR_mkdirat, AT_FDCWD, (long)"/proc", 0555);
  NS_ASSERT_MSG (ret == 0, "mkdir /proc failed: " << ret);
  ret = Call (__lkl__NR_mount, (long)"proc", (long)"/proc", (long)"proc", 0, 0);
  NS_ASSERT_MSG (ret == 0, "mount /proc failed: " << ret);
  m_epollFd = Call (__lkl__NR_epoll_create1, 0);
  NS_ASSERT_MSG (m_epollFd >= 0, "epoll_create1 failed: " << m_epollFd);
  m_kernel->GetTaskManager ()->Start (&LklSocketFdFactory::WatcherTrampoline, this, LKL_TASK_STACK);
  // Called right away for the devices the node already has.
  GetObject<Node> ()->RegisterDeviceAdditionListener (MakeCallback (&LklSocketFdFactory::NotifyAddDevice, this));
  m_booted = true;
  for (std::list<Task *>::iterator i = m_bootWaiters.begin (); i != m_bootWaiters.end (); ++i)
    {
      m_kernel->GetTaskManager ()->Wakeup (*i);
    }
  m_bootWaiters.clear ();
  for (std::list<std::pair<std::string, std::string> >::iterator i = m_earlySysctls.begin ();
       i != m_earlySysctls.end (); ++i)
    {
      SetTask (i->first, i->second);
    }
  m_earlySysctls.clear ();
}

namespace {
std::string
SysctlFile (std::string path)
{
  if (!path.empty () && path[0] == '/')
    {
      return path; // any kernel file, e.g. /proc/net/netstat
    }
  std::string file = "/proc/sys";
  if (path.empty () || path[0] != '.')
    {
      file += '/';
    }
  for (std::string::iterator i = path.begin (); i != path.end (); ++i)
    {
      file += *i == '.' ? '/' : *i;
    }
  return file;
}
} // namespace

void
LklSocketFdFactory::Set (std::string path, std::string value)
{
  NS_LOG_FUNCTION (this << path << value);
  if (!m_booted)
    {
      m_earlySysctls.push_back (std::make_pair (path, value));
      return;
    }
  ScheduleTask (MakeEvent (&LklSocketFdFactory::SetTask, this, path, value));
}

void
LklSocketFdFactory::SetTask (std::string path, std::string value)
{
  std::string file = SysctlFile (path);
  long fd = Call (__lkl__NR_openat, AT_FDCWD, (long)file.c_str (), O_WRONLY);
  if (fd < 0)
    {
      NS_LOG_WARN ("sysctl " << path << ": cannot open " << file << ": " << fd);
      return;
    }
  long ret = Call (__lkl__NR_write, fd, (long)value.c_str (), value.size ());
  if (ret < 0)
    {
      NS_LOG_WARN ("sysctl " << path << " = " << value << ": " << ret);
    }
  Call (__lkl__NR_close, fd);
}

std::string
LklSocketFdFactory::Get (std::string path)
{
  NS_LOG_FUNCTION (this << path);
  WaitBooted ();
  std::string file = SysctlFile (path);
  long fd = Call (__lkl__NR_openat, AT_FDCWD, (long)file.c_str (), O_RDONLY);
  if (fd < 0)
    {
      NS_LOG_WARN ("sysctl " << path << ": cannot open " << file << ": " << fd);
      return "";
    }
  std::string value;
  char buffer[4096];
  long n;
  while ((n = Call (__lkl__NR_read, fd, (long)buffer, sizeof (buffer))) > 0)
    {
      value.append (buffer, n);
    }
  Call (__lkl__NR_close, fd);
  return value;
}

void
LklSocketFdFactory::ScheduleTask (EventImpl *event)
{
  m_kernel->GetTaskManager ()->Start (&LklSocketFdFactory::ScheduleTaskTrampoline,
                                      event, LKL_TASK_STACK);
}

void
LklSocketFdFactory::ScheduleTaskTrampoline (void *context)
{
  EventImpl *event = (EventImpl *)context;
  event->Invoke ();
  event->Unref ();
  TaskManager::Current ()->Exit ();
}

void
LklSocketFdFactory::WaitBooted (void)
{
  TaskManager *manager = TaskManager::Current ();
  while (!m_booted)
    {
      m_bootWaiters.push_back (manager->RunningTask ());
      manager->Sleep ();
    }
}

long
LklSocketFdFactory::Call (long no, long a0, long a1, long a2, long a3, long a4, long a5)
{
  long params[6] = { a0, a1, a2, a3, a4, a5 };
  return m_kernel->Syscall (no, params);
}

namespace {
struct AddDeviceContext
{
  LklSocketFdFactory *factory;
  Ptr<NetDevice> device;
};
} // namespace

void
LklSocketFdFactory::NotifyAddDevice (Ptr<NetDevice> device)
{
  TaskManager *manager = m_kernel->GetTaskManager ();
  if (TaskManager::Current () == manager && manager->RunningTask ())
    {
      AddDevice (device);
      return;
    }
  // Adding a kernel device needs a task.
  AddDeviceContext *context = new AddDeviceContext ();
  context->factory = this;
  context->device = device;
  manager->Start (&LklSocketFdFactory::AddDeviceTrampoline, context, LKL_TASK_STACK);
}

void
LklSocketFdFactory::AddDeviceTrampoline (void *c)
{
  AddDeviceContext *context = (AddDeviceContext *)c;
  context->factory->AddDevice (context->device);
  delete context;
  TaskManager::Current ()->Exit ();
}

void
LklSocketFdFactory::AddDevice (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  if (DynamicCast<LoopbackNetDevice> (device))
    {
      return; // the kernel has its own loopback device
    }
  LklNetDevice *dev = new LklNetDevice ();
  memset (&dev->nd, 0, sizeof (dev->nd));
  dev->nd.ops = &g_netOps;
  dev->factory = this;
  dev->device = device;
  dev->poller = 0;
  dev->hup = false;
  dev->stalled = false;
  m_devices.push_back (dev);

  uint8_t mac[6];
  Address address = device->GetAddress ();
  Mac48Address mac48 = Mac48Address::IsMatchingType (address)
    ? Mac48Address::ConvertFrom (address) : Mac48Address::Allocate ();
  mac48.CopyTo (mac);
  dev->mac = mac48;
  struct lkl_netdev_args args;
  memset (&args, 0, sizeof (args));
  args.mac = mac;

  typedef int (*NetdevAdd)(struct lkl_netdev *, struct lkl_netdev_args *);
  NetdevAdd netdevAdd = (NetdevAdd) m_kernel->Lookup ("lkl_netdev_add");
  int id = netdevAdd (&dev->nd, &args);
  NS_ASSERT_MSG (id >= 0, "lkl_netdev_add failed: " << id);

  // Find the new device, named eth<n>, by its address (lkl_netdev_get_ifindex
  // looks for eth<id>, which the devices renamed below break), and name it
  // sim<n> like the libos stack, with the MTU of the ns-3 device.
  long s = Call (__lkl__NR_socket, AF_INET, SOCK_DGRAM, 0);
  NS_ASSERT (s >= 0);
  struct ifreq ifr;
  bool found = false;
  for (int ifindex = 1; ifindex < 4096 && !found; ifindex++)
    {
      memset (&ifr, 0, sizeof (ifr));
      ifr.ifr_ifindex = ifindex;
      if (Call (__lkl__NR_ioctl, s, SIOCGIFNAME, (long)&ifr) != 0
          || strncmp (ifr.ifr_name, "eth", 3) != 0
          || Call (__lkl__NR_ioctl, s, SIOCGIFHWADDR, (long)&ifr) != 0)
        {
          continue;
        }
      found = memcmp (ifr.ifr_hwaddr.sa_data, mac, 6) == 0;
    }
  NS_ASSERT_MSG (found, "no kernel device for " << mac48);
  long ret;
  snprintf (ifr.ifr_newname, IFNAMSIZ, "sim%u", m_nextDevice++);
  ret = Call (__lkl__NR_ioctl, s, SIOCSIFNAME, (long)&ifr);
  NS_ASSERT_MSG (ret == 0, "SIOCSIFNAME failed: " << ret);
  memcpy (ifr.ifr_name, ifr.ifr_newname, IFNAMSIZ);
  ifr.ifr_mtu = device->GetMtu ();
  ret = Call (__lkl__NR_ioctl, s, SIOCSIFMTU, (long)&ifr);
  NS_ASSERT_MSG (ret == 0, "SIOCSIFMTU failed: " << ret);
  if (!device->NeedsArp ())
    {
      // e.g. point-to-point links, which cannot carry ARP: the kernel then
      // sends to neighbors without resolving them.
      ret = Call (__lkl__NR_ioctl, s, SIOCGIFFLAGS, (long)&ifr);
      NS_ASSERT_MSG (ret == 0, "SIOCGIFFLAGS failed: " << ret);
      ifr.ifr_flags |= IFF_NOARP;
      ret = Call (__lkl__NR_ioctl, s, SIOCSIFFLAGS, (long)&ifr);
      NS_ASSERT_MSG (ret == 0, "SIOCSIFFLAGS failed: " << ret);
    }
  Call (__lkl__NR_close, s);

  // LTE devices do not support promiscuous mode: they deliver received
  // packets only to the non-promiscuous handlers.
  TypeId lte;
  bool promiscuous = !(TypeId::LookupByNameFailSafe ("ns3::LteNetDevice", &lte)
                       && device->GetInstanceTypeId ().IsChildOf (lte));
  GetObject<Node> ()->RegisterProtocolHandler (MakeCallback (&LklSocketFdFactory::RxFromDevice, this),
                                               0, device, promiscuous);
}

void
LklSocketFdFactory::RxFromDevice (Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol,
                                  const Address &from, const Address &to, NetDevice::PacketType type)
{
  LklNetDevice *dev = 0;
  for (std::vector<LklNetDevice *>::iterator i = m_devices.begin (); i != m_devices.end (); ++i)
    {
      if ((*i)->device == device)
        {
          dev = *i;
          break;
        }
    }
  if (!dev || dev->rx.size () >= RX_BACKLOG)
    {
      return;
    }
  // Rebuild the Ethernet frame the kernel expects.
  std::vector<uint8_t> frame (ETH_HEADER + p->GetSize ());
  // Devices with other address types (e.g. LTE's Mac64Address) have a kernel
  // device address of their own.
  Mac48Address dest = Mac48Address::IsMatchingType (to)
    ? Mac48Address::ConvertFrom (to) : dev->mac;
  Mac48Address source = Mac48Address::IsMatchingType (from)
    ? Mac48Address::ConvertFrom (from) : Mac48Address ("00:00:00:00:00:00");
  dest.CopyTo (&frame[0]);
  source.CopyTo (&frame[6]);
  frame[12] = protocol >> 8;
  frame[13] = protocol & 0xff;
  p->CopyData (&frame[ETH_HEADER], p->GetSize ());
  NS_LOG_DEBUG ("rx " << device << " protocol " << protocol << " " << frame.size () << " bytes");
  dev->rx.push_back (frame);
  dev->stalled = false;
  if (dev->poller)
    {
      m_kernel->GetTaskManager ()->Wakeup (dev->poller);
    }
}

UnixFd *
LklSocketFdFactory::CreateSocket (int domain, int type, int protocol)
{
  NS_LOG_FUNCTION (this << domain << type << protocol);
  WaitBooted ();
  if (m_mptcp && (domain == AF_INET || domain == AF_INET6)
      && (type & 0xf) == SOCK_STREAM && (protocol == 0 || protocol == IPPROTO_TCP))
    {
      protocol = 262; // IPPROTO_MPTCP
    }
  long fd = Call (__lkl__NR_socket, domain, type, protocol);
  if (fd < 0)
    {
      return 0;
    }
  return new LklSocketFd (this, fd);
}

void
LklSocketFdFactory::Register (LklSocketFd *socket)
{
  m_sockets[socket->GetKernelFd ()] = socket;
}

void
LklSocketFdFactory::Unregister (LklSocketFd *socket)
{
  m_sockets.erase (socket->GetKernelFd ());
  m_watched.erase (socket->GetKernelFd ());
}

void
LklSocketFdFactory::Watch (LklSocketFd *socket)
{
  // Like a kernel wait queue: once watched, every change of the socket's
  // state wakes its poll waiters, which check what they wait for.
  int fd = socket->GetKernelFd ();
  if (m_watched.count (fd))
    {
      return;
    }
  struct epoll_event event;
  memset (&event, 0, sizeof (event));
  event.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLRDHUP | EPOLLET;
  event.data.fd = fd;
  long ret = Call (__lkl__NR_epoll_ctl, m_epollFd, EPOLL_CTL_ADD, fd, (long)&event);
  NS_ASSERT_MSG (ret == 0, "epoll_ctl failed: " << ret);
  m_watched.insert (fd);
}

void
LklSocketFdFactory::WatcherTrampoline (void *context)
{
  ((LklSocketFdFactory *)context)->Watcher ();
}

void
LklSocketFdFactory::Watcher (void)
{
  struct epoll_event events[32];
  while (true)
    {
      long n = Call (__lkl__NR_epoll_pwait, m_epollFd, (long)events, 32, -1, 0, 8);
      for (long i = 0; i < n; i++)
        {
          std::map<int, LklSocketFd *>::iterator s = m_sockets.find (events[i].data.fd);
          if (s != m_sockets.end ())
            {
              s->second->NotifyEvents (events[i].events & 0xffff);
            }
        }
    }
}

} // namespace ns3
