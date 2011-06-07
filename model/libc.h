
#ifndef LIBC_H
#define LIBC_H
#define _LARGEFILE64_SOURCE 1
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <poll.h>
#include <semaphore.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat;
struct stat64;
struct _Unwind_Context;
struct _Unwind_Exception;
struct timezone;
struct itimerval;
struct sigaction;
struct option;
struct addrinfo;
struct itimerspec;
struct SimuGlobalVariables;

struct Libc
{
  void (*global_variables_setup) (struct SimuGlobalVariables *globals);
  int (*vfprintf_fn) (FILE *, const char *, va_list);
  int (*vsprintf_fn) (char *, const char *, va_list);
  int (*vsnprintf_fn) (char *, size_t, const char *, va_list);
  void *(*mmap64_fn) (void *addr, size_t len, int prot, int flags, int fd, off64_t offset);
  int (*munmap_fn) (void *start, size_t length);

  int (*xstat_fn) (int, const char *, struct stat *);
  int (*fxstat_fn) (int, int , struct stat *);
  int (*lxstat_fn) (int, const char *, struct stat *);

  int (*xstat64_fn) (int, const char *, struct stat64 *);
  int (*fxstat64_fn) (int, int , struct stat64 *);
  int (*lxstat64_fn) (int, const char *, struct stat64 *);

  int (*open_fn) (const char *, int, mode_t );
  int (*close_fn) (int);
  int (*fcntl_fn)(int fd, int cmd, unsigned long arg);
  int (*unlink_fn) (const char *);
  int (*remove_fn) (const char *);
  int (*mkdir_fn) (const char *pathname, mode_t mode);
  int (*rmdir_fn) (const char *pathname);
  int (*dup_fn) (int);
  int (*dup2_fn) (int, int);
  int (*atexit_fn) (void (*)(void));
  void (*__cxa_finalize_fn) (void *d);
  int (*__cxa_atexit_fn) (void (*func) (void *), void *arg, void *d);
  int (*__gxx_personality_v0_fn) (int a, int b, 
				  unsigned c, struct _Unwind_Exception *d, struct _Unwind_Context *e);
  __locale_t (*newlocale_fn) (int category_mask, const char *locale, __locale_t base);
  __locale_t (*uselocale_fn) (__locale_t __dataset);
  int (*wctob_fn) (wint_t c);
  wint_t (*btowc_fn) (int c);
  wctype_t (*wctype_l_fn) (__const char *__property, __locale_t __locale);

  uint32_t (*htonl_fn) (uint32_t hostlong);
  uint16_t (*htons_fn) (uint16_t hostshort);
  uint32_t (*ntohl_fn) (uint32_t netlong);
  uint16_t (*ntohs_fn) (uint16_t netshort);
  int (*socket_fn) (int domain, int type, int protocol);
  int (*getsockname_fn) (int s, struct sockaddr *name, socklen_t *namelen);
  int (*getpeername_fn) (int s, struct sockaddr *name, socklen_t *namelen);
  int (*bind_fn) (int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);
  ssize_t (*read_fn) (int fd, void *buf, size_t count);
  int (*connect_fn) (int sockfd, const struct sockaddr *serv_addr,socklen_t addrlen);
  ssize_t (*write_fn) (int fd, const void *buf, size_t count);
  ssize_t (*writev_fn) (int fd, const struct iovec *iov, int iovcnt);
  int (*setsockopt_fn) (int s, int level, int optname,
			const void *optval, socklen_t optlen);
  int (*getsockopt_fn) (int s, int level, int optname,
			void *optval, socklen_t *optlen);
  int (*select_fn)(int nfds, fd_set *readfds, fd_set *writefds,
                     fd_set *exceptfds, struct timeval *timeout);
  int (*listen_fn) (int s, int backlog);
  int (*accept_fn) (int sockfd, struct sockaddr *addr, socklen_t *addrlen);
  int (*shutdown_fn) (int s, int how);
  void * (*memset_fn) (void *s, int c, size_t n);
  void * (*memcpy_fn) (void *dest, const void *src, size_t n);
  int (*memcmp_fn) (const void *s1, const void *s2, size_t n);
  void *(*memmove_fn) (void *dest, const void *src, size_t n);
  char *(*strcpy_fn) (char *dest, const char *src);
  char *(*strcat_fn) (char *dest, const char *src);
  char *(*strncat_fn) (char *dest, const char *src, size_t n);
  char *(*strncpy_fn) (char *dest, const char *src, size_t n);
  int (*strcmp_fn)(const char *s1, const char *s2);
  int (*strncmp_fn) (const char *s1, const char *s2, size_t n);
  size_t (*strlen_fn) (const char *s);
  char *(*strchr_fn) (const char *s, int c);
  char *(*strrchr_fn) (const char *s, int c);
  int (*strcasecmp_fn) (const char *s1, const char *s2);
  int (*strncasecmp_fn) (const char *s1, const char *s2, size_t n);
  char *(*strdup_fn) (const char *s);
  char *(*strndup_fn) (const char *s, size_t n);
  unsigned int (*sleep_fn) (unsigned int seconds);
  int (*nanosleep_fn) (const struct timespec *req, struct timespec *rem);

