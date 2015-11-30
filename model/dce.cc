#include "dce-manager.h"
#include "process.h"
#include "utils.h"
#include "exec-utils.h"
#include "dce-errno.h"
#include "dce-libc-private.h"
#include "dce-signal.h"
#include "dce-netdb.h"
#include "dce-unistd.h"
#include "dce-time.h"
#include "sys/dce-socket.h"
#include "dce-pthread.h"
#include "dce-stdio.h"
#include "dce-stdarg.h"
#include "dce-stdlib.h"
#include "dce-locale.h"
#include "sys/dce-ioctl.h"
#include "dce-sched.h"
#include "arpa/dce-inet.h"
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "dce-random.h"
#include "net/dce-if.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/names.h"
#include "ns3/ipv4-l3-protocol.h"
#include "socket-fd-factory.h"

NS_LOG_COMPONENT_DEFINE ("Dce");

using namespace ns3;

int * dce___h_errno_location (void)
{
  GET_CURRENT_NOLOG ();
  return &current->h_err;
}
int * dce___errno_location (void)
{
  GET_CURRENT_NOLOG ();
  return &current->err;
}
pid_t dce_getpid (void)
{
  GET_CURRENT_NOLOG ();
  return current->process->pid;
}
pid_t dce_getppid (void)
{
  GET_CURRENT_NOLOG ();
  return current->process->ppid;
}
uid_t dce_getuid (void)
{
  GET_CURRENT_NOLOG ();
  return current->process->ruid;
}
uid_t dce_geteuid (void)
{
  GET_CURRENT_NOLOG ();
  return current->process->euid;
}
static bool is_ucapable (void)
{
  GET_CURRENT_NOLOG ();
  return current->process->euid == 0;
}
static bool is_gcapable (void)
{
  GET_CURRENT_NOLOG ();
  return current->process->egid == 0;
}
static bool is_set_ucapable (uid_t uid)
{
  GET_CURRENT (uid);
  return is_ucapable ()
         || current->process->euid == uid
         || current->process->ruid == uid
         || current->process->suid == uid;
}
static bool is_set_gcapable (gid_t gid)
{
  GET_CURRENT (gid);
  return is_gcapable ()
         || current->process->egid == gid
         || current->process->rgid == gid
         || current->process->sgid == gid;
}

int dce_setresuid (uid_t ruid, uid_t euid, uid_t suid)
{
  GET_CURRENT (ruid << euid << suid);
  if (ruid != (uid_t)-1
      && !is_set_ucapable (ruid))
    {
      current->err = EPERM;
      return -1;
    }
  if (euid != (uid_t)-1
      && !is_set_ucapable (euid))
    {
      current->err = EPERM;
      return -1;
    }
  if (suid != (uid_t)-1
      && !is_set_ucapable (suid))
    {
      current->err = EPERM;
      return -1;
    }
  if (ruid != (uid_t)-1)
    {
      current->process->ruid = ruid;
    }
  if (euid != (uid_t)-1)
    {
      current->process->euid = euid;
    }
  if (suid != (uid_t)-1)
    {
      current->process->suid = suid;
    }

  return 0;
}
int dce_setresgid (gid_t rgid, gid_t egid, gid_t sgid)
{
  GET_CURRENT (rgid << egid << sgid);
  if (rgid != (gid_t)-1
      && !is_set_ucapable (rgid))
    {
      current->err = EPERM;
      return -1;
    }
  if (egid != (gid_t)-1
      && !is_set_ucapable (egid))
    {
      current->err = EPERM;
      return -1;
    }
  if (sgid != (gid_t)-1
      && !is_set_ucapable (sgid))
    {
      current->err = EPERM;
      return -1;
    }
  if (rgid != (gid_t)-1)
    {
      current->process->rgid = rgid;
    }
  if (egid != (gid_t)-1)
    {
      current->process->egid = egid;
    }
  if (sgid != (gid_t)-1)
    {
      current->process->sgid = sgid;
    }
  return 0;
}
int dce_setreuid (uid_t ruid, uid_t euid)
{
  GET_CURRENT (ruid << euid);
  return dce_setresuid (ruid,euid,-1);
}
int dce_setregid (gid_t rgid, gid_t egid)
{
  GET_CURRENT (rgid << egid);
  return dce_setresgid (rgid,egid,-1);
}

