#include "kernel-socket-fd-factory.h"
#include "kernel-socket-fd.h"
#include "loader-factory.h"
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include "wait-queue.h"
#include "task-manager.h"
#include "kingsley-alloc.h"
#include "file-usage.h"
#include "dce-unistd.h"
#include "dce-stdlib.h"
#include "sys/dce-stat.h"
#include "dce-fcntl.h"
#include "dce-stdio.h"
#include "include/sim-init.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/random-variable-stream.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/packet.h"
#include "exec-utils.h"
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>

NS_LOG_COMPONENT_DEFINE ("DceKernelSocketFdFactory");

namespace ns3 {

// Sadly NetDevice Callback add by method AddLinkChangeCallback take no parameters ..
// .. so we need to create the following class to link NetDevice and KernelSocketFdFactory together
// in order to do Warn the factory about which NetDevice is changing .
class KernelDeviceStateListener : public SimpleRefCount<KernelDeviceStateListener>
{
public:
  KernelDeviceStateListener (Ptr<NetDevice>, Ptr<KernelSocketFdFactory>);

  void NotifyDeviceStateChange ();

private:
  Ptr<NetDevice> m_netDevice;
  Ptr<KernelSocketFdFactory> m_factory;
};

KernelDeviceStateListener::KernelDeviceStateListener (Ptr<NetDevice> d,
                                                    Ptr<KernelSocketFdFactory> f)
  : m_netDevice (d),
    m_factory (f)
{
}

void
KernelDeviceStateListener::NotifyDeviceStateChange ()
{
  m_factory->NotifyDeviceStateChange (m_netDevice);
}

NS_OBJECT_ENSURE_REGISTERED (KernelSocketFdFactory);

TypeId
KernelSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::KernelSocketFdFactory")
    .SetParent<SocketFdFactory> ()
    .AddConstructor<KernelSocketFdFactory> ()
    .AddAttribute ("ErrorRate", "The error rate of malloc().",
                   DoubleValue (DoubleValue (0.0)),
                   MakeDoubleAccessor (&KernelSocketFdFactory::m_rate),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RanVar", "The decision variable attached to this error model.",
                   StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"),
                   MakePointerAccessor (&KernelSocketFdFactory::m_ranvar),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}
KernelSocketFdFactory::KernelSocketFdFactory ()
  : m_loader (0),
    m_exported (0),
    m_alloc (new KingsleyAlloc ()),
    m_logFile (0)
{
  TypeId::LookupByNameFailSafe ("ns3::LteUeNetDevice", &m_lteUeTid);
  m_variable = CreateObject<UniformRandomVariable> ();
}

KernelSocketFdFactory::~KernelSocketFdFactory ()
{
  for (uint32_t i = 0; i < m_devices.size (); i++)
    {
      // Note: we don't really destroy devices from here
      // because calling destroy requires a task context
      // m_exported->dev_destroy(m_devices[i].second);
    }
  delete m_exported;
  delete m_loader;
  delete m_alloc;
  if (m_logFile != 0)
    {
      fclose (m_logFile);
    }
  m_exported = 0;
  m_loader = 0;
  m_alloc = 0;
  m_logFile = 0;
}

void
KernelSocketFdFactory::DoDispose (void)
{
  for (std::list<Task *>::const_iterator i = m_kernelTasks.begin (); i != m_kernelTasks.end (); ++i)
    {
      m_manager->Stop (*i);
    }
  m_kernelTasks.clear ();
  m_manager = 0;
  m_listeners.clear ();
}

int
KernelSocketFdFactory::Vprintf (struct SimKernel *kernel, const char *str, va_list args)
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  return vfprintf (self->m_logFile, str, args);
}

