#ifndef SIMU_UNISTD_H
#define SIMU_UNISTD_H

#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>

#include "../dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(ssize_t , write, (int fd, const void *buf, size_t count));
DCE(ssize_t , writev, (int fd, const struct iovec *iov, int iovcnt));
DCE(ssize_t , read, (int fd, void *buf, size_t count));
DCE(ssize_t , readv, (int fd, const struct iovec *iov, int iovcnt));
DCE(void , exit, (int status));
DCE(unsigned int , sleep, (unsigned int seconds));
DCE(int , usleep, (useconds_t usec));
DCE(pid_t , getpid, (void));
DCE(pid_t , getppid, (void));
DCE(int , pause, (void));
DCE(int , getopt, (int argc, char * const argv[], const char *optstringt));
DCE(int , getopt_long, (int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex));
DCE(uid_t , getuid, (void));
DCE(uid_t , geteuid, (void));
DCE(int , setuid, (uid_t uid));
DCE(int , setgid, (gid_t gid));
DCE(int , seteuid, (uid_t euid));
DCE(int , setegid, (gid_t egid));
DCE(int , setreuid, (uid_t ruid, uid_t euid));
DCE(int , setregid, (gid_t rgid, gid_t egid));
DCE(int , setresuid, (uid_t ruid, uid_t euid, uid_t suid));
DCE(int , setresgid, (gid_t rgid, gid_t egid, gid_t sgid));

DCE(int , isatty, (int desc));
DCE(char* , ttyname, (int fd));
DCE(char * , getcwd, (char *buf, size_t size));
DCE(char * , getwd, (char *buf));
DCE(char * , get_current_dir_name, (void));

DCE(int , chdir, (const char *path));
DCE(int , fchdir, (int fd));
DCE(int , dup, (int oldfd));
DCE(int , dup2, (int oldfd, int newfd));
DCE(int , close, (int fd));
DCE(off_t , lseek, (int fildes, off_t offset, int whence));
DCE(off64_t , lseek64, (int fildes, off64_t offset, int whence));
DCE(int , unlink, (const char *pathname));
DCE(int , rmdir, (const char *pathname));
DCE(int , access, (const char *pathname, int mode));
DCE(pid_t , fork, (void));
DCE(int , execv, (const char *path, char *const argv[]));
DCE(int , execl, (const char *path, const char *arg, ...));
DCE(int , execve, (const char *filename, char *const argv[], char *const envp[]));
DCE(int , execlp, (const char *file, const char *arg, ...));
DCE(int , execvp, (const char *file, char *const argv[]));
DCE(int , execle, (const char *path, const char *arg, ...));

DCE(int , truncate, (const char *path, off_t length));
DCE(int , ftruncate, (int fd, off_t length));
DCE(int , ftruncate64, (int fd, off_t length));

DCE(void * , sbrk, (intptr_t increment));
DCE(int , getpagesize, (void));

DCE(gid_t , getgid, (void));
DCE(gid_t , getegid, (void));
DCE(pid_t , getpgrp, (void));

DCE(int , euidaccess, (const char *pathname, int mode));
DCE(int , eaccess, (const char *pathname, int mode));

DCE(int , pipe, (int pipefd[2]));

DCE(ssize_t , pread, (int fd, void *buf, size_t count, off_t offset));
DCE(ssize_t , pwrite, (int fd, const void *buf, size_t count, off_t offset));
DCE(int , chown,(const char *path, uid_t owner, gid_t group));
DCE(int , initgroups,(const char *user, gid_t group));

DCE(int , daemon, (int nochdir, int noclose));

DCE(unsigned int , alarm, (unsigned int seconds));

DCE(ssize_t , readlink, (const char *p, char *b, size_t bufsize));
DCE(int , fsync,(int fd));


NATIVE (pathconf)
NATIVE (getdtablesize)

NATIVE (sysconf)

#ifdef __cplusplus
}
#endif

#endif /* SIMU_UNISTD_H */