  long int (*random_fn)(void);
  int (*rand_fn)(void);
  void (*srandom_fn)(unsigned int seed);
  void (*srand_fn)(unsigned int seed);
  unsigned short int * (*seed48_fn) (unsigned short int seed16v[3]);
  double (*drand48_fn) (void);
  long int (*nrand48_fn) (unsigned short int xsubi[3]);
  long int (*lrand48_fn) (void);
  long int (*mrand48_fn) (void);
  double (*erand48_fn) (unsigned short xsubi[3]);
  long int (*jrand48_fn) (unsigned short int xsubi[3]);
  void (*srand48_fn) (long int seedval);
  void (*lcong48_fn) (unsigned short param[7]);
  int (*drand48_r_fn) (struct drand48_data *buffer, double *result);
  int (*erand48_r_fn) (unsigned short xsubi[3], struct drand48_data *buffer, double *result);
  int (*lrand48_r_fn) (struct drand48_data *buffer, long int *result);
  int (*nrand48_r_fn) (unsigned short int xsubi[3], struct drand48_data *buffer, long int *result);
  int (*mrand48_r_fn) (struct drand48_data *buffer,long int *result);
  int (*jrand48_r_fn) (unsigned short int xsubi[3], struct drand48_data *buffer, long int *result);
  int (*srand48_r_fn) (long int seedval, struct drand48_data *buffer);
  int (*seed48_r_fn) (unsigned short int seed16v[3], struct drand48_data *buffer);
  int (*lcong48_r_fn) (unsigned short int param[7], struct drand48_data *buffer);

  FILE *(*fopen_fn) (const char *path, const char *mode);
  FILE *(*fdopen_fn) (int fildes, const char *mode);
  FILE *(*freopen_fn) (const char *path, const char *mode, FILE *stream);
  int (*fclose_fn) (FILE *fp);
  int (*fcloseall_fn) (void);
  size_t (*fread_fn) (void *ptr, size_t size, size_t nmemb, FILE *stream);
  size_t (*fwrite_fn) (const void *ptr, size_t size, size_t nmemb,
		       FILE *stream);
  int (*fputs_fn) (const char *, FILE *);
  int (*fputc_fn) (int, FILE *);
  char* (*fgets_fn) (char *, int, FILE *);
  int (*fgetc_fn) (FILE *);
  int (*ungetc_fn) (int c, FILE *stream);

  int (*fflush_fn) (FILE *stream);
  int (*fseek_fn)(FILE *stream, long offset, int whence);
  long (*ftell_fn)(FILE *stream);
  void (*rewind_fn)(FILE *stream);
  int (*fgetpos_fn) (FILE *stream, fpos_t *pos);
  int (*fsetpos_fn) (FILE *stream, const fpos_t *pos);

  int (*ferror_fn)(FILE *stream);
  int (*feof_fn)(FILE *stream);
  int (*fileno_fn) (FILE *stream);
  void (*clearerr_fn) (FILE *stream);


  int (*setvbuf_fn) (FILE *stream, char *buf, int mode, size_t size);


  int *(*__errno_location_fn) (void);
  int (*getopt_r_fn) (int argc, char * const argv[], const char *optstring, 
		      char **poptarg, int *poptind, int *popterr, int *poptopt);
  int (*getopt_long_r_fn) (int argc, char * const argv[], const char *optstring, 
      const struct option *longopts, int *longindex,
      char **poptarg, int *poptind, int *popterr, int *poptopt);
  pid_t (*getpid_fn) (void);
  pid_t (*getppid_fn) (void);
  uid_t (*getuid_fn) (void);
  uid_t (*geteuid_fn) (void);
  int (*setuid_fn) (uid_t uid);
  int (*setgid_fn) (gid_t gid);
  int (*seteuid_fn) (uid_t euid);
  int (*setegid_fn) (gid_t egid);
  int (*setreuid_fn) (uid_t ruid, uid_t euid);
  int (*setregid_fn) (gid_t rgid, gid_t egid);
  int (*setresuid_fn) (uid_t ruid, uid_t euid, uid_t suid);
  int (*setresgid_fn) (gid_t rgid, gid_t egid, gid_t sgid);