void *
KernelSocketFdFactory::Malloc (struct SimKernel *kernel, unsigned long size)
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  if (self->m_ranvar->GetValue () < self->m_rate)
    {
      NS_LOG_DEBUG ("return null");
      // Inject fault
      return NULL;
    }

  size += sizeof (size_t);
  uint8_t *buffer = self->m_alloc->Malloc (size);
  memcpy (buffer, &size, sizeof (size_t));
  buffer += sizeof (size_t);
  return buffer;
}
void
KernelSocketFdFactory::Free (struct SimKernel *kernel, void *ptr)
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  uint8_t *buffer = (uint8_t*)ptr;
  size_t size;
  buffer -= sizeof (size_t);
  memcpy (&size, buffer, sizeof (size_t));
  self->m_alloc->Free (buffer, size);
}
void *
KernelSocketFdFactory::Memcpy (struct SimKernel *kernel, void *dst, const void *src, unsigned long size)
{
  return memcpy (dst, src, size);
}
void *
KernelSocketFdFactory::Memset (struct SimKernel *kernel, void *dst, char value, unsigned long size)
{
  return memset (dst, value, size);
}
int
KernelSocketFdFactory::AtExit (struct SimKernel *kernel, void (*function)(void))
{
  NS_LOG_FUNCTION (kernel << function);
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  Ptr<DceManager> manager = self->GetObject<DceManager> ();
  Process *p = manager->SearchProcess (self->m_pid);

  // Register process-level atexit store
  struct AtExitHandler handler;
  handler.type = AtExitHandler::NORMAL;
  handler.value.normal = function;
  p->atExitHandlers.push_back (handler);
  return 0;
}
int
KernelSocketFdFactory::Access (struct SimKernel *kernel, const char *pathname, int mode)
{
  return dce_access (pathname, mode);
}
char*
KernelSocketFdFactory::Getenv (struct SimKernel *kernel, const char *name)
{
  return dce_getenv (name);
}
int
KernelSocketFdFactory::Mkdir (struct SimKernel *kernel, const char *pathname, mode_t mode)
{
  return dce_mkdir (pathname, mode);
}
int
KernelSocketFdFactory::Open (struct SimKernel *kernel, const char *pathname, int flags)
{
  return dce_open (pathname, flags, 0666);
}
int
KernelSocketFdFactory::__Fxstat (struct SimKernel *kernel, int ver, int fd, void *buf)
{
  return dce___fxstat (ver, fd, (struct stat *)buf);
}
int
KernelSocketFdFactory::Fseek (struct SimKernel *kernel, FILE *stream, long offset, int whence)
{
  return dce_fseek (stream, offset, whence);
}
void
KernelSocketFdFactory::Setbuf (struct SimKernel *kernel, FILE *stream, char *buf)
{
  return dce_setbuf (stream, buf);
}
long
KernelSocketFdFactory::Ftell (struct SimKernel *kernel, FILE *stream)
{
  return dce_ftell (stream);
}
FILE*
KernelSocketFdFactory::FdOpen (struct SimKernel *kernel, int fd, const char *mode)
{
  return dce_fdopen (fd, mode);
}
size_t
KernelSocketFdFactory::Fread (struct SimKernel *kernel, void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return dce_fread (ptr, size, nmemb, stream);
}
size_t
KernelSocketFdFactory::Fwrite (struct SimKernel *kernel, const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return dce_fwrite (ptr, size, nmemb, stream);
}
int
KernelSocketFdFactory::Fclose (struct SimKernel *kernel, FILE *fp)
{
  return dce_fclose (fp);
}
unsigned long
KernelSocketFdFactory::Random (struct SimKernel *kernel)
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  union
  {
    uint8_t buffer[sizeof(unsigned long)];
    unsigned long v;
  } u;
  for (uint8_t i = 0; i < sizeof (u.buffer); i++)
    {
      u.buffer[i] = self->m_variable->GetInteger (0,255);
    }
  return u.v;
}
void
KernelSocketFdFactory::EventTrampoline (void (*fn)(void *context),
                                       void *context, void (*pre_fn)(void),
                                       Ptr<EventIdHolder> event)
{
  m_loader->NotifyStartExecute ();
  pre_fn ();
  fn (context);
  m_loader->NotifyEndExecute ();
}
void *
KernelSocketFdFactory::EventScheduleNs (struct SimKernel *kernel, __u64 ns, void (*fn)(void *context), void *context,
                                       void (*pre_fn)(void))
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  Ptr<EventIdHolder> ev = Create<EventIdHolder> ();
  TaskManager *manager = TaskManager::Current ();

  ev->id = manager->ScheduleMain (NanoSeconds (ns),
                                  MakeEvent (&KernelSocketFdFactory::EventTrampoline, self, fn, context, pre_fn, ev));

  return &ev->id;
}
void
KernelSocketFdFactory::EventCancel (struct SimKernel *kernel, void *ev)
{
  EventId *event = (EventId *)ev;
  Simulator::Remove (*event);
}
static __u64 CurrentNs (struct SimKernel *kernel)
{
  return Simulator::Now ().GetNanoSeconds ();
}

