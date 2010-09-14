#ifndef SIMU_UNISTD_H
#define SIMU_UNISTD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t simu_write (int fd, const void *buf, size_t count);
ssize_t simu_writev (int fd, const struct iovec *iov, int iovcnt);
ssize_t simu_read (int fd, void *buf, size_t count);
void simu_exit (int status);
unsigned int simu_sleep(unsigned int seconds);
pid_t simu_getpid (void);
pid_t simu_getppid (void);
int simu_pause (void);
int simu_getopt_r (int argc, char * const argv[], const char *optstring, 
		   char **poptarg, int *poptind, int *popterr, int *poptopt);
int simu_getopt_long_r (int argc, char * const argv[], const char *optstring, 
		   const struct option *longopts, int *longindex,
		   char **poptarg, int *poptind, int *popterr, int *poptopt);
uid_t simu_getuid(void);
uid_t simu_geteuid(void);
int simu_setuid(uid_t uid);
int simu_setgid(gid_t gid);
int simu_seteuid(uid_t euid);
int simu_setegid(gid_t egid);
int simu_setreuid(uid_t ruid, uid_t euid);
int simu_setregid(gid_t rgid, gid_t egid);
int simu_setresuid(uid_t ruid, uid_t euid, uid_t suid);
int simu_setresgid(gid_t rgid, gid_t egid, gid_t sgid);

int simu_isatty(int desc);
char *simu_getcwd (char *buf, size_t size);
char *simu_getwd (char *buf);
char *simu_get_current_dir_name (void);

int simu_chdir(const char *path);
int simu_fchdir(int fd);
int simu_dup(int oldfd);
int simu_dup2(int oldfd, int newfd);
int simu_close (int fd);
off_t simu_lseek(int fildes, off_t offset, int whence);
off64_t simu_lseek64(int fildes, off64_t offset, int whence);
int simu_unlink (const char *pathname);
int simu_rmdir(const char *pathname);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_UNISTD_H */
