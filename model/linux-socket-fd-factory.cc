#include "linux-socket-fd-factory.h"
#include "linux-socket-fd.h"
#include "loader-factory.h"
#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include "wait-queue.h"
#include "task-manager.h"
#include "kingsley-alloc.h"
#include "file-usage.h"
#include "sim/include/sim-init.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/random-variable.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/packet.h"
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

NS_LOG_COMPONENT_DEFINE("LinuxSocketFdFactory");

namespace ns3 {

// Sadly NetDevice Callback add by method AddLinkChangeCallback take no parameters ..
// .. so we need to create the following class to link NetDevice and LinuxSocketFdFactory together
// in order to do Warn the factory about which NetDevice is changing .
class LinuxDeviceStateListener : public SimpleRefCount<LinuxDeviceStateListener>
{
public:
  LinuxDeviceStateListener(Ptr<NetDevice>, Ptr<LinuxSocketFdFactory>);

  void NotifyDeviceStateChange ();

private:
  Ptr<NetDevice> m_netDevice;
  Ptr<LinuxSocketFdFactory> m_factory;
};

LinuxDeviceStateListener::LinuxDeviceStateListener(Ptr<NetDevice> d,
    Ptr<LinuxSocketFdFactory> f):
    m_netDevice(d), m_factory(f)
{
}

void
LinuxDeviceStateListener::NotifyDeviceStateChange ()
{
  m_factory->NotifyDeviceStateChange (m_netDevice);
}

NS_OBJECT_ENSURE_REGISTERED(LinuxSocketFdFactory);

TypeId 
LinuxSocketFdFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinuxSocketFdFactory")
    .SetParent<SocketFdFactory> ()
    .AddConstructor<LinuxSocketFdFactory> ()
    .AddAttribute ("Library", "File to load in memory",
		   StringValue("liblinux.so"),
		   MakeStringAccessor (&LinuxSocketFdFactory::m_library),
		   MakeStringChecker ())
    ;
  return tid;
}
LinuxSocketFdFactory::LinuxSocketFdFactory ()
  : m_loader (0),
    m_exported (0),
    m_alloc (new KingsleyAlloc ()),
    m_logFile (0)
{}

LinuxSocketFdFactory::~LinuxSocketFdFactory ()
{
  for (uint32_t i = 0; i < m_devices.size(); i++)
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
LinuxSocketFdFactory::DoDispose (void)
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
LinuxSocketFdFactory::Vprintf (struct SimKernel *kernel, const char *str, va_list args)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
  return vfprintf (self->m_logFile, str, args);
}

void *
LinuxSocketFdFactory::Malloc (struct SimKernel *kernel, unsigned long size)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
  size += sizeof (size_t);
  uint8_t *buffer = self->m_alloc->Malloc (size);
  memcpy (buffer, &size, sizeof (size_t));
  buffer += sizeof (size_t);
  return buffer;
}
void 
LinuxSocketFdFactory::Free (struct SimKernel *kernel, void *ptr)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
  uint8_t *buffer = (uint8_t*)ptr;
  size_t size;
  buffer -= sizeof (size_t);
  memcpy (&size, buffer, sizeof (size_t));
  self->m_alloc->Free (buffer, size);
}
void *
LinuxSocketFdFactory::Memcpy (struct SimKernel *kernel, void *dst, const void *src, unsigned long size)
{
  return memcpy (dst, src, size);
}
void *
LinuxSocketFdFactory::Memset (struct SimKernel *kernel, void *dst, char value, unsigned long size)
{
  return memset (dst, value, size);
}
unsigned long 
LinuxSocketFdFactory::Random (struct SimKernel *kernel)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
  union {
    uint8_t buffer[sizeof(unsigned long)];
    unsigned long v;
  } u;
  for (uint8_t i = 0; i < sizeof(u.buffer); i++)
    {
      u.buffer[i] = self->m_variable.GetInteger(0,255);
    }
  return u.v;
}
void 
LinuxSocketFdFactory::EventTrampoline (void (*fn) (void *context), 
				       void *context, void (*pre_fn) (void),
				       Ptr<EventIdHolder> event)
{
  m_loader->NotifyStartExecute ();
  pre_fn ();
  fn (context);
  m_loader->NotifyEndExecute ();
}
void *
LinuxSocketFdFactory::EventScheduleNs (struct SimKernel *kernel, unsigned long ns, void (*fn) (void *context), void *context,
				       void (*pre_fn) (void))
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
  Ptr<EventIdHolder> ev = Create<EventIdHolder> ();
  EventId event = Simulator::Schedule (NanoSeconds(ns), 
				       &LinuxSocketFdFactory::EventTrampoline, 
				       self, fn, context, pre_fn, ev);
  ev->id = event;
  return &ev->id;
}
void 
LinuxSocketFdFactory::EventCancel (struct SimKernel *kernel, void *ev)
{
  EventId *event = (EventId *)ev;
  Simulator::Remove(*event);
}
static __u64 CurrentNs (struct SimKernel *kernel)
{
  return Simulator::Now ().GetNanoSeconds ();
}