void
KernelSocketFdFactory::TaskSwitch (enum Task::SwitchType type, void *context)
{
  NS_LOG_FUNCTION (type << context);
  Loader *loader = (Loader *)context;
  switch (type)
    {
    case Task::TO:
      loader->NotifyStartExecute ();
      break;
    case Task::FROM:
      loader->NotifyEndExecute ();
      break;
    }
}

struct SimTask *
KernelSocketFdFactory::TaskStart (struct SimKernel *kernel, void (*callback)(void *), void *context)
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  Task *task = self->m_manager->Start (callback, context, 1 << 17);
  struct SimTask *simTask = self->m_exported->task_create (task, 0);
  task->SetExtraContext (simTask);
  task->SetSwitchNotifier (&KernelSocketFdFactory::TaskSwitch, self->m_loader);
  self->m_kernelTasks.push_back (task);
  return (struct SimTask *)task->GetExtraContext ();
}
struct SimTask *
KernelSocketFdFactory::TaskCurrent (struct SimKernel *kernel)
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  TaskManager *manager = TaskManager::Current ();
  Task *current = manager->CurrentTask ();
  if (current->GetExtraContext () == 0)
    {
      uint32_t pid = 0;
      struct Thread *thread = (struct Thread *)current->GetContext ();
      if (thread != 0)
        {
          pid = thread->process->pid;
        }
      struct SimTask *simTask = self->m_exported->task_create (current, pid);
      current->SetExtraContext (simTask);
    }
  return (struct SimTask *)current->GetExtraContext ();
}
void
KernelSocketFdFactory::TaskWait (struct SimKernel *kernel)
{
  // force initialization of 'current'
  TaskCurrent (kernel);
  // now, sleep.
  TaskManager::Current ()->Sleep ();
}
int
KernelSocketFdFactory::TaskWakeup (struct SimKernel *kernel, struct SimTask *task)
{
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  TaskManager *manager = TaskManager::Current ();
  if (!manager)
    {
      return 1;
    }
  Task *other = (Task *)self->m_exported->task_get_private (task);
  bool isBlocked = other->IsBlocked ();
  manager->Wakeup (other);
  return isBlocked ? 1 : 0;
}
void
KernelSocketFdFactory::TaskYield (struct SimKernel *kernel)
{
  // force initialization of 'current'
  TaskCurrent (kernel);
  // now, yield.
  TaskManager::Current ()->Yield ();
}
void
KernelSocketFdFactory::SendMain (bool *r, NetDevice *dev, Ptr<Packet> p, const Address& d, uint16_t pro)
{
  *r = dev->Send (p, d, pro);
}
void
KernelSocketFdFactory::DevXmit (struct SimKernel *kernel, struct SimDevice *dev, unsigned char *data, int len)
{
  NS_LOG_FUNCTION (dev);
  KernelSocketFdFactory *self = (KernelSocketFdFactory *)kernel;
  NetDevice *nsDev = (NetDevice *)self->m_exported->dev_get_private (dev);
  NS_ASSERT (len >= 14);

  struct ethhdr
  {
    unsigned char   h_dest[6];
    unsigned char   h_source[6];
    uint16_t        h_proto;
  } *hdr = (struct ethhdr *)data;
  data += 14;
  len -= 14;
  Ptr<Packet> p = Create<Packet> (data, len);
  uint16_t protocol = ntohs (hdr->h_proto);
  Mac48Address dest;
  dest.CopyFrom (hdr->h_dest);
  TaskManager *manager = TaskManager::Current ();
  bool r = false;

  manager->ExecOnMain (MakeEvent (&KernelSocketFdFactory::SendMain, &r, nsDev, p, dest, protocol));
}