int dce_seteuid (uid_t euid)
{
  GET_CURRENT (euid);
  return dce_setresuid (-1, euid, -1);
}
int dce_setegid (gid_t egid)
{
  GET_CURRENT (egid);
  return dce_setresgid (-1, egid, -1);
}
int dce_setuid (uid_t uid)
{
  GET_CURRENT (uid);
  if (is_set_ucapable (uid))
    {
      current->process->ruid = uid;
      if (current->process->euid == 0)
        {
          current->process->euid = uid;
          current->process->suid = uid;
        }
      return 0;
    }
  else
    {
      current->err = EPERM;
      return -1;
    }
}
int dce_setgid (gid_t gid)
{
  GET_CURRENT (gid);
  if (is_set_gcapable (gid))
    {
      current->process->rgid = gid;
      if (current->process->egid == 0)
        {
          current->process->egid = gid;
          current->process->sgid = gid;
        }
      return 0;
    }
  else
    {
      current->err = EPERM;
      return -1;
    }
}

unsigned int dce_sleep (unsigned int seconds)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  current->process->manager->Wait (Seconds (seconds));
  return 0;
}
int dce_usleep (useconds_t usec)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  current->process->manager->Wait (MicroSeconds (usec));
  return 0;
}

int dce_kill (pid_t pid, int sig)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << pid << sig);
  Process *process = current->process->manager->SearchProcess (pid);

  if (process == 0)
    {
      current->err = ESRCH;
      return -1;
    }

  UtilsSendSignal (process, SIGKILL);

  return 0;
}

void dce_abort ()
{
  Thread *current = Current ();
  NS_LOG_WARN (current);

  UtilsSendSignal (Current ()->process, SIGABRT);
  // If we are still alive force the exitation
  dce_exit (-2);
}

void dce___assert_fail (const char *__assertion, const char *__file,
                        unsigned int __line, const char *__function)
{
  dce_abort ();
}

void
dce___stack_chk_fail (void)
{
  dce_abort ();
}

int dce_pause (void)
{
  //Thread *current = Current ();
  //NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  //NS_ASSERT (current != 0);
  // XXX
  return 0;
}


int dce_gettimeofday (struct timeval *tv, struct timezone *tz)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  NS_ASSERT (tz == 0);
  *tv = UtilsTimeToTimeval (UtilsSimulationTimeToTime (Now ()));
  return 0;
}
int dce_nanosleep (const struct timespec *req, struct timespec *rem)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  if (req == 0)
    {
      current->err = EFAULT;
      return -1;
    }
  if ((req->tv_sec < 0) || (req->tv_nsec < 0) || (req->tv_nsec > 999999999))
    {
      current->err = EINVAL;
      return -1;
    }
  Time reqTime = UtilsTimespecToTime (*req);
  Time remTime = current->process->manager->Wait (reqTime);
  if (remTime == Seconds (0.0))
    {
      return 0;
    }
  else
    {
      current->err = EINTR;
      if (rem != 0)
        {
          *rem = UtilsTimeToTimespec (remTime);
        }
      return -1;
    }
}

long int dce_random (void)
{
  Thread *current = Current ();
  return current->process->rndVariable->GetInteger ();
}
int dce_rand (void)
{
  Thread *current = Current ();
  return current->process->rndVariable->GetInteger ();
}
unsigned short int * dce_seed48 (unsigned short int seed16v[3])
{
  Thread *current = Current ();
  seed48_r (seed16v, &(current->process->seed48Current));

  return current->process->seed48Current.__old_x;
}
double dce_drand48 (void)
{
  Thread *current = Current ();
  double res;

  drand48_r (&(current->process->seed48Current), &res);

  return res;
}
long int dce_nrand48 (unsigned short int xsubi[3])
{
  Thread *current = Current ();

  long int res = 0;

  nrand48_r (xsubi, &(current->process->seed48Current), &res);

  return res;
}
long int dce_lrand48 (void)
{
  Thread *current = Current ();

  long int res = 0;

  lrand48_r (&(current->process->seed48Current), &res);

  return res;
}

