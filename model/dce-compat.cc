/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "dce-compat.h"
#include "dce-fcntl.h"
#include "dce-unistd.h"
#include <algorithm>
#include "dce-stdio.h"
#include "dce-stdlib.h"
#include "dce-dirent.h"
#include "dce-epoll.h"
#include "dce-syslog.h"
#include "sys/dce-stat.h"
#include "process.h"
#include "unix-file-fd.h"
#include "file-usage.h"
#include "utils.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>
#include <sys/mman.h>
#include <sched.h>

NS_LOG_COMPONENT_DEFINE ("DceCompat");

using namespace ns3;

extern "C" int dce_accept (int fd, struct sockaddr *my_addr, socklen_t *addrlen);

// Path with the AT_FDCWD/absolute semantics the *at() functions use: only
// those two cases can be mapped into the node file system.
static bool
AtPath (int dirfd, const char *pathname, std::string &full)
{
  if (pathname == 0 || (dirfd != AT_FDCWD && pathname[0] != '/'))
    {
      Current ()->err = ENOSYS;
      return false;
    }
  full = UtilsGetRealFilePath (pathname);
  return true;
}

static int
Unsupported (int err)
{
  Current ()->err = err;
  return -1;
}

int dce_accept4 (int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
  int r = dce_accept (fd, addr, addrlen);
  if (r >= 0 && (flags & SOCK_NONBLOCK))
    {
      dce_fcntl (r, F_SETFL, dce_fcntl (r, F_GETFL, 0) | O_NONBLOCK);
    }
  return r;
}

int dce_faccessat (int dirfd, const char *pathname, int mode, int flags)
{
  if (dirfd != AT_FDCWD && pathname[0] != '/')
    {
      return Unsupported (ENOSYS);
    }
  return dce_access (pathname, mode);
}

int dce_fchmod (int fd, mode_t mode)
{
  return 0; // permissions of files in the node file system are not modelled
}

int dce_lchmod (const char *path, mode_t mode)
{
  return 0;
}

int dce_lchown (const char *path, uid_t owner, gid_t group)
{
  return 0;
}

int dce_creat64 (const char *path, mode_t mode)
{
  return dce_creat (path, mode);
}

int dce_openat64 (int dirfd, const char *pathname, int flags, ...)
{
  mode_t mode = 0;
  if (flags & O_CREAT)
    {
      va_list vl;
      va_start (vl, flags);
      mode = va_arg (vl, mode_t);
      va_end (vl);
    }
  return dce_openat (dirfd, pathname, flags, mode);
}

int dce___open64_2 (const char *path, int flags)
{
  return dce_open (path, flags);
}

int dce___openat_2 (int dirfd, const char *path, int flags)
{
  return dce_openat (dirfd, path, flags);
}

FILE * dce_freopen64 (const char *path, const char *mode, FILE *stream)
{
  return dce_freopen (path, mode, stream);
}

FILE * dce_tmpfile64 (void)
{
  return dce_tmpfile ();
}

int dce_mkostemp64 (char *temp, int flags)
{
  int fd = dce_mkstemp (temp);
  if (fd >= 0 && (flags & O_NONBLOCK))
    {
      dce_fcntl (fd, F_SETFL, dce_fcntl (fd, F_GETFL, 0) | O_NONBLOCK);
    }
  return fd;
}

// struct dirent and struct dirent64 have the same layout on 64 bit targets.
int dce_scandir64 (const char *dirp, struct dirent64 ***namelist,
                   int (*filter)(const struct dirent64 *),
                   int (*compar)(const struct dirent64 **, const struct dirent64 **))
{
  return dce_scandir (dirp, (struct dirent ***)namelist,
                      (int (*)(const struct dirent *))filter,
                      (int (*)(const struct dirent **, const struct dirent **))compar);
}

int dce_scandirat (int dirfd, const char *dirp, struct dirent ***namelist,
                   int (*filter)(const struct dirent *),
                   int (*compar)(const struct dirent **, const struct dirent **))
{
  if (dirfd != AT_FDCWD && dirp[0] != '/')
    {
      return Unsupported (ENOSYS);
    }
  return dce_scandir (dirp, namelist, filter, compar);
}