void
KernelSocketFdFactory::SignalRaised (struct SimKernel *kernel, struct SimTask *task, int signalNumber)
{
  NS_LOG_FUNCTION ("XXX: Not Yet Implemented " << signalNumber);
}

struct SimDevice *
KernelSocketFdFactory::DevToDev (Ptr<NetDevice> device)
{
  for (uint32_t i = 0; i < m_devices.size (); i++)
    {
      if (m_devices[i].first == device)
        {
          struct SimDevice *dev = m_devices[i].second;
          return dev;
        }
    }
  return 0;
}

void
KernelSocketFdFactory::RxFromDevice (Ptr<NetDevice> device, Ptr<const Packet> p,
                                    uint16_t protocol, const Address & from,
                                    const Address &to, NetDevice::PacketType type)
{
  struct SimDevice *dev = DevToDev (device);
  if (dev == 0)
    {
      return;
    }
  m_loader->NotifyStartExecute (); // Restore the memory of the kernel before access it !
  struct SimDevicePacket packet = m_exported->dev_create_packet (dev, p->GetSize () + 14);
  p->CopyData (((unsigned char *)packet.buffer) + 14, p->GetSize ());
  struct ethhdr
  {
    unsigned char   h_dest[6];
    unsigned char   h_source[6];
    uint16_t        h_proto;
  } *hdr = (struct ethhdr *)packet.buffer;
  if (device->GetInstanceTypeId () != m_lteUeTid)
    {
      Mac48Address realFrom = Mac48Address::ConvertFrom (from);
      realFrom.CopyTo (hdr->h_source);
    }
  Mac48Address realTo = Mac48Address::ConvertFrom (to);
  realTo.CopyTo (hdr->h_dest);
  hdr->h_proto = ntohs (protocol);
  m_exported->dev_rx (dev, packet);
  m_loader->NotifyEndExecute ();
}

void
KernelSocketFdFactory::NotifyDeviceStateChange (Ptr<NetDevice> device)
{
  ScheduleTask (MakeEvent (&KernelSocketFdFactory::NotifyDeviceStateChangeTask,
                           this, device));
}
void
KernelSocketFdFactory::NotifyDeviceStateChangeTask (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (device);
  struct SimDevice *dev = DevToDev (device);
  if (dev == 0)
    {
      return;
    }
  Mac48Address ad = Mac48Address::ConvertFrom (device->GetAddress ());
  uint8_t buffer[6];
  ad.CopyTo (buffer);
  m_loader->NotifyStartExecute (); // Restore the memory of the kernel before access it !
  m_exported->dev_set_address (dev, buffer);
  m_exported->dev_set_mtu (dev, device->GetMtu ());
  m_loader->NotifyEndExecute ();
}

void
KernelSocketFdFactory::ScheduleTaskTrampoline (void *context)
{
  Task *current = TaskManager::Current ()->CurrentTask ();
  KernelSocketFdFactory *self = (KernelSocketFdFactory *) current->GetExtraContext ();
  current->SetExtraContext (0);
  EventImpl *event = (EventImpl *)context;
  event->Invoke ();
  event->Unref ();
  self->m_kernelTasks.remove (current);
  TaskManager::Current ()->Exit ();
}