long int dce_mrand48 (void)
{
  Thread *current = Current ();

  long int res;

  jrand48_r (current->process->seed48Current.__x,  &(current->process->seed48Current), &res);

  return res;
}

double dce_erand48 (unsigned short xsubi[3])
{
  Thread *current = Current ();
  double res;

  erand48_r (xsubi,  &(current->process->seed48Current), &res);

  return res;
}
long int dce_jrand48 (unsigned short int xsubi[3])
{
  Thread *current = Current ();

  long int res = 0;

  jrand48_r (xsubi, &(current->process->seed48Current), &res);

  return res;
}

void dce_srand48 (long int seedval)
{
  Thread *current = Current ();

  srand48_r (seedval, &(current->process->seed48Current));
}

void dce_lcong48 (unsigned short param[7])
{
  Thread *current = Current ();

  lcong48_r (param, &(current->process->seed48Current));
}

//ignore seeds as RandomVariable implementation ensures that we take different random streams.
//TODO: support getting the same rng stream for several processes
void dce_srandom (unsigned int seed)
{
  return;
}
void dce_srand (unsigned int seed)
{
  return;
}

const char * dce_inet_ntop (int af, const void *src,
                            char *dst, socklen_t cnt)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << af << src << dst << cnt);
  Thread *current = Current ();
  const char *retval = inet_ntop (af, src, dst, cnt);
  if (retval == 0)
    {
      current->err = errno;
    }
  return retval;
}
int dce_getopt (int argc, char * const argv[], const char *optstring)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << argc << argv << optstring);
  NS_ASSERT (Current () != 0);
  Process *process = Current ()->process;

  /* The following is pretty evil but it all comes down to the fact
   * that the libc does not export getopt_internal_r which is really the
   * function we want to call here.
   */
  char *optargsaved = optarg;
  int optindsaved = optind;
  int opterrsaved = opterr;
  int optoptsaved = optopt;
  optarg = *process->poptarg;
  optind = *process->poptind;
  opterr = *process->popterr;
  optopt = *process->poptopt;
  int retval = getopt (argc, argv, optstring);
  *process->poptarg = optarg;
  *process->poptind = optind;
  *process->popterr = opterr;
  *process->poptopt = optopt;
  optarg = optargsaved;
  optind = optindsaved;
  opterr = opterrsaved;
  optopt = optoptsaved;
  return retval;
}
int dce_getopt_long (int argc, char * const argv[], const char *optstring,
                     const struct option *longopts, int *longindex)
{
  NS_LOG_FUNCTION (Current () << "node" << UtilsGetNodeId () << argc << argv << optstring <<
                   longopts << longindex);
  NS_ASSERT (Current () != 0);
  Process *process = Current ()->process;

  /* The following is pretty evil but it all comes down to the fact
   * that the libc does not export getopt_internal_r which is really the
   * function we want to call here.
   */
  char *optargsaved = optarg;
  int optindsaved = optind;
  int opterrsaved = opterr;
  int optoptsaved = optopt;
  optarg = *process->poptarg;
  optind = *process->poptind;
  opterr = *process->popterr;
  optopt = *process->poptopt;
  int retval = getopt_long (argc, argv, optstring, longopts, longindex);
  *process->poptarg = optarg;
  *process->poptind = optind;
  *process->popterr = opterr;
  *process->poptopt = optopt;
  optarg = optargsaved;
  optind = optindsaved;
  opterr = opterrsaved;
  optopt = optoptsaved;
  return retval;
}
int dce_sched_yield (void)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  current->process->manager->Yield ();
  return 0;
}
static void Itimer (Process *process)
{
  if (!process->itimerInterval.IsZero ())
    {
      process->itimer = Simulator::Schedule (process->itimerInterval,
                                             &Itimer, process);
    }
  // wakeup one thread
  UtilsSendSignal (process, SIGALRM);
}
int dce_getitimer (int which, struct itimerval *value)
{

  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << which << value);
  NS_ASSERT (current != 0);
  if (value == 0)
    {
      current->err = EFAULT;
      return -1;
    }
  // We don't support other kinds of timers.
  NS_ASSERT (which == ITIMER_REAL);
  value->it_interval = UtilsTimeToTimeval (current->process->itimerInterval);
  value->it_value = UtilsTimeToTimeval (Simulator::GetDelayLeft (current->process->itimer));
  return 0;
}
int dce_setitimer (int which, const struct itimerval *value,
                   struct itimerval *ovalue)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << which << value << ovalue);
  NS_ASSERT (current != 0);
  if (value == 0)
    {
      current->err = EINVAL;
      return -1;
    }
  // We don't support other kinds of timers.
  NS_ASSERT (which == ITIMER_REAL);
  if (ovalue != 0)
    {
      ovalue->it_interval = UtilsTimeToTimeval (current->process->itimerInterval);
      ovalue->it_value = UtilsTimeToTimeval (Simulator::GetDelayLeft (current->process->itimer));
    }

  current->process->itimer.Cancel ();
  current->process->itimerInterval = UtilsTimevalToTime (value->it_interval);
  if (value->it_value.tv_sec == 0
      && value->it_value.tv_usec == 0)
    {
      return 0;
    }
  TaskManager *manager = TaskManager::Current ();
  current->process->itimer = manager->ScheduleMain (
      UtilsTimevalToTime (value->it_value),
      MakeEvent (&Itimer, current->process));

  return 0;
}
char * dce_getcwd (char *buf, size_t size)
{
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  uint32_t cwd_size = current->process->cwd.size ();
  if ((buf != 0 && size < cwd_size + 1)
      || (buf == 0 && size != 0 && size < cwd_size + 1))
    {
      current->err = ERANGE;
      return 0;
    }
  // from here on, we know that we will have enough space
  // in the buffer for the strcpy
  if (buf == 0)
    {
      if (size == 0)
        {
          buf = (char *)dce_malloc (cwd_size + 1);
          size = cwd_size + 1;
        }
      else
        {
          buf = (char *)dce_malloc (size);
        }
      buf[size - 1] = 0;
    }
  const char *source = current->process->cwd.c_str ();
  strcpy (buf, source);
  return buf;
}
char * dce_getwd (char *buf)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << buf);
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();
  uint32_t cwd_size = current->process->cwd.size ();
  if (PATH_MAX < cwd_size + 1)
    {
      current->err = ENAMETOOLONG;
      return 0;
    }
  const char *source = current->process->cwd.c_str ();
  strcpy (buf, source);
  return buf;
}
char * dce_get_current_dir_name (void)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  return dce_getcwd (0, 0);
}