void 
LinuxSocketFdFactory::TaskSwitch (enum Task::SwitchType type, void *context)
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
LinuxSocketFdFactory::TaskStart (struct SimKernel *kernel, void (*callback) (void *), void *context)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
  Task *task = self->m_manager->Start (callback, context, 1<<17);
  struct SimTask *simTask = self->m_exported->task_create(task, 0);
  task->SetExtraContext (simTask);
  task->SetSwitchNotifier (&LinuxSocketFdFactory::TaskSwitch, self->m_loader);
  self->m_kernelTasks.push_back (task);
  return (struct SimTask *)task->GetExtraContext ();
}
struct SimTask *
LinuxSocketFdFactory::TaskCurrent (struct SimKernel *kernel)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
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
      struct SimTask *simTask = self->m_exported->task_create(current, pid);
      current->SetExtraContext (simTask);
    }
  return (struct SimTask *)current->GetExtraContext ();
}
void 
LinuxSocketFdFactory::TaskWait (struct SimKernel *kernel)
{
  // force initialization of 'current'
  TaskCurrent (kernel);
  // now, sleep.
  TaskManager::Current ()->Sleep ();
}
int 
LinuxSocketFdFactory::TaskWakeup (struct SimKernel *kernel, struct SimTask *task)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;  
  TaskManager *manager = TaskManager::Current ();
  Task *other = (Task *)self->m_exported->task_get_private(task);
  bool isBlocked = other->IsBlocked ();
  manager->Wakeup (other);
  return isBlocked?1:0;
}
void 
LinuxSocketFdFactory::TaskYield (struct SimKernel *kernel)
{
  // force initialization of 'current'
  TaskCurrent (kernel);
  // now, yield.
  TaskManager::Current ()->Yield ();
}
void 
LinuxSocketFdFactory::DevXmit (struct SimKernel *kernel, struct SimDevice *dev, unsigned char *data, int len)
{
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *)kernel;
  NetDevice *nsDev = (NetDevice *)self->m_exported->dev_get_private (dev);
  NS_ASSERT (len >= 14);

  struct ethhdr {
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
  nsDev->Send (p, dest, protocol);
}

void
LinuxSocketFdFactory::SignalRaised ( struct SimKernel *kernel, struct SimTask *task , int signalNumber)
{
    NS_LOG_FUNCTION ("XXX: Not Yet Implemented "<<signalNumber);
}

struct SimDevice *
LinuxSocketFdFactory::DevToDev (Ptr<NetDevice> device)
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
LinuxSocketFdFactory::RxFromDevice (Ptr<NetDevice> device, Ptr<const Packet> p,
				    uint16_t protocol, const Address & from,
				    const Address &to, NetDevice::PacketType type)
{
  struct SimDevice *dev = DevToDev (device);
  if (dev == 0)
    {
      return;
    }

  struct SimDevicePacket packet = m_exported->dev_create_packet (dev, p->GetSize () + 14);
  p->CopyData (((unsigned char *)packet.buffer) + 14, p->GetSize ());
  struct ethhdr {
    unsigned char   h_dest[6];
    unsigned char   h_source[6];
    uint16_t        h_proto;
  } *hdr = (struct ethhdr *)packet.buffer;
  Mac48Address realFrom = Mac48Address::ConvertFrom (from);
  Mac48Address realTo = Mac48Address::ConvertFrom (to);
  realFrom.CopyTo (hdr->h_source);
  realTo.CopyTo (hdr->h_dest);
  hdr->h_proto = ntohs (protocol);
  m_loader->NotifyStartExecute ();
  m_exported->dev_rx (dev, packet);
  m_loader->NotifyEndExecute ();
}