void
KernelSocketFdFactory::ScheduleTask (EventImpl *event)
{
  Task *task = m_manager->Start (&KernelSocketFdFactory::ScheduleTaskTrampoline,
                                 event, 1 << 17);
  task->SetExtraContext (this);
  task->SetSwitchNotifier (&KernelSocketFdFactory::TaskSwitch, m_loader);
  m_kernelTasks.push_back (task);
}

void
KernelSocketFdFactory::NotifyAddDevice (Ptr<NetDevice> device)
{
  ScheduleTask (MakeEvent (&KernelSocketFdFactory::NotifyAddDeviceTask, this, device));
}
void
KernelSocketFdFactory::NotifyAddDeviceTask (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (device);
  int flags = 0;
  //NS_ASSERT (!device->IsPointToPoint ());
  //NS_ASSERT (device->NeedsArp ());
  //NS_ASSERT (device->IsMulticast ());
  //NS_ASSERT (device->IsBroadcast ());
  if (device->IsMulticast ())
    {
      flags |= SIM_DEV_MULTICAST;
    }
  if (device->IsBroadcast ())
    {
      flags |= SIM_DEV_BROADCAST;
    }
  if (!device->NeedsArp ())
    {
      flags |= SIM_DEV_NOARP;
    }
  m_loader->NotifyStartExecute (); // Restore the memory of the kernel before access it !
#if ((LIBOS_API_VERSION == 2))
  struct SimDevice *dev = m_exported->dev_create ("sim%d", PeekPointer (device), (enum SimDevFlags)flags);
#else
  struct SimDevice *dev = m_exported->dev_create (PeekPointer (device), (enum SimDevFlags)flags);
#endif  // LIBOS_API_VERSION
  m_loader->NotifyEndExecute ();
  Ptr<KernelDeviceStateListener> listener = Create <KernelDeviceStateListener> (device, this);
  m_listeners.push_back (listener);
  device->AddLinkChangeCallback (MakeCallback (&KernelDeviceStateListener::NotifyDeviceStateChange, listener));

  m_devices.push_back (std::make_pair (device,dev));
  Ptr<Node> node = GetObject<Node> ();
  if (device->GetInstanceTypeId () == m_lteUeTid)
    {
      node->RegisterProtocolHandler (MakeCallback (&KernelSocketFdFactory::RxFromDevice, this),
                                     0, device, false);
    }
  else
    {
      node->RegisterProtocolHandler (MakeCallback (&KernelSocketFdFactory::RxFromDevice, this),
                                     0, device, true);
    }
  NotifyDeviceStateChangeTask (device);
}


