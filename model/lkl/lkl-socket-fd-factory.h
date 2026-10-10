/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LKL_SOCKET_FD_FACTORY_H
#define LKL_SOCKET_FD_FACTORY_H

#include "socket-fd-factory.h"
#include "ns3/net-device.h"
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ns3 {

class EventImpl;
class LklKernel;
class LklSocketFd;
class Task;
struct LklNetDevice;

/**
 * Network stack of a node backed by a Linux kernel (LKL): select it with
 * DceManagerHelper::SetNetworkStack ("ns3::LklSocketFdFactory").
 *
 * The kernel boots when the simulation starts. Every ns-3 NetDevice of the
 * node but its loopback device (the kernel has its own) becomes a kernel
 * network device named sim<n>, in device order; sockets of DCE
 * applications are kernel sockets.
 */
class LklSocketFdFactory : public SocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  LklSocketFdFactory ();
  virtual ~LklSocketFdFactory ();

  virtual UnixFd * CreateSocket (int domain, int type, int protocol);

  Ptr<LklKernel> GetKernel (void) const;

  /**
   * Write a sysctl, like libos: path ".net.ipv4.tcp_sack" is
   * /proc/sys/net/ipv4/tcp_sack. Sysctls set before the kernel booted are
   * written right after boot.
   */
  void Set (std::string path, std::string value);
  /**
   * Read a sysctl, or any kernel file given by its absolute path (e.g.
   * /proc/net/netstat); must be called from a task of this node.
   */
  std::string Get (std::string path);
  /** Run the event in a new task of this node. */
  void ScheduleTask (EventImpl *event);

  // Used by LklSocketFd.
  void Register (LklSocketFd *socket);
  void Unregister (LklSocketFd *socket);
  /** From now on, wake the socket's poll waiters when its state changes. */
  void Watch (LklSocketFd *socket);

private:
  virtual void DoDispose (void);
  virtual void NotifyNewAggregate (void);
  static void BootTrampoline (void *context);
  static void ScheduleTaskTrampoline (void *context);
  void SetTask (std::string path, std::string value);
  void Boot (void);
  void WaitBooted (void);
  void NotifyAddDevice (Ptr<NetDevice> device);
  void AddDevice (Ptr<NetDevice> device);
  static void AddDeviceTrampoline (void *context);
  void RxFromDevice (Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol,
                     const Address &from, const Address &to, NetDevice::PacketType type);
  static void WatcherTrampoline (void *context);
  void Watcher (void);
  long Call (long no, long a0 = 0, long a1 = 0, long a2 = 0,
             long a3 = 0, long a4 = 0, long a5 = 0);

  std::string m_library;
  std::string m_cmdline;
  bool m_mptcp;
  Ptr<LklKernel> m_kernel;
  bool m_booted;
  std::list<Task *> m_bootWaiters;
  std::list<std::pair<std::string, std::string> > m_earlySysctls;
  std::vector<LklNetDevice *> m_devices;
  uint32_t m_nextDevice;
  int m_epollFd;
  std::map<int, LklSocketFd *> m_sockets;
  std::set<int> m_watched;
};

} // namespace ns3

#endif /* LKL_SOCKET_FD_FACTORY_H */