int dce_chdir (const char *path)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);

  int retval;
  std::string newCwd = UtilsGetRealFilePath (path);
  // test to see if the target directory exists
  retval = ::open (newCwd.c_str (), O_DIRECTORY | O_RDONLY);
  if (retval == -1)
    {
      current->err = errno;
      return -1;
    }
  ::close (retval);
  current->process->cwd = UtilsGetVirtualFilePath (path);
  return 0;
}
int dce_fchdir (int fd)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  int realFd = getRealFd (fd, current);

  if (realFd < 0)
    {
      current->err = EBADF;
      return -1;
    }
  std::string p = PathOfFd (realFd);

  if (0 == p.length ())
    {
      current->err = EBADF;
      return -1;
    }
  std::string base = UtilsGetCurrentDirName () + "/" +  UtilsGetRealFilePath ("/");
  current->process->cwd =  UtilsGetVirtualFilePath (std::string (p, base.length () - 1));
  return 0;
}
unsigned dce_if_nametoindex (const char *ifname)
{
  Thread *current = Current ();
  Ptr<SocketFdFactory> factory = 0;
  factory = current->process->manager->GetObject<SocketFdFactory> ();

  if (factory->GetInstanceTypeId () == TypeId::LookupByName ("ns3::LinuxSocketFdFactory"))
    {
      struct ifreq ifr;
      int fd = dce_socket (AF_INET, SOCK_DGRAM, 0);
      if (fd < 0)
        {
          return 0;
        }

      strncpy (ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
      if (dce_ioctl (fd, SIOCGIFINDEX, (char *)&ifr) < 0)
        {
          current->err = errno;
          return -1;
        }
      return ifr.ifr_ifindex;
    }
  else
    {
      int index = 0;
      Ptr<Node> node = Current ()->process->manager->GetObject<Node> ();
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

      for (uint32_t i = 0; i < node->GetNDevices (); ++i)
        {
          Ptr<NetDevice> dev = node->GetDevice (i);
          if (ifname == Names::FindName (dev))
            {
              index = ipv4->GetInterfaceForDevice (dev);
              return index;
            }
        }
      return 0;
    }
}
char * dce_if_indextoname (unsigned ifindex, char *ifname)
{
  struct ifreq ifr;
  int fd = dce_socket (AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    {
      return 0;
    }

  ifr.ifr_ifindex = ifindex;
  if (dce_ioctl (fd, SIOCGIFNAME, (char *)&ifr) < 0)
    {
      return 0;
    }
  return strncpy (ifname, ifr.ifr_name, IFNAMSIZ);
}
pid_t dce_fork (void)
{
  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread);
  DceManager *manager = thread->process->manager;
  return manager->Clone (thread);
}

int dce_execv (const char *path, char *const argv[])
{
  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread << UtilsGetNodeId () << path);

  std::string fileName = SearchExecFile (path, getuid (), getgid (), &(thread->err));

  if  (0 == fileName.length ())
    {
      // Errno setted by FindExecFile
      return -1;
    }

  return thread->process->manager->Execve (fileName.c_str (), path, argv, *(thread->process->penvp));
}
int dce_execl (const char *path, const char *arg, ...)
{
  va_list ap;
  va_start (ap, arg);

  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread << UtilsGetNodeId () << path);

  std::string fileName = SearchExecFile (path, getuid (), getgid (), &(thread->err));

  if  (0 == fileName.length ())
    {
      // Errno setted by FindExecFile
      return -1;
    }

  int nb = 1;

  va_list cp;
  va_copy (cp, ap);
  char *p =  0;
  do
    {
      p = va_arg (cp, char *);
      nb++;
    }
  while (p);

  char const** argv = (char const **) dce_malloc (nb * sizeof (char *)); // Use dce_malloc to be sure it will be freed when exec is successfull

  argv[0] = arg;
  nb = 1;

  do
    {
      argv[nb++] = p = va_arg (ap, char *);
    }
  while (p);

  int retval = thread->process->manager->Execve (fileName.c_str (), path, (char* const*) argv, *(thread->process->penvp));

  dce_free (argv);

  return retval;
}
int dce_execve (const char *path, char *const argv[], char *const envp[])
{
  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread << UtilsGetNodeId () << path);

  std::string fileName = SearchExecFile (path, getuid (), getgid (), &(thread->err));

  if  (0 == fileName.length ())
    {
      // Errno setted by FindExecFile
      return -1;
    }

  return thread->process->manager->Execve (fileName.c_str (), path, argv, envp);
}