  int (*inet_aton_fn) (const char *cp, struct in_addr *inp);
  char *(*inet_ntoa_fn) (struct in_addr in);
  const char *(*inet_ntop_fn) (int af, const void *src,
			       char *dst, socklen_t cnt);
  in_addr_t (*inet_network_fn) (const char *cp);
  struct in_addr (*inet_makeaddr_fn) (in_addr_t net, in_addr_t host);
  in_addr_t (*inet_lnaof_fn)(struct in_addr in);
  in_addr_t (*inet_netof_fn)(struct in_addr in);
  in_addr_t (*inet_addr_fn) (const char *cp);

  void *(*calloc_fn) (size_t nmemb, size_t size);
  void *(*malloc_fn) (size_t size);
  void (*free_fn) (void *ptr);
  void *(*realloc_fn) (void *ptr, size_t size);
  int (*gettimeofday_fn) (struct timeval *tv, struct timezone *tz);
  time_t (*time_fn) (time_t *t);
  int (*isatty_fn) (int desc);
  ssize_t (*send_fn)(int s, const void *buf, size_t len, int flags);
  ssize_t (*sendto_fn)(int s, const void *buf, size_t len, int flags,
		       const struct sockaddr *to, socklen_t tolen);
  ssize_t (*sendmsg_fn)(int s, const struct msghdr *msg, int flags);
  ssize_t (*recv_fn) (int s, void *buf, size_t len, int flags);
  ssize_t (*recvfrom_fn) (int s, void *buf, size_t len, int flags,
			  struct sockaddr *from, socklen_t *fromlen);
  ssize_t (*recvmsg_fn) (int s, struct msghdr *msg, int flags);
  int (*ioctl_fn) (int d, int request, char *argp);
  int (*sched_yield_fn) (void);
  int (*poll_fn) (struct pollfd *fds, nfds_t nfds, int timeout);
  struct cmsghdr *(*__cmsg_nxthdr_fn) (struct msghdr *mhdr, struct cmsghdr *cmsg);
  int (*atoi_fn) (const char *nptr);
  long (*atol_fn) (const char *nptr);
  long long (*atoll_fn) (const char *nptr);
  double (*atof_fn) (const char *nptr);
  int (*getitimer_fn)(int which, struct itimerval *value);
  int (*setitimer_fn)(int which, const struct itimerval *value,
		      struct itimerval *ovalue);
  void (*exit_fn) (int status);
  sighandler_t (*signal_fn) (int signum, sighandler_t handler);
  int (*sigaction_fn) (int signum, const struct sigaction *act,
		       struct sigaction *oldact);
  int (*sigemptyset_fn) (sigset_t *set);
  int (*sigfillset_fn) (sigset_t *set);
  int (*sigaddset_fn) (sigset_t *set, int signum);
  int (*sigdelset_fn) (sigset_t *set, int signum);
  int (*sigismember_fn) (const sigset_t *set, int signum);
  long int (*strtol_fn) (const char *nptr, char **endptr, int base);
  long long int (*strtoll_fn) (const char *nptr, char **endptr, int base);
  unsigned long int (*strtoul_fn) (const char *nptr, char **endptr, int base);
  unsigned long long int (*strtoull_fn) (const char *nptr, char **endptr, int base);
  double (*strtod_fn) (const char *nptr, char **endptr);
  char *(*getcwd_fn) (char *buf, size_t size);
  char *(*getwd_fn) (char *buf);
  char *(*get_current_dir_name_fn) (void);
  int (*chdir_fn) (const char *path);
  int (*fchdir_fn) (int fd);
  struct tm *(*localtime_fn)(const time_t *timep);
  size_t (*strftime_fn) (char *s, size_t max, const char *format,
			 const struct tm *tm);
  const unsigned short int **(*__ctype_b_loc_fn) (void);
  char *(*strerror_fn) (int errnum);
  int (*xsi_strerror_r_fn)(int errnum, char *buf, size_t buflen);
  char *(*gnu_strerror_r_fn)(int errnum, char *buf, size_t buflen);

