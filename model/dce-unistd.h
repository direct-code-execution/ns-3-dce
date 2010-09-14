#ifndef SIMU_UNISTD_H
#define SIMU_UNISTD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t dce_write (int fd, const void *buf, size_t count);
ssize_t dce_writev (int fd, const struct iovec *iov, int iovcnt);
ssize_t dce_read (int fd, void *buf, size_t count);
void dce_exit (int status);
unsigned int dce_sleep(unsigned int seconds);
pid_t dce_getpid (void);
pid_t dce_getppid (void);
int dce_pause (void);
int dce_getopt_r (int argc, char * const argv[], const char *optstring, 
		   char **poptarg, int *poptind, int *popterr, int *poptopt);
int dce_getopt_long_r (int argc, char * const argv[], const char *optstring, 
		   const struct option *longopts, int *longindex,
		   char **poptarg, int *poptind, int *popterr, int *poptopt);
uid_t dce_getuid(void);
uid_t dce_geteuid(void);
int dce_setuid(uid_t uid);
int dce_setgid(gid_t gid);
int dce_seteuid(uid_t euid);
int dce_setegid(gid_t egid);
int dce_setreuid(uid_t ruid, uid_t euid);
int dce_setregid(gid_t rgid, gid_t egid);
int dce_setresuid(uid_t ruid, uid_t euid, uid_t suid);
int dce_setresgid(gid_t rgid, gid_t egid, gid_t sgid);

int dce_isatty(int desc);
char *dce_getcwd (char *buf, size_t size);
char *dce_getwd (char *buf);
char *dce_get_current_dir_name (void);

int dce_chdir(const char *path);
int dce_fchdir(int fd);
int dce_dup(int oldfd);
int dce_dup2(int oldfd, int newfd);
int dce_close (int fd);
off_t dce_lseek(int fildes, off_t offset, int whence);
off64_t dce_lseek64(int fildes, off64_t offset, int whence);
int dce_unlink (const char *pathname);
int dce_rmdir(const char *pathname);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_UNISTD_H */
