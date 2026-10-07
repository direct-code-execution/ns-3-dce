/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "dce-compat.h"
#include "dce-fcntl.h"
#include "dce-unistd.h"
#include "dce-stdio.h"
#include "dce-stdlib.h"
#include "dce-dirent.h"
#include "process.h"
#include "utils.h"
#include "ns3/log.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>

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
  return dce_vsnprintf (s, slen, fmt, ap);
}