  int (*pthread_create_fn) (pthread_t *thread,
			    const pthread_attr_t *attr,
			    void *(*start_routine)(void*), void * arg);
  void (*pthread_exit_fn) (void *retval);
  pthread_t (*pthread_self_fn) (void);
  int (*pthread_mutex_destroy_fn) (pthread_mutex_t *mutex);
  int (*pthread_mutex_init_fn) (pthread_mutex_t * mutex,
				const pthread_mutexattr_t * attr);
  int (*pthread_mutex_lock_fn) (pthread_mutex_t *mutex);
  int (*pthread_mutex_unlock_fn) (pthread_mutex_t *mutex);
  int (*pthread_mutex_trylock_fn) (pthread_mutex_t *mutex);
  int (*pthread_mutexattr_init_fn) (pthread_mutexattr_t *attribute);
  int (*pthread_mutexattr_destroy_fn) (pthread_mutexattr_t *attribute);
  int (*pthread_mutexattr_settype_fn) (pthread_mutexattr_t *attribute, int kind);
  int (*pthread_key_create_fn) (pthread_key_t *key, void (*destructor)(void*));
  int (*pthread_key_delete_fn) (pthread_key_t key);
  int (*pthread_setspecific_fn) (pthread_key_t key, const void *value);
  void *(*pthread_getspecific_fn) (pthread_key_t key);
  int (*pthread_once_fn) (pthread_once_t *, void (*)(void));
  int (*pthread_cancel_fn) (pthread_t thread);
  int (*pthread_kill_fn) (pthread_t thread, int sig);
  int (*pthread_join_fn) (pthread_t thread_handle, void **value_ptr);
  int (*pthread_detach_fn) (pthread_t thread);
  int (*sem_init_fn)(sem_t *sem, int pshared, unsigned int value);
  int (*sem_destroy_fn)(sem_t *sem);
  int (*sem_post_fn)(sem_t *sem);
  int (*sem_wait_fn)(sem_t *sem);
  int (*sem_trywait_fn)(sem_t *sem);
  int (*sem_timedwait_fn)(sem_t *sem, const struct timespec *abs_timeout);
  int (*sem_getvalue_fn)(sem_t *sem, int *sval);
  int (*pthread_cond_destroy_fn) (pthread_cond_t *cond);
  int (*pthread_cond_init_fn) (pthread_cond_t *cond,
			       const pthread_condattr_t *attr);
  int (*pthread_cond_broadcast_fn) (pthread_cond_t *cond);
  int (*pthread_cond_signal_fn) (pthread_cond_t *cond);
  int (*pthread_cond_timedwait_fn)(pthread_cond_t * cond,
				   pthread_mutex_t * mutex,
				   const struct timespec * abstime);
  int (*pthread_cond_wait_fn)(pthread_cond_t * cond,
			      pthread_mutex_t * mutex);
  int (*pthread_condattr_destroy_fn)(pthread_condattr_t *attr);
  int (*pthread_condattr_init_fn)(pthread_condattr_t *attr);

  struct tm *(*gmtime_r_fn)(const time_t *timep, struct tm *result);

  struct hostent *(*gethostbyname_fn) (const char *name);
  struct hostent *(*gethostbyname2_fn) (const char *name, int af);
  int (*getaddrinfo_fn) (const char *node, const char *service,
			 const struct addrinfo *hints,
			 struct addrinfo **res);
  void (*freeaddrinfo_fn) (struct addrinfo *res);
  const char *(*gai_strerror_fn)(int errcode);
  char *(*getenv_fn) (const char *name);
  int (*putenv_fn)(char *string);
  int (*setenv_fn)(const char *name, const char *value, int overwrite);
  int (*unsetenv_fn)(const char *name);
  int (*clearenv_fn)(void);
  int (*toupper_fn)(int c);
  int (*tolower_fn)(int c);


  int (*timerfd_create_fn) (int clockid, int flags);
  int (*timerfd_settime_fn)(int fd, int flags,
			 const struct itimerspec *new_value,
			 struct itimerspec *old_value);
  int (*timerfd_gettime_fn) (int fd, struct itimerspec *cur_value);

  unsigned (*if_nametoindex_fn) (const char *ifname);

  pid_t (*fork_fn) (void);

  void (*qsort_fn) (void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

  mode_t (*umask_fn) (mode_t mask);

  void (*abort_fn) (void);

  int32_t * * (*ctype_tolower_loc_fn) (void);

  char * (*ctime_fn) (const time_t *timep);

};

#ifdef __cplusplus
}
#endif

#endif /* LIBC_H */