int dce_utimensat (int dirfd, const char *pathname, const struct timespec times[2], int flags)
{
  std::string full;
  if (!AtPath (dirfd, pathname, full))
    {
      return -1;
    }
  int r = ::utimensat (AT_FDCWD, full.c_str (), times, flags);
  if (r == -1)
    {
      Current ()->err = errno;
    }
  return r;
}

int dce_statx (int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *buf)
{
  std::string full;
  if (pathname == 0 || pathname[0] == 0 || !AtPath (dirfd, pathname, full))
    {
      return Unsupported (ENOSYS); // the caller falls back to fstat()
    }
  int r = ::statx (AT_FDCWD, full.c_str (), flags, mask, buf);
  if (r == -1)
    {
      Current ()->err = errno;
    }
  return r;
}

int dce_statvfs64 (const char *path, struct statvfs64 *buf)
{
  int r = ::statvfs64 (UtilsGetRealFilePath (path).c_str (), buf);
  if (r == -1)
    {
      Current ()->err = errno;
    }
  return r;
}

int dce_fallocate64 (int fd, int mode, off64_t offset, off64_t len)
{
  return Unsupported (EOPNOTSUPP);
}

int dce_posix_fallocate64 (int fd, off64_t offset, off64_t len)
{
  return EOPNOTSUPP;
}

ssize_t dce_copy_file_range (int fd_in, off64_t *off_in, int fd_out, off64_t *off_out, size_t len, unsigned int flags)
{
  return Unsupported (ENOSYS); // callers fall back to read()/write()
}

ssize_t dce_splice (int fd_in, off64_t *off_in, int fd_out, off64_t *off_out, size_t len, unsigned int flags)
{
  return Unsupported (ENOSYS);
}

// Extended attributes are not modelled: report them as unsupported, which
// is what a file system without them does.
ssize_t dce_getxattr (const char *path, const char *name, void *value, size_t size)
{
  return Unsupported (ENOTSUP);
}
ssize_t dce_lgetxattr (const char *path, const char *name, void *value, size_t size)
{
  return Unsupported (ENOTSUP);
}
ssize_t dce_fgetxattr (int fd, const char *name, void *value, size_t size)
{
  return Unsupported (ENOTSUP);
}
ssize_t dce_listxattr (const char *path, char *list, size_t size)
{
  return Unsupported (ENOTSUP);
}
ssize_t dce_llistxattr (const char *path, char *list, size_t size)
{
  return Unsupported (ENOTSUP);
}
ssize_t dce_flistxattr (int fd, char *list, size_t size)
{
  return Unsupported (ENOTSUP);
}
int dce_setxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
  return Unsupported (ENOTSUP);
}
int dce_lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
  return Unsupported (ENOTSUP);
}
int dce_fsetxattr (int fd, const char *name, const void *value, size_t size, int flags)
{
  return Unsupported (ENOTSUP);
}
int dce_removexattr (const char *path, const char *name)
{
  return Unsupported (ENOTSUP);
}

// inotify descriptors would be host descriptors unknown to the DCE file
// table: report the facility as unavailable, libraries then do without
// file monitoring.
int dce_inotify_init (void)
{
  return Unsupported (ENOSYS);
}
int dce_inotify_init1 (int flags)
{
  return Unsupported (ENOSYS);
}
int dce_inotify_add_watch (int fd, const char *pathname, uint32_t mask)
{
  return Unsupported (EBADF);
}
int dce_inotify_rm_watch (int fd, int wd)
{
  return Unsupported (EBADF);
}