void
LinuxSocketFdFactory::NotifyDeviceStateChange (Ptr<NetDevice> device)
{
  ScheduleTask (MakeEvent (&LinuxSocketFdFactory::NotifyDeviceStateChangeTask, 
			   this, device));
}
void
LinuxSocketFdFactory::NotifyDeviceStateChangeTask (Ptr<NetDevice> device)
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
  m_exported->dev_set_address(dev, buffer);
  m_exported->dev_set_mtu (dev, device->GetMtu ());
}

void
LinuxSocketFdFactory::ScheduleTaskTrampoline (void *context)
{
  Task *current = TaskManager::Current ()->CurrentTask ();
  LinuxSocketFdFactory *self = (LinuxSocketFdFactory *) current->GetExtraContext ();
  current->SetExtraContext (0);
  EventImpl *event = (EventImpl *)context;
  event->Invoke ();
  event->Unref ();
  self->m_kernelTasks.remove (current);
  TaskManager::Current ()->Exit ();
}

void
LinuxSocketFdFactory::ScheduleTask (EventImpl *event)
{
  Task *task = m_manager->Start (&LinuxSocketFdFactory::ScheduleTaskTrampoline, 
				 event, 1<<17);
  task->SetExtraContext (this);
  task->SetSwitchNotifier (&LinuxSocketFdFactory::TaskSwitch, m_loader);
  m_kernelTasks.push_back (task);
}

void
LinuxSocketFdFactory::NotifyAddDevice (Ptr<NetDevice> device)
{
  ScheduleTask (MakeEvent (&LinuxSocketFdFactory::NotifyAddDeviceTask, this, device));
}
void
LinuxSocketFdFactory::NotifyAddDeviceTask (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (device);
  int flags = 0;
  //NS_ASSERT (!device->IsPointToPoint ());
  //NS_ASSERT (device->NeedsArp ());
  NS_ASSERT (device->IsMulticast ());
  NS_ASSERT (device->IsBroadcast ());
  if (device->IsMulticast ())
    {
      flags |= SIM_DEV_MULTICAST;
    }
  if (device->IsBroadcast ())
    {
      flags |= SIM_DEV_BROADCAST;
    }
  struct SimDevice *dev = m_exported->dev_create(PeekPointer(device), (enum SimDevFlags)flags);

  Ptr<LinuxDeviceStateListener> listener = Create <LinuxDeviceStateListener> (device, this);
  m_listeners.push_back (listener);
  device->AddLinkChangeCallback (MakeCallback (&LinuxDeviceStateListener::NotifyDeviceStateChange, listener));

  m_devices.push_back (std::make_pair(device,dev));
  Ptr<Node> node = GetObject<Node> ();
  node->RegisterProtocolHandler (MakeCallback (&LinuxSocketFdFactory::RxFromDevice, this), 
				 0, device, true);
  NotifyDeviceStateChangeTask (device);
}

std::vector<std::pair<std::string,struct SimSysFile *> >
LinuxSocketFdFactory::GetSysFileList (void)
{
  struct MyIterator 
  {
    struct SimSysIterator head;
    static void ReportStartDir (const struct SimSysIterator *iter, const char *dirname)
    {
      struct MyIterator *self = (struct MyIterator *)iter;
      self->m_stack.push_back (self->m_currentPath);
      self->m_currentPath += "." + std::string (dirname);
    }
    static void ReportEndDir (const struct SimSysIterator *iter)
    {
      struct MyIterator *self = (struct MyIterator *)iter;
      self->m_currentPath = self->m_stack.back ();
      self->m_stack.pop_back ();
    }
    static void ReportFile (const struct SimSysIterator *iter, const char *filename, int flags, struct SimSysFile *file)
    {
      struct MyIterator *self = (struct MyIterator *)iter;
      std::string path = self->m_currentPath + "." + filename;
      self->m_list.push_back (std::make_pair (path, file));
    }
    std::vector<std::string> m_stack;
    std::vector<std::pair<std::string,struct SimSysFile *> > m_list;
    std::string m_currentPath;
  } iter;
  iter.head.report_start_dir = &MyIterator::ReportStartDir;
  iter.head.report_end_dir = &MyIterator::ReportEndDir;
  iter.head.report_file = &MyIterator::ReportFile;
  m_loader->NotifyStartExecute ();
  m_exported->sys_iterate_files ((struct SimSysIterator *)&iter);
  m_loader->NotifyEndExecute ();
  return iter.m_list;
}