int dce_execlp (const char *file, const char *arg, ...)
{
  va_list ap;
  va_start (ap, arg);

  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread << UtilsGetNodeId () << file);

  std::string vpath = "";
  char *pvpath = seek_env ("PATH", *thread->process->penvp);
  if (pvpath)
    {
      vpath = std::string (pvpath);
    }
  std::string fileName = file;

  fileName = SearchExecFile (fileName, vpath, getuid (), getgid (), &(thread->err));

  if  (0 == fileName.length ())
    {
      // Errno setted by FindExecFile
      return -1;
    }

  int nb = 1;
  va_list cp;
  va_copy (cp, ap);
  char *p =  0;
  do
    {
      p = va_arg (cp, char *);
      nb++;
    }
  while (p);

  char const** argv = (char const **) dce_malloc (nb * sizeof (char *)); // Use dce_malloc to be sure it will be freed when exec is successfull

  argv[0] = arg;
  nb = 1;

  do
    {
      argv[nb++] = p = va_arg (ap, char *);
    }
  while (p);

  int retval = thread->process->manager->Execve (fileName.c_str (), file, (char* const*) argv, *(thread->process->penvp));

  dce_free (argv);

  return retval;
}
int dce_execvp (const char *file, char *const argv[])
{
  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread << UtilsGetNodeId () << file);

  std::string vpath = "";
  char *pvpath = seek_env ("PATH", *thread->process->penvp);
  if (pvpath)
    {
      vpath = std::string (pvpath);
    }
  std::string fileName = file;
  fileName = SearchExecFile (fileName, vpath, getuid (), getgid (), &(thread->err));
  if  (0 == fileName.length ())
    {
      // Errno setted by FindExecFile
      return -1;
    }

  return thread->process->manager->Execve (fileName.c_str (), file, argv, *(thread->process->penvp));
}
int dce_execle (const char *path, const char *arg, ...)
{
  va_list ap;
  va_start (ap, arg);

  Thread *thread = Current ();
  NS_LOG_FUNCTION (thread << UtilsGetNodeId () << path);
  std::string fileName = SearchExecFile (path,  getuid (), getgid (), &(thread->err));

  if  (0 == fileName.length ())
    {
      // Errno setted by FindExecFile
      return -1;
    }
  int nb = 1;
  va_list cp;
  va_copy (cp, ap);
  char *p =  0;
  do
    {
      p = va_arg (cp, char *);
      nb++;
    }
  while (p);

  char const** envp = (char const **) va_arg (cp, char **);
  char const** argv = (char const **) dce_malloc (nb * sizeof (char *)); // Use dce_malloc to be sure it will be freed when exec is successfull

  argv[0] = arg;
  nb = 1;

  do
    {
      argv[nb++] = p = va_arg (ap, char *);
    }
  while (p);

  int retval = thread->process->manager->Execve (fileName.c_str (), path,
                                                 (char* const*) argv, (char* const*) envp);

  dce_free (argv);

  return retval;
}