// No child processes of the host from the simulation.
int dce_posix_spawn (pid_t *pid, const char *path, const posix_spawn_file_actions_t *file_actions,
                     const posix_spawnattr_t *attrp, char *const argv[], char *const envp[])
{
  NS_LOG_WARN ("posix_spawn(\"" << (path ? path : "") << "\") is not supported");
  return ENOSYS;
}
int dce_posix_spawnp (pid_t *pid, const char *file, const posix_spawn_file_actions_t *file_actions,
                      const posix_spawnattr_t *attrp, char *const argv[], char *const envp[])
{
  NS_LOG_WARN ("posix_spawnp(\"" << (file ? file : "") << "\") is not supported");
  return ENOSYS;
}
int dce_posix_spawnattr_init (posix_spawnattr_t *attr)
{
  return 0;
}
int dce_posix_spawnattr_destroy (posix_spawnattr_t *attr)
{
  return 0;
}
int dce_posix_spawnattr_setflags (posix_spawnattr_t *attr, short flags)
{
  return 0;
}
int dce_posix_spawnattr_setsigdefault (posix_spawnattr_t *attr, const sigset_t *sigdefault)
{
  return 0;
}
int dce_posix_spawn_file_actions_init (posix_spawn_file_actions_t *file_actions)
{
  return 0;
}
int dce_posix_spawn_file_actions_destroy (posix_spawn_file_actions_t *file_actions)
{
  return 0;
}
int dce_posix_spawn_file_actions_addclose (posix_spawn_file_actions_t *file_actions, int fd)
{
  return 0;
}
int dce_posix_spawn_file_actions_adddup2 (posix_spawn_file_actions_t *file_actions, int fd, int newfd)
{
  return 0;
}
int dce_waitid (idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
  return Unsupported (ECHILD);
}
int dce_close_range (unsigned int first, unsigned int last, int flags)
{
  return Unsupported (ENOSYS); // callers fall back to close() loops
}
int dce_recvmmsg (int fd, struct mmsghdr *msgvec, unsigned int vlen, int flags, struct timespec *timeout)
{
  return Unsupported (ENOSYS);
}

// fortified variants
char * dce___getcwd_chk (char *buf, size_t size, size_t buflen)
{
  return dce_getcwd (buf, size);
}
char * dce___fgets_unlocked_chk (char *buf, size_t size, int n, FILE *stream)
{
  return dce_fgets (buf, n, stream);
}
int dce___vsprintf_chk (char *s, int flag, size_t slen, const char *fmt, va_list ap)
{
  // See __sprintf_chk in libc.cc: an unknown size is (size_t) -1.
  return dce_vsnprintf (s, slen > INT_MAX ? INT_MAX : slen, fmt, ap);
}

// The stack of a DCE thread is not described by a pthread_attr_t; the
// callers (WAMR) fall back to their defaults.
void dce__Exit (int status)
{
  dce_exit (status);
}

int dce_fdatasync (int fd)
{
  return dce_fsync (fd);
}

ssize_t dce_pread64 (int fd, void *buf, size_t count, off64_t offset)
{
  return dce_pread (fd, buf, count, offset);
}

ssize_t dce_pwrite64 (int fd, const void *buf, size_t count, off64_t offset)
{
  return dce_pwrite (fd, buf, count, offset);
}

int dce_mallopt (int param, int value)
{
  return 1; // accepted, no effect on the DCE allocator
}

// sendfile(2) as a read/write loop over DCE file descriptors (OpenSSL's
// kernel TLS path refers to it).
ssize_t dce_sendfile (int out_fd, int in_fd, off_t *offset, size_t count)
{
  char buf[16384];
  size_t done = 0;
  while (done < count)
    {
      size_t want = std::min (sizeof (buf), count - done);
      ssize_t got = offset ? dce_pread (in_fd, buf, want, *offset + done)
        : dce_read (in_fd, buf, want);
      if (got <= 0)
        {
          if (got < 0 && done == 0)
            {
              return -1;
            }
          break;
        }
      ssize_t put = dce_write (out_fd, buf, got);
      if (put <= 0)
        {
          if (done == 0)
            {
              return -1;
            }
          break;
        }
      done += put;
      if (put < got)
        {
          break;
        }
    }
  if (offset)
    {
      *offset += done;
    }
  return done;
}

// DCE threads are not host threads: the affinity is the simulator's.
int dce_pthread_getaffinity_np (pthread_t thread, size_t cpusetsize, cpu_set_t *cpuset)
{
  if (::sched_getaffinity (0, cpusetsize, cpuset) != 0)
    {
      return errno;
    }
  return 0;
}

int dce_pthread_setaffinity_np (pthread_t thread, size_t cpusetsize, const cpu_set_t *cpuset)
{
  return 0;
}

