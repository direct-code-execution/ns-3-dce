#ifndef KERNEL_SOCKET_FD_FACTORY_H
#define KERNEL_SOCKET_FD_FACTORY_H

#include "socket-fd-factory.h"
#include "task-manager.h"
#include "ns3/net-device.h"
#include "ns3/random-variable-stream.h"
#include <sys/socket.h>
#include <vector>
#include <string>
#include <utility>
#include <stdarg.h>
#include <stdio.h>
#include "linux/types.h"

extern "C" {
struct DceHandle;
struct KernelHandle;
struct DceSocket;
struct SimTask;
struct DceKernel;
struct SimSysFile;
struct SimDevice;
}

class KingsleyAlloc;

namespace ns3 {

class Loader;
class NetDevice;
class Task;
class TaskManager;
class Packet;
class KernelDeviceStateListener;
class PollTable;

class KernelSocketFdFactory : public SocketFdFactory
{
public:
  static TypeId GetTypeId (void);
  KernelSocketFdFactory ();
  virtual ~KernelSocketFdFactory ();

  virtual UnixFd * CreateSocket (int domain, int type, int protocol);

  void ScheduleTask (EventImpl *event);
  std::string m_library;

protected:
  void InitializeStack (void);
  struct KernelHandle *m_kernelHandle;
  Ptr<TaskManager> m_manager;
  Loader *m_loader;
  FILE *m_logFile;

private:
  friend class KernelSocketFd;
  friend class KernelDeviceStateListener;
  struct EventIdHolder : public SimpleRefCount<EventIdHolder>
  {
    EventId id;
  };

  // called from KernelSocketFd
  int Close (struct DceSocket *socket);
  ssize_t Recvmsg (struct DceSocket *socket, struct msghdr *msg, int flags);
  ssize_t Sendmsg (struct DceSocket *socket, const struct msghdr *msg, int flags);
  int Getsockname (struct DceSocket *socket, struct sockaddr *name, socklen_t *namelen);
  int Getpeername (struct DceSocket *socket, struct sockaddr *name, socklen_t *namelen);
  int Bind (struct DceSocket *socket, const struct sockaddr *my_addr, socklen_t addrlen);
  int Connect (struct DceSocket *socket, const struct sockaddr *my_addr, socklen_t addrlen, int flags);
  int Listen (struct DceSocket *socket, int backlog);
  int Shutdown (struct DceSocket *socket, int how);
  int Accept (struct DceSocket *socket, struct sockaddr *my_addr, socklen_t *addrlen, int flags);
  int Ioctl (struct DceSocket *socket, unsigned long request, char *argp);
  int Setsockopt (struct DceSocket *socket, int level, int optname,
                  const void *optval, socklen_t optlen);
  int Getsockopt (struct DceSocket *socket, int level, int optname,
                  void *optval, socklen_t *optlen);
  bool CanRecv (struct DceSocket *socket);
  bool CanSend (struct DceSocket *socket);
  void* PollWait (struct DceSocket *socket, void *ctxt);
  void  FreePoll (struct DceSocket *socket, void *ctxt);
  int Poll (struct DceSocket *socket, PollTable* ptable);
  void PollFreeWait (void *ref);

  // called from kernel.
  static int Vprintf (struct DceKernel *kernel, const char *str, va_list args);
  static void * Malloc (struct DceKernel *kernel, unsigned long size);
  static void Free (struct DceKernel *kernel, void *buffer);
  static void * Memcpy (struct DceKernel *kernel, void *dst, const void *src, unsigned long size);
  static void * Memset (struct DceKernel *kernel, void *dst, char value, unsigned long size);
  static int AtExit (struct DceKernel *kernel, void (*function)(void));
  static void Panic (struct DceKernel *kernel);
  static int Access (struct DceKernel *kernel, const char *pathname, int mode);
  static char* Getenv (struct DceKernel *kernel, const char *name);
  static int Mkdir (struct DceKernel *kernel, const char *pathname, mode_t mode);
  static int Open (struct DceKernel *kernel, const char *pathname, int flags);
  static size_t Fread (struct DceKernel *kernel, void *ptr, size_t size, size_t nmemb, FILE *stream);
  static size_t Fwrite (struct DceKernel *kernel, const void *ptr, size_t size, size_t nmemb, FILE *stream);
  static FILE* FdOpen (struct DceKernel *kernel, int fd, const char *mode);
  static int __Fxstat (struct DceKernel *kernel, int ver, int fd, void *buf);
  static int Fseek (struct DceKernel *kernel, FILE *stream, long offset, int whence);
  static void Setbuf (struct DceKernel *kernel, FILE *stream, char *buf);
  static long Ftell (struct DceKernel *kernel, FILE *stream);
  static int Fclose (struct DceKernel *kernel, FILE *fp);
  static unsigned long Random (struct DceKernel *kernel);
  static void *EventScheduleNs (struct DceKernel *kernel, __u64 ns, void (*fn)(void *context), void *context,
                                void (*pre_fn)(void));
  static void EventCancel (struct DceKernel *kernel, void *ev);
  static struct SimTask *TaskStart (struct DceKernel *kernel, void (*callback)(void *), void *context);
  static struct SimTask * TaskCurrent (struct DceKernel *kernel);
  static void TaskWait (struct DceKernel *kernel);
  static int TaskWakeup (struct DceKernel *kernel, struct SimTask *task);
  static void TaskYield (struct DceKernel *kernel);
  static void DevXmit (struct DceKernel *kernel, struct SimDevice *dev, unsigned char *data, int len);
  static void SignalRaised (struct DceKernel *kernel, struct SimTask *task, int signalNumber);
  static void PollEvent (int flag, void *context);


  // inherited from Object.
  virtual void DoDispose (void);
  // called from Node
  void NotifyAddDevice (Ptr<NetDevice> device);
  // called during initialization with a task context
  // to enter the kernel functions.
  void StartInitializationTask (void);
  void RxFromDevice (Ptr<NetDevice> device, Ptr<const Packet> p,
                     uint16_t protocol, const Address & from,
                     const Address &to, NetDevice::PacketType type);
  struct SimDevice * DevToDev (Ptr<NetDevice> dev);
  void NotifyDeviceStateChange (Ptr<NetDevice> device);
  void NotifyDeviceStateChangeTask (Ptr<NetDevice> device);
  void NotifyAddDeviceTask (Ptr<NetDevice> device);

  void DoSet (std::string path, std::string value);
  static void TaskSwitch (enum Task::SwitchType type, void *context);
  static void ScheduleTaskTrampoline (void *context);
  void EventTrampoline (void (*fn)(void *context),
                        void *context, void (*pre_fn)(void),
                        Ptr<EventIdHolder> event);
  static void SendMain (bool *r, NetDevice *d, Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);

  std::vector<std::pair<Ptr<NetDevice>,struct SimDevice *> > m_devices;
  std::list<Task *> m_kernelTasks;
  Ptr<UniformRandomVariable> m_variable;
  KingsleyAlloc *m_alloc;
  std::vector<Ptr<KernelDeviceStateListener> > m_listeners;
  double m_rate;
  Ptr<RandomVariableStream> m_ranvar;
  uint16_t m_pid;
  TypeId m_lteUeTid;
};

} // namespace ns3

#endif /* KERNEL_SOCKET_FD_FACTORY_H */