void
LinuxSocketFdFactory::SetTask (std::string path, std::string value)
{
  std::vector<std::pair<std::string,struct SimSysFile *> > files = GetSysFileList ();
  for (uint32_t i = 0; i < files.size (); i++)
    {
      if (files[i].first == path)
	{
	  const char *s = value.c_str ();
	  int toWrite = value.size ();
	  int written;
	  written = m_exported->sys_file_write (files[i].second, s, toWrite, 0);
	  break;
	}
    }
}

void 
LinuxSocketFdFactory::Set (std::string path, std::string value)
{
  if (m_manager == 0)
    {
      m_earlySysfs.push_back (std::make_pair (path,value));
    }
  else
    {
      ScheduleTask (MakeEvent (&LinuxSocketFdFactory::SetTask, this, path, value));
    }
}

void
LinuxSocketFdFactory::InitializeStack (void)
{  
  void *handle = m_loader->Load (m_library, RTLD_LOCAL);
  void *symbol = m_loader->Lookup (handle, "sim_init");
  SimInit init = (SimInit) symbol;
  if (init == 0)
    {
      NS_FATAL_ERROR ("Oops. Can't find initialization function");
    }
  m_exported = new struct SimExported ();
  struct SimImported imported;
  imported.vprintf = &LinuxSocketFdFactory::Vprintf;
  imported.malloc = &LinuxSocketFdFactory::Malloc;
  imported.free = &LinuxSocketFdFactory::Free;
  imported.memcpy = &LinuxSocketFdFactory::Memcpy;
  imported.memset = &LinuxSocketFdFactory::Memset;
  imported.random = &LinuxSocketFdFactory::Random;
  imported.event_schedule_ns = &LinuxSocketFdFactory::EventScheduleNs;
  imported.event_cancel = &LinuxSocketFdFactory::EventCancel;
  imported.current_ns = &CurrentNs;
  imported.task_start = &LinuxSocketFdFactory::TaskStart;
  imported.task_wait = &LinuxSocketFdFactory::TaskWait;
  imported.task_current = &LinuxSocketFdFactory::TaskCurrent;
  imported.task_wakeup = &LinuxSocketFdFactory::TaskWakeup;
  imported.task_yield = &LinuxSocketFdFactory::TaskYield;
  imported.dev_xmit = &LinuxSocketFdFactory::DevXmit;
  imported.signal_raised = &LinuxSocketFdFactory::SignalRaised;
  imported.poll_event = &LinuxSocketFdFactory::PollEvent;
  init (m_exported, &imported, (struct SimKernel *)this);

  // update the linux device list with simulation device list
  Ptr<Node> node = GetObject<Node> ();
  node->RegisterDeviceAdditionListener (MakeCallback (&LinuxSocketFdFactory::NotifyAddDevice,
						      this));
  Set (".net.ipv4.conf.all.forwarding", "1");
  Set (".net.ipv4.conf.all.log_martians", "1");

  while (!m_earlySysfs.empty ())
    {
      std::pair<std::string,std::string> op = m_earlySysfs.front ();
      Set (op.first, op.second);
      m_earlySysfs.pop_front ();
    }
}

void 
LinuxSocketFdFactory::NotifyNewAggregate (void)
{
  Ptr<Node> node = this->GetObject<Node> ();
  Ptr<LoaderFactory> loaderFactory = this->GetObject<LoaderFactory> ();
  Ptr<TaskManager> taskManager = this->GetObject<TaskManager> ();
  if (node != 0 && loaderFactory != 0 && taskManager != 0 && m_loader == 0)
    {
      m_manager = taskManager;
      m_loader = loaderFactory->Create (0, 0, 0);
      UtilsEnsureDirectoryExists (UtilsGetAbsRealFilePath (node->GetId (), "/var"));
      UtilsEnsureDirectoryExists (UtilsGetAbsRealFilePath (node->GetId (), "/var/log"));
      std::string path = UtilsGetAbsRealFilePath (node->GetId (), "/var/log/messages");
      m_logFile = fopen (path.c_str (), "w");
      setlinebuf(m_logFile);
      // must use ScheduleWithContext to ensure that the initialization task gets
      // a node context to be able to retrieve the task manager when it runs.
      // i.e., TaskManager::Current() needs it.
      Simulator::ScheduleWithContext (node->GetId (), Seconds (0.0),
				      &LinuxSocketFdFactory::ScheduleTask, this,
				      MakeEvent (&LinuxSocketFdFactory::InitializeStack, this));
    }
}