// libpulse / libsystemd / libdbus
int dce_epoll_pwait2 (int epfd, struct epoll_event *events, int maxevents,
                      const struct timespec *timeout, const sigset_t *sigmask)
{
  int ms = -1;
  if (timeout != 0)
    {
      ms = timeout->tv_sec * 1000 + (timeout->tv_nsec + 999999) / 1000000;
    }
  return dce_epoll_wait (epfd, events, maxevents, ms);
}

int dce_fstatvfs64 (int fd, struct statvfs64 *buf)
{
  return Unsupported (ENOSYS);
}

long dce_fpathconf (int fd, int name)
{
  switch (name)
    {
    case _PC_PIPE_BUF:
      return 4096;
    case _PC_NAME_MAX:
      return 255;
    case _PC_PATH_MAX:
      return 4096;
    default:
      return -1;
    }
}

int dce_fremovexattr (int fd, const char *name)
{
  return Unsupported (ENOTSUP);
}

int dce_mkdirat (int dirfd, const char *pathname, mode_t mode)
{
  std::string full;
  if (!AtPath (dirfd, pathname, full))
    {
      return -1;
    }
  int r = ::mkdir (full.c_str (), mode);
  if (r == -1)
    {
      Current ()->err = errno;
    }
  return r;
}

int dce___openat64_2 (int dirfd, const char *path, int flags)
{
  return dce_openat (dirfd, path, flags);
}

ssize_t dce_readlinkat (int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
  if (dirfd != AT_FDCWD && pathname[0] != '/')
    {
      return Unsupported (ENOSYS);
    }
  return dce_readlink (pathname, buf, bufsiz);
}

ssize_t dce___readlinkat_chk (int dirfd, const char *pathname, char *buf, size_t bufsiz, size_t buflen)
{
  return dce_readlinkat (dirfd, pathname, buf, bufsiz);
}

int dce_signalfd (int fd, const sigset_t *mask, int flags)
{
  return Unsupported (ENOSYS);
}

int dce_name_to_handle_at (int dirfd, const char *pathname, struct file_handle *handle, int *mount_id, int flags)
{
  return Unsupported (ENOSYS);
}

// Process-wide host settings a DCE application must not change.
int dce_personality (unsigned long persona)
{
  return 0;
}
int dce_capget (void *hdrp, void *datap)
{
  return Unsupported (ENOSYS);
}
int dce_capset (void *hdrp, const void *datap)
{
  return Unsupported (EPERM);
}
int dce_setpriority (int which, id_t who, int prio)
{
  return 0;
}
int dce_pthread_setschedparam (pthread_t thread, int policy, const struct sched_param *param)
{
  return 0; // DCE threads are scheduled by the simulator
}
int dce_pthread_getschedparam (pthread_t thread, int *policy, struct sched_param *param)
{
  if (policy)
    {
      *policy = SCHED_OTHER;
    }
  if (param)
    {
      memset (param, 0, sizeof (*param));
    }
  return 0;
}

// SDL2
int dce_fseeko64 (FILE *stream, off64_t offset, int whence)
{
  return dce_fseeko (stream, offset, whence);
}
off64_t dce_ftello64 (FILE *stream)
{
  return dce_ftello (stream);
}
int dce_pthread_setcanceltype (int type, int *oldtype)
{
  if (oldtype)
    {
      *oldtype = PTHREAD_CANCEL_DEFERRED;
    }
  return 0;
}
int dce_sched_getparam (pid_t pid, struct sched_param *param)
{
  if (param)
    {
      memset (param, 0, sizeof (*param));
    }
  return 0;
}
int dce_sched_getscheduler (pid_t pid)
{
  return SCHED_OTHER;
}
int dce_sched_setscheduler (pid_t pid, int policy, const struct sched_param *param)
{
  return 0; // DCE processes are scheduled by the simulator
}
int dce_sigtimedwait (const sigset_t *set, siginfo_t *info, const struct timespec *timeout)
{
  return Unsupported (EAGAIN);
}
pid_t dce_vfork (void)
{
  return Unsupported (ENOSYS);
}

int dce___xstat64 (int ver, const char *path, struct stat64 *buf)
{
  return dce_stat64 (path, buf);
}

