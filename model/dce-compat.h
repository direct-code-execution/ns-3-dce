/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DCE_COMPAT_H
#define DCE_COMPAT_H
/*
 * Entry points needed by GLib/GIO/GTK and the libraries around them.
 * Most of them map a path into the node file system and call the host, or
 * report the operation as unsupported in a way the library handles.
 */
#include <sys/types.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <dirent.h>
#include <spawn.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <sys/epoll.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_accept4 (int fd, struct sockaddr *addr, socklen_t *addrlen, int flags);
int dce_faccessat (int dirfd, const char *pathname, int mode, int flags);
int dce_fchmod (int fd, mode_t mode);
int dce_lchmod (const char *path, mode_t mode);
int dce_lchown (const char *path, uid_t owner, gid_t group);
int dce_creat64 (const char *path, mode_t mode);
int dce_openat64 (int dirfd, const char *pathname, int flags, ...);
int dce___open64_2 (const char *path, int flags);
int dce___openat_2 (int dirfd, const char *path, int flags);
FILE * dce_freopen64 (const char *path, const char *mode, FILE *stream);
FILE * dce_tmpfile64 (void);
int dce_mkostemp64 (char *temp, int flags);
int dce_scandir64 (const char *dirp, struct dirent64 ***namelist,
                   int (*filter)(const struct dirent64 *),
                   int (*compar)(const struct dirent64 **, const struct dirent64 **));
int dce_scandirat (int dirfd, const char *dirp, struct dirent ***namelist,
                   int (*filter)(const struct dirent *),
                   int (*compar)(const struct dirent **, const struct dirent **));
int dce_utimensat (int dirfd, const char *pathname, const struct timespec times[2], int flags);
int dce_statx (int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *buf);
int dce_statvfs64 (const char *path, struct statvfs64 *buf);
int dce_fallocate64 (int fd, int mode, off64_t offset, off64_t len);
int dce_posix_fallocate64 (int fd, off64_t offset, off64_t len);
ssize_t dce_copy_file_range (int fd_in, off64_t *off_in, int fd_out, off64_t *off_out, size_t len, unsigned int flags);
ssize_t dce_splice (int fd_in, off64_t *off_in, int fd_out, off64_t *off_out, size_t len, unsigned int flags);
ssize_t dce_getxattr (const char *path, const char *name, void *value, size_t size);
ssize_t dce_lgetxattr (const char *path, const char *name, void *value, size_t size);
ssize_t dce_fgetxattr (int fd, const char *name, void *value, size_t size);
ssize_t dce_listxattr (const char *path, char *list, size_t size);
ssize_t dce_llistxattr (const char *path, char *list, size_t size);
ssize_t dce_flistxattr (int fd, char *list, size_t size);
int dce_setxattr (const char *path, const char *name, const void *value, size_t size, int flags);
int dce_lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags);
int dce_fsetxattr (int fd, const char *name, const void *value, size_t size, int flags);
int dce_removexattr (const char *path, const char *name);
int dce_inotify_init (void);
int dce_inotify_init1 (int flags);
int dce_inotify_add_watch (int fd, const char *pathname, uint32_t mask);
int dce_inotify_rm_watch (int fd, int wd);
int dce_posix_spawn (pid_t *pid, const char *path, const posix_spawn_file_actions_t *file_actions,
                     const posix_spawnattr_t *attrp, char *const argv[], char *const envp[]);
int dce_posix_spawnp (pid_t *pid, const char *file, const posix_spawn_file_actions_t *file_actions,
                      const posix_spawnattr_t *attrp, char *const argv[], char *const envp[]);
int dce_posix_spawnattr_init (posix_spawnattr_t *attr);
int dce_posix_spawnattr_destroy (posix_spawnattr_t *attr);
int dce_posix_spawnattr_setflags (posix_spawnattr_t *attr, short flags);
int dce_posix_spawnattr_setsigdefault (posix_spawnattr_t *attr, const sigset_t *sigdefault);
int dce_posix_spawn_file_actions_init (posix_spawn_file_actions_t *file_actions);
int dce_posix_spawn_file_actions_destroy (posix_spawn_file_actions_t *file_actions);
int dce_posix_spawn_file_actions_addclose (posix_spawn_file_actions_t *file_actions, int fd);
int dce_posix_spawn_file_actions_adddup2 (posix_spawn_file_actions_t *file_actions, int fd, int newfd);
int dce_waitid (idtype_t idtype, id_t id, siginfo_t *infop, int options);
int dce_close_range (unsigned int first, unsigned int last, int flags);
int dce_recvmmsg (int fd, struct mmsghdr *msgvec, unsigned int vlen, int flags, struct timespec *timeout);
char * dce___getcwd_chk (char *buf, size_t size, size_t buflen);
char * dce___fgets_unlocked_chk (char *buf, size_t size, int n, FILE *stream);
int dce___vsprintf_chk (char *s, int flag, size_t slen, const char *fmt, va_list ap);
int dce_pthread_getattr_np (pthread_t thread, pthread_attr_t *attr);
int dce_pthread_getaffinity_np (pthread_t thread, size_t cpusetsize, cpu_set_t *cpuset);
ssize_t dce_sendfile (int out_fd, int in_fd, off_t *offset, size_t count);
int dce_pthread_setaffinity_np (pthread_t thread, size_t cpusetsize, const cpu_set_t *cpuset);
void dce__Exit (int status);
int dce_fdatasync (int fd);
ssize_t dce_pread64 (int fd, void *buf, size_t count, off64_t offset);
ssize_t dce_pwrite64 (int fd, const void *buf, size_t count, off64_t offset);
int dce_mallopt (int param, int value);
int dce_epoll_pwait2 (int epfd, struct epoll_event *events, int maxevents,
                      const struct timespec *timeout, const sigset_t *sigmask);
int dce_fstatvfs64 (int fd, struct statvfs64 *buf);
long dce_fpathconf (int fd, int name);
int dce_fremovexattr (int fd, const char *name);
int dce_mkdirat (int dirfd, const char *pathname, mode_t mode);
int dce___openat64_2 (int dirfd, const char *path, int flags);
ssize_t dce_readlinkat (int dirfd, const char *pathname, char *buf, size_t bufsiz);
ssize_t dce___readlinkat_chk (int dirfd, const char *pathname, char *buf, size_t bufsiz, size_t buflen);
int dce_signalfd (int fd, const sigset_t *mask, int flags);
int dce_name_to_handle_at (int dirfd, const char *pathname, struct file_handle *handle, int *mount_id, int flags);
int dce_personality (unsigned long persona);
int dce_capget (void *hdrp, void *datap);
int dce_capset (void *hdrp, const void *datap);
int dce_setpriority (int which, id_t who, int prio);
int dce_pthread_setschedparam (pthread_t thread, int policy, const struct sched_param *param);
int dce_pthread_getschedparam (pthread_t thread, int *policy, struct sched_param *param);
int dce_fseeko64 (FILE *stream, off64_t offset, int whence);
off64_t dce_ftello64 (FILE *stream);
int dce_pthread_setcanceltype (int type, int *oldtype);
int dce_sched_getparam (pid_t pid, struct sched_param *param);
int dce_sched_getscheduler (pid_t pid);
int dce_sched_setscheduler (pid_t pid, int policy, const struct sched_param *param);
int dce_sigtimedwait (const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
pid_t dce_vfork (void);

#ifdef __cplusplus
}
#endif

#endif /* DCE_COMPAT_H */