UnixFd *
LinuxSocketFdFactory::CreateSocket (int domain, int type, int protocol)
{
  GET_CURRENT(this << domain << type << protocol);
  struct SimSocket *socket;
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_socket(domain, type, protocol, &socket);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      return 0;
    }
  UnixFd *fd = new LinuxSocketFd (this, socket);
  return fd;
}

int 
LinuxSocketFdFactory::Close (struct SimSocket *socket)
{
  GET_CURRENT(socket);
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
LinuxSocketFdFactory::Recvmsg(struct SimSocket *socket, struct msghdr *msg, int flags)
{
  GET_CURRENT(socket << msg << flags);
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
LinuxSocketFdFactory::Sendmsg(struct SimSocket *socket, const struct msghdr *msg, int flags)
{
  GET_CURRENT(socket << msg << flags);
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
LinuxSocketFdFactory::Getsockname(struct SimSocket *socket, struct sockaddr *name, socklen_t *namelen)
{
  GET_CURRENT(socket << name << namelen);
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
LinuxSocketFdFactory::Getpeername(struct SimSocket *socket, struct sockaddr *name, socklen_t *namelen)
{
  GET_CURRENT(socket << name << namelen);
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
LinuxSocketFdFactory::Bind (struct SimSocket *socket, const struct sockaddr *my_addr, socklen_t addrlen)
{
  GET_CURRENT(socket << my_addr << addrlen);
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
LinuxSocketFdFactory::Connect (struct SimSocket *socket, const struct sockaddr *my_addr, socklen_t addrlen)
{
  GET_CURRENT(socket << my_addr << addrlen);
  // XXX: handle O_NONBLOCK with flags.
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_connect (socket, my_addr, addrlen, 0);
  m_loader->NotifyEndExecute ();
  if (retval < 0)
    {
      current->err = -retval;
      return -1;
    }
  return retval;
}
int 
LinuxSocketFdFactory::Listen (struct SimSocket *socket, int backlog)
{
  GET_CURRENT(socket << backlog);
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
LinuxSocketFdFactory::Shutdown(struct SimSocket *socket, int how)
{
  GET_CURRENT(socket << how);
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
LinuxSocketFdFactory::Accept (struct SimSocket *socket, struct sockaddr *my_addr, socklen_t *addrlen)
{
  GET_CURRENT(socket << my_addr << addrlen);
  struct SimSocket *newSocket;
  // XXX: handle O_NONBLOCK
  m_loader->NotifyStartExecute ();
  int retval = m_exported->sock_accept (socket, &newSocket, 0);
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

  UnixFd *unixFd = new LinuxSocketFd (this, newSocket);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);

  return fd;
}
int 
LinuxSocketFdFactory::Ioctl (struct SimSocket *socket, int request, char *argp)
{
  GET_CURRENT(socket << request << argp);
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
LinuxSocketFdFactory::Setsockopt (struct SimSocket *socket, int level, int optname,
				  const void *optval, socklen_t optlen)
{
  GET_CURRENT(socket << level << optname << optval << optlen);
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
LinuxSocketFdFactory::Getsockopt (struct SimSocket *socket, int level, int optname,
				  void *optval, socklen_t *optlen)
{
  GET_CURRENT(socket << level << optname << optval << optlen);
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
LinuxSocketFdFactory::PollEvent ( int flag, void *context)
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
LinuxSocketFdFactory::Poll (struct SimSocket *socket, PollTable* ptable)
{
  struct poll_table_ref kernelInOut = { 0 };
  if (ptable)
    {
      // Fill Opaque and ptable.
      kernelInOut.opaque = ptable;
      kernelInOut.ret = ptable->GetEventMask ();
    }

  GET_CURRENT(socket);
  m_loader->NotifyStartExecute ();
  m_exported->sock_poll(socket, &kernelInOut);
  m_loader->NotifyEndExecute ();

  if (ptable)
    {
      ptable->PollWait (kernelInOut.opaque , MakeCallback( &LinuxSocketFdFactory::PollFreeWait, this ));
    }

  return kernelInOut.ret;
}
void
LinuxSocketFdFactory::PollFreeWait (void *ref )
{
  m_loader->NotifyStartExecute ();
  m_exported->sock_pollfreewait (ref);
  m_loader->NotifyEndExecute ();
}
} // namespace ns3