int dce___lxstat64 (int ver, const char *path, struct stat64 *buf)
{
  return dce_lstat64 (path, buf);
}

int dce___fxstat64 (int ver, int fd, struct stat64 *buf)
{
  return dce_fstat64 (fd, buf);
}

int dce___fxstatat64 (int ver, int dirfd, const char *path, struct stat64 *buf, int flags)
{
  return dce_fstatat64 (dirfd, path, buf, flags);
}

int dce_futimens (int fd, const struct timespec times[2])
{
  // the time stamps of an open file: the node's files keep the host's
  return 0;
}

int dce_posix_fadvise64 (int fd, off64_t offset, off64_t len, int advice)
{
  return dce_posix_fadvise (fd, offset, len, advice);
}

ssize_t dce_sendfile64 (int out_fd, int in_fd, off64_t *offset, size_t count)
{
  return dce_sendfile (out_fd, in_fd, (off_t *) offset, count);
}

int dce_tcdrain (int fd)
{
  // the terminals of the simulation write through at once
  return 0;
}

int dce_pthread_getname_np (pthread_t thread, char *name, size_t len)
{
  // pthread_setname_np() keeps no name
  if (len < 1)
    {
      return ERANGE;
    }
  name[0] = 0;
  return 0;
}

pid_t dce_gettid (void)
{
  // Linux: the main thread's is the pid, the others' are unique too
  Thread *current = Current ();
  NS_ASSERT (current != 0);
  if (current->tid == 0)
    {
      return current->process->pid;
    }
  return ((current->process->pid & 0x7fff) << 16) | current->tid;
}

int dce_memfd_create (const char *name, unsigned int flags)
{
  // an anonymous file of the host, as a file descriptor of the process
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId () << name << flags);
  NS_ASSERT (current != 0);
  int fd = UtilsAllocateFd ();
  if (fd == -1)
    {
      current->err = EMFILE;
      return -1;
    }
  int realFd = ::memfd_create (name, flags);
  if (realFd == -1)
    {
      current->err = errno;
      return -1;
    }
  UnixFd *unixFd = new UnixFileFd (realFd);
  unixFd->IncFdCount ();
  current->process->openFiles[fd] = new FileUsage (fd, unixFd);
  return fd;
}

static std::string
ShmPath (const char *name)
{
  // POSIX shared memory objects are files in the node's /dev/shm
  while (*name == '/')
    {
      name++;
    }
  return std::string ("/dev/shm/") + name;
}

int dce_shm_open (const char *name, int oflag, mode_t mode)
{
  UtilsEnsureAllDirectoriesExist (UtilsGetRealFilePath ("/dev/shm/"));
  return dce_open (ShmPath (name).c_str (), oflag | O_NOFOLLOW | O_CLOEXEC, mode);
}

int dce_shm_unlink (const char *name)
{
  return dce_unlink (ShmPath (name).c_str ());
}

// The host directory and path for a path of the node's file system relative
// to dirfd, as the *at functions take them: false if dirfd is not a
// directory of the process.
static bool
AtPath (int dirfd, const char *path, int &realDirFd, std::string &realPath)
{
  Thread *current = Current ();
  if (dirfd != AT_FDCWD && path[0] != '/')
    {
      realDirFd = getRealFd (dirfd, current);
      realPath = path;
      if (realDirFd < 0)
        {
          current->err = EBADF;
          return false;
        }
      return true;
    }
  realDirFd = AT_FDCWD;
  realPath = UtilsGetRealFilePath (path);
  return true;
}

static int
AtResult (int retval)
{
  if (retval == -1)
    {
      Current ()->err = errno;
    }
  return retval;
}

int dce_symlink (const char *target, const char *linkpath)
{
  return dce_symlinkat (target, AT_FDCWD, linkpath);
}

int dce_symlinkat (const char *target, int newdirfd, const char *linkpath)
{
  // the link's contents are the program's, as it wrote them
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << target << linkpath);
  int dirFd;
  std::string path;
  if (!AtPath (newdirfd, linkpath, dirFd, path))
    {
      return -1;
    }
  return AtResult (::symlinkat (target, dirFd, path.c_str ()));
}