void
KernelSocketFdFactory::InitializeStack (void)
{
  std::string filePath = SearchExecFile ("DCE_PATH", m_library, 0);
  if (filePath.length () <= 0)
    {
      std::string line = "Stack file '";
      line += m_library;
      line += "' not found ! Please check your DCE_PATH environment variable.";
      NS_ASSERT_MSG (filePath.length () > 0, line.c_str ());
      return ;
    }
  NS_LOG_INFO ("loading " + filePath);
  void *handle = m_loader->Load (filePath, RTLD_LOCAL);
  void *symbol = m_loader->Lookup (handle, "sim_init");
  SimInit init = (SimInit) symbol;
  if (init == 0)
    {
      NS_FATAL_ERROR ("Oops. Can't find initialization function");
    }
  m_exported = new struct SimExported ();
  struct SimImported imported;
  imported.vprintf = &KernelSocketFdFactory::Vprintf;
  imported.malloc = &KernelSocketFdFactory::Malloc;
  imported.free = &KernelSocketFdFactory::Free;
  imported.memcpy = &KernelSocketFdFactory::Memcpy;
  imported.memset = &KernelSocketFdFactory::Memset;
  imported.atexit = &KernelSocketFdFactory::AtExit;
  imported.access = &KernelSocketFdFactory::Access;
  imported.getenv = &KernelSocketFdFactory::Getenv;
  imported.mkdir = &KernelSocketFdFactory::Mkdir;
  imported.open = &KernelSocketFdFactory::Open;
  imported.__fxstat = &KernelSocketFdFactory::__Fxstat;
  imported.fseek = &KernelSocketFdFactory::Fseek;
  imported.setbuf = &KernelSocketFdFactory::Setbuf;
  imported.ftell = &KernelSocketFdFactory::Ftell;
  imported.fdopen = &KernelSocketFdFactory::FdOpen;
  imported.fread = &KernelSocketFdFactory::Fread;
  imported.fwrite = &KernelSocketFdFactory::Fwrite;
  imported.fclose = &KernelSocketFdFactory::Fclose;
  imported.random = &KernelSocketFdFactory::Random;
  imported.event_schedule_ns = &KernelSocketFdFactory::EventScheduleNs;
  imported.event_cancel = &KernelSocketFdFactory::EventCancel;
  imported.current_ns = &CurrentNs;
  imported.task_start = &KernelSocketFdFactory::TaskStart;
  imported.task_wait = &KernelSocketFdFactory::TaskWait;
  imported.task_current = &KernelSocketFdFactory::TaskCurrent;
  imported.task_wakeup = &KernelSocketFdFactory::TaskWakeup;
  imported.task_yield = &KernelSocketFdFactory::TaskYield;
  imported.dev_xmit = &KernelSocketFdFactory::DevXmit;
  imported.signal_raised = &KernelSocketFdFactory::SignalRaised;
  imported.poll_event = &KernelSocketFdFactory::PollEvent;
  // create internal process
  Ptr<DceManager> manager = this->GetObject<DceManager> ();
  m_pid = manager->StartInternalTask ();

  init (m_exported, &imported, (struct SimKernel *)this);

  // update the kernel device list with simulation device list
  Ptr<Node> node = GetObject<Node> ();
  node->RegisterDeviceAdditionListener (MakeCallback (&KernelSocketFdFactory::NotifyAddDevice,
                                                      this));
  NS_LOG_FUNCTION (this << "m_exported " << m_exported);
}

UnixFd *
KernelSocketFdFactory::CreateSocket (int domain, int type, int protocol)
{
  GET_CURRENT (this << domain << type << protocol);
  struct SimSocket *socket;
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_socket (domain, type, protocol, &socket);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      return 0;
    }
  UnixFd *fd = new KernelSocketFd (this, socket);
  return fd;
}