char * dce_setlocale (int category, const char *locale)
{
  static char loc[] = "";
  return loc;
}
int dce_sysinfo (struct sysinfo *info)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);
  if (!info)
    {
      current->err = ENAMETOOLONG;
      return -1;
    }

  info->uptime = (long)UtilsSimulationTimeToTime (Now ()).GetSeconds ();
  // XXX
  return 0;
}
int dce_daemon (int nochdir, int noclose)
{
  return 0;
}
unsigned int dce_alarm (unsigned int s)
{
  struct itimerval it;
  memset (&it, 0, sizeof (it));

  unsigned int ret = 0;

  if (!dce_getitimer (ITIMER_REAL, &it))
    {
      ret = it.it_value.tv_sec;
    }

  memset (&it, 0, sizeof (it));
  it.it_value.tv_sec = s;
  it.it_value.tv_usec = 0;

  dce_setitimer (ITIMER_REAL, &it, NULL);

  return ret;
}
ssize_t dce_readlink (const char *path, char *buf, size_t bufsize)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);

  std::string fullpath = UtilsGetRealFilePath (path);

  ssize_t ret = readlink (fullpath.c_str (), buf, bufsize);

  if (ret)
    {
      current->err = errno;
      return ret;
    }

  int l = UtilsGetRealFilePath ("/").length ();

  memcpy (buf, buf + l, l);
  buf [l] = 0;

  return 0;
}
#ifdef HAVE_GETCPUFEATURES
extern "C"
{
extern const struct cpu_features * __get_cpu_features (void);
}
const struct cpu_features * dce___get_cpu_features (void)
{
  return __get_cpu_features ();
}
#endif // HAVE_GETCPUFEATURES