int dce_link (const char *oldpath, const char *newpath)
{
  return dce_linkat (AT_FDCWD, oldpath, AT_FDCWD, newpath, 0);
}

int dce_linkat (int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << oldpath << newpath);
  int oldDirFd, newDirFd;
  std::string oldReal, newReal;
  if (!AtPath (olddirfd, oldpath, oldDirFd, oldReal) || !AtPath (newdirfd, newpath, newDirFd, newReal))
    {
      return -1;
    }
  return AtResult (::linkat (oldDirFd, oldReal.c_str (), newDirFd, newReal.c_str (), flags));
}

int dce_chmod (const char *path, mode_t mode)
{
  return dce_fchmodat (AT_FDCWD, path, mode, 0);
}

int dce_fchmodat (int dirfd, const char *path, mode_t mode, int flags)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode);
  int dirFd;
  std::string real;
  if (!AtPath (dirfd, path, dirFd, real))
    {
      return -1;
    }
  return AtResult (::fchmodat (dirFd, real.c_str (), mode, flags));
}

int dce_fchownat (int dirfd, const char *path, uid_t owner, gid_t group, int flags)
{
  // the users of the simulation are not the host's: the file must exist
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << owner << group);
  int dirFd;
  std::string real;
  if (!AtPath (dirfd, path, dirFd, real))
    {
      return -1;
    }
  struct stat st;
  return AtResult (::fstatat (dirFd, real.c_str (), &st, flags & AT_SYMLINK_NOFOLLOW));
}

int dce_renameat (int olddirfd, const char *oldpath, int newdirfd, const char *newpath)
{
  return dce_renameat2 (olddirfd, oldpath, newdirfd, newpath, 0);
}

int dce_renameat2 (int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << oldpath << newpath << flags);
  int oldDirFd, newDirFd;
  std::string oldReal, newReal;
  if (!AtPath (olddirfd, oldpath, oldDirFd, oldReal) || !AtPath (newdirfd, newpath, newDirFd, newReal))
    {
      return -1;
    }
  return AtResult (::renameat2 (oldDirFd, oldReal.c_str (), newDirFd, newReal.c_str (), flags));
}

int dce_mkfifo (const char *path, mode_t mode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode);
  return AtResult (::mkfifo (UtilsGetRealFilePath (path).c_str (), mode));
}

int dce_mknod (const char *path, mode_t mode, dev_t dev)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path << mode);
  return AtResult (::mknod (UtilsGetRealFilePath (path).c_str (), mode, dev));
}

int dce_lutimes (const char *path, const struct timeval tv[2])
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << path);
  return AtResult (::lutimes (UtilsGetRealFilePath (path).c_str (), tv));
}

char *dce_mkdtemp (char *temp)
{
  // the host makes the name in the node's directory; give it back
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << temp);
  std::string real = UtilsGetRealFilePath (temp);
  size_t length = strlen (temp);
  if (length < 6 || real.length () < 6)
    {
      Current ()->err = EINVAL;
      return 0;
    }
  std::vector<char> buffer (real.begin (), real.end ());
  buffer.push_back (0);
  if (::mkdtemp (buffer.data ()) == 0)
    {
      Current ()->err = errno;
      return 0;
    }
  memcpy (temp + length - 6, buffer.data () + real.length () - 6, 6);
  return temp;
}

int dce_mkostemp (char *temp, int flags)
{
  return dce_mkostemp64 (temp, flags);
}

int dce_getrlimit (__rlimit_resource_t resource, struct rlimit *rlim)
{
  return AtResult (::getrlimit (resource, rlim));
}

int dce_getrlimit64 (__rlimit_resource_t resource, struct rlimit64 *rlim)
{
  return AtResult (::getrlimit64 (resource, rlim));
}

int dce_setrlimit (__rlimit_resource_t resource, const struct rlimit *rlim)
{
  return AtResult (::setrlimit (resource, rlim));
}

int dce_setrlimit64 (__rlimit_resource_t resource, const struct rlimit64 *rlim)
{
  return AtResult (::setrlimit64 (resource, rlim));
}