int
KernelSocketFdFactory::Close (struct SimSocket *socket)
{
  GET_CURRENT (socket);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_close (socket);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
ssize_t
KernelSocketFdFactory::Recvmsg (struct SimSocket *socket, struct msghdr *msg, int flags)
{
  GET_CURRENT (socket << msg << flags);
  m_loader->NotifyStartExecute ();
  ssize_t retval = m_exported->sock_recvmsg (socket, msg, flags);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
ssize_t
KernelSocketFdFactory::Sendmsg (struct SimSocket *socket, const struct msghdr *msg, int flags)
{
  GET_CURRENT (socket << msg << flags);
  m_loader->NotifyStartExecute ();
  ssize_t retval = m_exported->sock_sendmsg (socket, msg, flags);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Getsockname (struct SimSocket *socket, struct sockaddr *name, socklen_t *namelen)
{
  GET_CURRENT (socket << name << namelen);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_getsockname (socket, name, (int*)namelen);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Getpeername (struct SimSocket *socket, struct sockaddr *name, socklen_t *namelen)
{
  GET_CURRENT (socket << name << namelen);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_getpeername (socket, name, (int*)namelen);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Bind (struct SimSocket *socket, const struct sockaddr *my_addr, socklen_t addrlen)
{
  GET_CURRENT (socket << my_addr << addrlen);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_bind (socket, my_addr, addrlen);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Connect (struct SimSocket *socket, const struct sockaddr *my_addr,
                               socklen_t addrlen, int flags)
{
  GET_CURRENT (socket << my_addr << addrlen << flags);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_connect (socket, my_addr, addrlen, flags);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Listen (struct SimSocket *socket, int backlog)
{
  GET_CURRENT (socket << backlog);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_listen (socket, backlog);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Shutdown (struct SimSocket *socket, int how)
{
  GET_CURRENT (socket << how);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_shutdown (socket, how);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Accept (struct SimSocket *socket, struct sockaddr *my_addr, socklen_t *addrlen, int flags)
{
  GET_CURRENT (socket << my_addr << addrlen << flags);
  struct SimSocket *newSocket;
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_accept (socket, &newSocket, flags);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  if (my_addr != 0)
    {
      m_loader->NotifyStartExecute ();
      retval = m_exported->sock_getpeername (newSocket, my_addr, (int*)addrlen);
      if (retval < 0)
        {
          current->err = -retval;
          m_exported->sock_close (newSocket);
          m_loader->NotifyEndExecute ();
          return -1;
        }
      m_loader->NotifyEndExecute ();
    }
  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      m_loader->NotifyStartExecute ();
      m_exported->sock_close (newSocket);
      current->err = EMFILE;
      m_loader->NotifyEndExecute ();
      return -1;
    }

  UnixFd *unixFd = new KernelSocketFd (this, newSocket);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);

  return fd;
}
int
KernelSocketFdFactory::Ioctl (struct SimSocket *socket, unsigned long request, char *argp)
{
  GET_CURRENT (socket << request << argp);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_ioctl (socket, request, argp);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Setsockopt (struct SimSocket *socket, int level, int optname,
                                  const void *optval, socklen_t optlen)
{
  GET_CURRENT (socket << level << optname << optval << optlen);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_setsockopt (socket, level, optname, optval, optlen);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int
KernelSocketFdFactory::Getsockopt (struct SimSocket *socket, int level, int optname,
                                  void *optval, socklen_t *optlen)
{
  GET_CURRENT (socket << level << optname << optval << optlen);
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_getsockopt (socket, level, optname, optval, (int*)optlen);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
void
KernelSocketFdFactory::PollEvent (int flag, void *context)
{
  PollTable* ptable = (PollTable*)context;
  ptable->WakeUpCallback ();
}

/**
 * Struct used to pass pool table context between DCE and Kernel and back from Kernel to DCE
 *
 * When calling sock_poll we provide in ret field the wanted eventmask, and in the opaque field
 * the DCE poll table
 *
 * if a corresponding event occurs later, the PollEvent will be called by kernel with the DCE
 * poll table in context variable, then we will able to wake up the thread blocked in poll call.
 *
 * Back from sock_poll method the kernel change ret field with the response from poll return of the
 * corresponding kernel socket, and in opaque field there is a reference to the kernel poll table
 * we will use this reference to remove us from the file wait queue when ending the DCE poll call or
 * when ending the DCE process which is currently polling.
 *
 */
struct poll_table_ref
{
  int ret;
  void *opaque;
};
int
KernelSocketFdFactory::Poll (struct SimSocket *socket, PollTable* ptable)
{
  struct poll_table_ref kernelInOut =
  {
    0
  };
  if (ptable)
    {
      // Fill Opaque and ptable.
      kernelInOut.opaque = ptable;
      kernelInOut.ret = ptable->GetEventMask ();
    }

  GET_CURRENT (socket);
  m_loader->NotifyStartExecute ();
  m_exported->sock_poll (socket, &kernelInOut);
  m_loader->NotifyEndExecute ();

  if (ptable)
    {
      ptable->PollWait (kernelInOut.opaque, MakeCallback (&KernelSocketFdFactory::PollFreeWait, this));
    }

  return kernelInOut.ret;
}
void
KernelSocketFdFactory::PollFreeWait (void *ref)
{
  m_loader->NotifyStartExecute ();
  m_exported->sock_pollfreewait (ref);
  m_loader->NotifyEndExecute ();
}
} // namespace ns3
