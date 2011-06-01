#define _GNU_SOURCE 1
#undef __OPTIMIZE__
#include "libc.h"
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include "dce-global-variables.h"

  // macros stolen from glibc.
#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)))


static struct Libc g_libc;

int printf(const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  int retval = g_libc.vfprintf_fn (stdout, format, ap);
  va_end (ap);
  return retval;
}
int sprintf(char *str, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  int retval = g_libc.vsprintf_fn (str, format, ap);
  va_end (ap);
  return retval;
}
int snprintf(char *str, size_t size, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  int retval = g_libc.vsnprintf_fn (str, size, format, ap);
  va_end (ap);
  return retval;
}
int fprintf(FILE *stream, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  int retval = g_libc.vfprintf_fn (stream, format, ap);
  va_end (ap);
  return retval;
}
int vprintf(const char *format, va_list ap)
{
  return g_libc.vfprintf_fn (stdout, format, ap);
}
int vsprintf(char *str, const char *format, va_list ap)
{
  return g_libc.vsprintf_fn (str, format, ap);
}
int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  return g_libc.vsnprintf_fn (str, size, format, ap);
}
int vfprintf(FILE *stream, const char *format, va_list ap)
{
  return g_libc.vfprintf_fn (stream, format, ap);
}

int open (const char *pathname, int flags, ...)
{
  va_list list;
  va_start (list, flags);
  mode_t mode = 0;
  if (flags & O_CREAT)
    {
      mode = va_arg (list, mode_t);
    }
  int fd = g_libc.open_fn (pathname, flags, mode);
  va_end (list);
  return fd;
}
int close (int fd)
{
  int retval = g_libc.close_fn (fd);
  return retval;
}
int unlink(const char *pathname)
{
  int retval = g_libc.unlink_fn (pathname);
  return retval;
}
int remove (const char *pathname)
{
  int retval = g_libc.remove_fn (pathname);
  return retval;
}
int mkdir (const char *pathname, mode_t mode)
{
  int retval = g_libc.mkdir_fn (pathname, mode);
  return retval;
}
int rmdir(const char *pathname)
{
  int retval = g_libc.rmdir_fn (pathname);
  return retval;
}
int atexit_fn (void (*fn)(void))
{
  return g_libc.atexit_fn (fn);
}
void __cxa_finalize (void *d)
{
  g_libc.__cxa_finalize_fn (d);
}

int __cxa_atexit (void (*func) (void *), void *arg, void *d)
{
  return g_libc.__cxa_atexit_fn (func, arg, d);
}

int __gxx_personality_v0 (int a, int b, 
			  unsigned c, struct _Unwind_Exception *d, struct _Unwind_Context *e)
{
  return g_libc.__gxx_personality_v0_fn (a,b,c,d,e);
}
__locale_t __newlocale (int category_mask, const char *locale, __locale_t base)
{
  return g_libc.newlocale_fn (category_mask, locale, base);
}
weak_alias (__newlocale, newlocale);
__locale_t __uselocale (__locale_t __dataset)
{
  return g_libc.uselocale_fn (__dataset);
}
weak_alias (__uselocale, uselocale);
int wctob (wint_t c)
{
  return g_libc.wctob_fn (c);
}
wint_t btowc (int c)
{
  return g_libc.btowc_fn (c);
}
wctype_t __wctype_l (__const char *__property, __locale_t __locale)
{
  return g_libc.wctype_l_fn (__property, __locale);
}
weak_alias (__wctype_l, wctype_l);
int puts (const char *s)
{
  return g_libc.fputs_fn (s, stdout);
}
int fputs (const char *s, FILE *stream)
{
  return g_libc.fputs_fn (s, stream);
}
int fputc (int c, FILE *stream)
{
  return g_libc.fputc_fn (c, stream);
}
int _IO_putc(int c, FILE *stream)
{
  return g_libc.fputc_fn (c, stream);
}
int putchar (int c)
{
  return putc (c, stdout);
}
char* fgets (char *s, int size, FILE *stream)
{
  return g_libc.fgets_fn (s, size, stream);
}
int fgetc (FILE *stream)
{
  return g_libc.fgetc_fn (stream);
}
// for the getc macro
int _IO_getc (FILE *stream)
{
  return g_libc.fgetc_fn (stream);
}
// Note: it looks like that the stdio.h header does
// not define putc and getc as macros if you include
// them from C++ so that we do need to define the putc
// and getc functions anyway. 
#undef putc
int putc(int c, FILE *stream)
{
  return g_libc.fputc_fn (c, stream);
}
#undef getc
int getc (FILE *stream)
{
  return g_libc.fgetc_fn (stream);
}
int ungetc(int c, FILE *stream)
{
  return g_libc.ungetc_fn (c, stream);
}
uint32_t htonl (uint32_t hostlong)
{
  return g_libc.htonl_fn (hostlong);
}
uint16_t htons (uint16_t hostshort)
{
  return g_libc.htons_fn (hostshort);
}
uint32_t ntohl (uint32_t netlong)
{
  return g_libc.ntohl_fn (netlong);
}
uint16_t ntohs (uint16_t netshort)
{
  return g_libc.ntohs_fn (netshort);
}
int socket (int domain, int type, int protocol)
{
  return g_libc.socket_fn (domain, type, protocol);
}
int bind (int sockfd, const struct sockaddr *my_addr, socklen_t addrlen)
{
  return g_libc.bind_fn (sockfd, my_addr, addrlen);
}
ssize_t read (int fd, void *buf, size_t count)
{
  return g_libc.read_fn (fd, buf, count);
}
int connect (int sockfd, const struct sockaddr *serv_addr,socklen_t addrlen)
{
  return g_libc.connect_fn (sockfd, serv_addr, addrlen);
}
int listen (int s, int backlog)
{
  return g_libc.listen_fn (s, backlog);
}
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  return g_libc.accept_fn (sockfd, addr, addrlen);
}
int shutdown(int s, int how)
{
  return g_libc.shutdown_fn (s, how);
}
ssize_t write (int fd, const void *buf, size_t count)
{
  return g_libc.write_fn (fd, buf, count);
}
ssize_t writev (int fd, const struct iovec *iov, int iovcnt)
{
  return g_libc.writev_fn (fd, iov, iovcnt);
}
void * memset (void *s, int c, size_t n)
{
  return g_libc.memset_fn (s,c,n);
}
void *memcpy(void *dest, const void *src, size_t n)
{
  return g_libc.memcpy_fn (dest, src, n);
}
int memcmp (const void *s1, const void *s2, size_t n)
{
  return g_libc.memcmp_fn (s1, s2, n);
}
void *memmove(void *dest, const void *src, size_t n)
{
  return g_libc.memmove_fn (dest, src, n);
}
char *strcpy (char *dest, const char *src)
{
  return g_libc.strcpy_fn (dest, src);
}
char *strncpy(char *dest, const char *src, size_t n)
{
  return g_libc.strncpy_fn (dest, src, n);
}
int strcmp(const char *s1, const char *s2)
{
  return g_libc.strcmp_fn (s1, s2);
}
int strncmp(const char *s1, const char *s2, size_t n)
{
  return g_libc.strncmp_fn (s1, s2, n);
}
size_t strlen (const char *s)
{
  return g_libc.strlen_fn (s);
}
char *strchr(const char *s, int c)
{
  return g_libc.strchr_fn (s, c);
}
char *strrchr(const char *s, int c)
{
  return g_libc.strrchr_fn (s, c);
}
int strcasecmp(const char *s1, const char *s2)
{
  return g_libc.strcasecmp_fn (s1, s2);
}
int strncasecmp(const char *s1, const char *s2, size_t n)
{
  return g_libc.strncasecmp_fn (s1, s2, n);
}
char *strdup(const char *s)
{
  return g_libc.strdup_fn (s);
}
weak_alias (strdup, __strdup);
char *strndup(const char *s, size_t n)
{
  return g_libc.strndup_fn (s, n);
}
unsigned int sleep(unsigned int seconds)
{
  return g_libc.sleep_fn (seconds);
}
int nanosleep (const struct timespec *req, struct timespec *rem)
{
  return g_libc.nanosleep_fn (req, rem);
}
long int random (void)
{
  return g_libc.random_fn ();
}
int rand (void)
{
  return g_libc.rand_fn ();
}
void srandom (unsigned int seed)
{
  return g_libc.srandom_fn (seed);
}
void srand (unsigned int seed)
{
  return g_libc.srand_fn (seed);
}
FILE *fopen (const char *path, const char *mode)
{
  return g_libc.fopen_fn (path, mode);
}
FILE *fdopen (int fildes, const char *mode)
{
  return g_libc.fdopen_fn (fildes, mode);
}
FILE *freopen (const char *path, const char *mode, FILE *stream)
{
  return g_libc.freopen_fn (path, mode, stream);
}
int fclose (FILE *fp)
{
  return g_libc.fclose_fn (fp);
}
int fcloseall (void)
{
  return g_libc.fcloseall_fn ();
}
size_t fread (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return g_libc.fread_fn (ptr, size, nmemb, stream);
}
size_t fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return g_libc.fwrite_fn (ptr, size, nmemb, stream);
}
int fflush (FILE *stream)
{
  return g_libc.fflush_fn (stream);
}
int fseek(FILE *stream, long offset, int whence)
{
  return g_libc.fseek_fn (stream, offset, whence);
}
long ftell(FILE *stream)
{
  return g_libc.ftell_fn (stream);
}
int ferror(FILE *stream)
{
  return g_libc.ferror_fn (stream);
}
int feof (FILE *stream)
{
  return g_libc.feof_fn (stream);
}
int fileno(FILE *stream)
{
  return g_libc.fileno_fn (stream);
}
void clearerr(FILE *stream)
{
  return g_libc.clearerr_fn (stream);
}
int fcntl(int fd, int cmd, ...)
{
  unsigned long arg;
  va_list list;
  va_start (list, cmd);
  arg = va_arg (list, unsigned long);
  va_end (list);
  int status = g_libc.fcntl_fn (fd, cmd, arg);
  return status;
}
void rewind(FILE *stream)
{
  return g_libc.rewind_fn (stream);
}
int fgetpos (FILE *stream, fpos_t *pos)
{
  return g_libc.fgetpos_fn (stream, pos);
}
int fsetpos (FILE *stream, const fpos_t *pos)
{
  return g_libc.fsetpos_fn (stream, pos);
}


int setvbuf (FILE *stream, char *buf, int mode, size_t size)
{
  return g_libc.setvbuf_fn (stream, buf, mode, size);
}
void setbuf(FILE *stream, char *buf)
{
  setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

void setbuffer(FILE *stream, char *buf, size_t size)
{
  setvbuf(stream, buf, buf ? _IOFBF : _IONBF, size);
}

void setlinebuf(FILE *stream)
{
  setvbuf(stream, (char *) NULL, _IOLBF, 0);
}


int *__errno_location (void)
{
  return g_libc.__errno_location_fn ();
}
int getopt(int argc, char * const argv[], const char *optstring)
{
  return g_libc.getopt_r_fn (argc, argv, optstring, &optarg, &optind, &opterr, &optopt);
}
int getopt_long(int argc, char * const argv[], const char *optstring,
    const struct option *longopts, int *longindex)
{
  return g_libc.getopt_long_r_fn (argc, argv, optstring, longopts, longindex,
    &optarg, &optind, &opterr, &optopt);
}
pid_t getpid (void)
{
  return g_libc.getpid_fn ();
}
pid_t getppid (void)
{
  return g_libc.getppid_fn ();
}
uid_t getuid(void)
{
  return g_libc.getuid_fn ();
}
uid_t geteuid(void)
{
  return g_libc.geteuid_fn ();
}
int setuid(uid_t uid)
{
  return g_libc.setuid_fn (uid);
}
int setgid(gid_t gid)
{
  return g_libc.setgid_fn (gid);
}
int seteuid(uid_t euid)
{
  return g_libc.seteuid_fn (euid);
}
int setegid(gid_t egid)
{
  return g_libc.setegid_fn (egid);
}
int setreuid(uid_t ruid, uid_t euid)
{
  return g_libc.setreuid_fn (ruid, euid);
}
int setregid(gid_t rgid, gid_t egid)
{
  return g_libc.setregid_fn (rgid, egid);
}
int setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
  return g_libc.setresuid_fn (ruid, euid, suid);
}
int setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
  return g_libc.setresgid_fn (rgid,egid,sgid);
}
int inet_aton(const char *cp, struct in_addr *inp)
{
  return g_libc.inet_aton_fn (cp, inp);
}
char *inet_ntoa(struct in_addr in)
{
  return g_libc.inet_ntoa_fn (in);
}
const char *inet_ntop (int af, const void *src,
		       char *dst, socklen_t cnt)
{
  return g_libc.inet_ntop_fn (af, src, dst, cnt);
}
in_addr_t inet_network(const char *cp)
{
  return g_libc.inet_network_fn (cp);
}
struct in_addr inet_makeaddr(in_addr_t net, in_addr_t host)
{
  return g_libc.inet_makeaddr_fn (net,host);
}
in_addr_t inet_lnaof(struct in_addr in)
{
  return g_libc.inet_lnaof_fn (in);
}
in_addr_t inet_netof(struct in_addr in)
{
  return g_libc.inet_netof_fn (in);
}
in_addr_t inet_addr(const char *cp)
{
  return g_libc.inet_addr_fn (cp);
}
int getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
  return g_libc.getsockname_fn (s, name, namelen);
}
int getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
  return g_libc.getpeername_fn (s, name, namelen);
}
int select(int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
{
  return g_libc.select_fn(nfds, readfds, writefds, exceptfds, timeout);
}
int setsockopt (int s, int level, int optname,
		const void *optval, socklen_t optlen)
{
  return g_libc.setsockopt_fn (s, level, optname, optval, optlen);
}
int getsockopt(int s, int level, int optname,
	       void *optval, socklen_t *optlen)
{
  return g_libc.getsockopt_fn (s, level, optname, optval, optlen);
}
void *calloc (size_t nmemb, size_t size)
{
  return g_libc.calloc_fn (nmemb, size);
}
void *malloc (size_t size)
{
  return g_libc.malloc_fn (size);
}
void free (void *ptr)
{
  return g_libc.free_fn (ptr);
}
void *realloc (void *ptr, size_t size)
{
  return g_libc.realloc_fn (ptr, size);
}
sighandler_t signal(int signum, sighandler_t handler)
{
  return g_libc.signal_fn (signum, handler);
}
int sigaction(int signum, const struct sigaction *act,
	      struct sigaction *oldact)
{
  return g_libc.sigaction_fn (signum, act, oldact);
}
int sigemptyset(sigset_t *set)
{
  return g_libc.sigemptyset_fn (set);
}
int sigfillset(sigset_t *set)
{
  return g_libc.sigfillset_fn (set);
}
int sigaddset(sigset_t *set, int signum)
{
  return g_libc.sigaddset_fn (set, signum);
}
int sigdelset(sigset_t *set, int signum)
{
  return g_libc.sigdelset_fn (set, signum);
}
int sigismember(const sigset_t *set, int signum)
{
  return g_libc.sigismember_fn (set, signum);
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  return g_libc.gettimeofday_fn (tv, tz);
}
time_t time(time_t *t)
{
  return g_libc.time_fn (t);
}
int isatty(int desc)
{
  return g_libc.isatty_fn (desc);
}
ssize_t send(int s, const void *buf, size_t len, int flags)
{
  return g_libc.send_fn (s, buf, len, flags);
}
ssize_t sendto(int s, const void *buf, size_t len, int flags,
	       const struct sockaddr *to, socklen_t tolen)
{
  return g_libc.sendto_fn (s, buf, len, flags, to, tolen);
}
ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
{
  return g_libc.sendmsg_fn (s, msg, flags);
}
ssize_t recv(int s, void *buf, size_t len, int flags)
{
  return g_libc.recv_fn (s, buf, len, flags);
}
ssize_t recvfrom(int s, void *buf, size_t len, int flags,
		 struct sockaddr *from, socklen_t *fromlen)
{
  return g_libc.recvfrom_fn (s, buf, len, flags, from, fromlen);
}
ssize_t recvmsg(int s, struct msghdr *msg, int flags)
{
  return g_libc.recvmsg_fn (s, msg, flags);
}
int ioctl(int d, int request, ...)
{
  va_list ap;
  va_start (ap, request);
  char *argp = va_arg (ap, char *);
  int retval = g_libc.ioctl_fn (d, request, argp);
  va_end (ap);
  return retval;
}
int sched_yield(void)
{
  return g_libc.sched_yield_fn ();
}
int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
  return g_libc.poll_fn (fds, nfds, timeout);
}
struct cmsghdr *__cmsg_nxthdr (struct msghdr *mhdr, struct cmsghdr *cmsg)
{
  return g_libc.__cmsg_nxthdr_fn (mhdr, cmsg);
}
int atoi(const char *nptr)
{
  return g_libc.atoi_fn (nptr);
}
long atol(const char *nptr)
{
  return g_libc.atol_fn (nptr);
}
long long atoll(const char *nptr)
{
  return g_libc.atoll_fn (nptr);
}
long long atoq(const char *nptr)
{
  return g_libc.atoll_fn (nptr);
}
double atof(const char *nptr)
{
  return g_libc.atof_fn (nptr);
}
int getitimer(int which, struct itimerval *value)
{
  return g_libc.getitimer_fn (which, value);
}
int setitimer(int which, const struct itimerval *value,
	      struct itimerval *ovalue)
{
  return g_libc.setitimer_fn (which, value, ovalue);
}
void exit(int status)
{
  g_libc.exit_fn (status);
  int a = 0;
  while (1)
    {
      // loop forever to quiet compiler warning:
      // warning: ‘noreturn’ function does return
      a++;
    }
}
long int strtol (const char *nptr, char **endptr, int base)
{
  return g_libc.strtol_fn (nptr, endptr, base);
}
long long int strtoll (const char *nptr, char **endptr, int base)
{
  return g_libc.strtoll_fn (nptr, endptr, base);
}
unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
  return g_libc.strtoul_fn (nptr, endptr, base);
}
unsigned long long int strtoull(const char *nptr, char **endptr,
				int base)
{
  return g_libc.strtoull_fn (nptr, endptr, base);
}

long int __strtol_internal (const char *nptr, char **endptr, int base, int group)
{
  return g_libc.strtol_fn (nptr, endptr, base);
}
double strtod (const char *nptr, char **endptr)
{
  return g_libc.strtod_fn (nptr, endptr);
}
void herror(const char *s)
{}
char *setlocale(int category, const char *locale)
{
  static char loc[] = "";
  return loc;
}
char *getcwd(char *buf, size_t size)
{
  return g_libc.getcwd_fn (buf, size);
}
char *getwd(char *buf)
{
  return g_libc.getwd_fn (buf);
}

char *get_current_dir_name(void)
{
  return g_libc.get_current_dir_name_fn ();
}
int chdir(const char *path)
{
  return g_libc.chdir_fn (path);
}
int fchdir(int fd)
{
  return g_libc.fchdir_fn (fd);
}
int pthread_create (pthread_t *thread,
		    const pthread_attr_t *attr,
		    void *(*start_routine)(void*), void * arg)
{
  return g_libc.pthread_create_fn (thread, attr, start_routine, arg);
}
void pthread_exit(void *retval)
{
  return g_libc.pthread_exit_fn (retval);
}
pthread_t pthread_self (void)
{
  return g_libc.pthread_self_fn ();
}
int pthread_cancel (pthread_t thread)
{
  return g_libc.pthread_cancel_fn (thread);
}
int pthread_mutex_destroy (pthread_mutex_t *mutex)
{
  return g_libc.pthread_mutex_destroy_fn (mutex);
}
int pthread_mutex_init (pthread_mutex_t * mutex,
			const pthread_mutexattr_t * attr)
{
  return g_libc.pthread_mutex_init_fn (mutex, attr);
}
int pthread_mutex_lock (pthread_mutex_t *mutex)
{
  return g_libc.pthread_mutex_lock_fn (mutex);
}
int pthread_mutex_unlock (pthread_mutex_t *mutex)
{
  return g_libc.pthread_mutex_unlock_fn (mutex);
}
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  return g_libc.pthread_mutex_trylock_fn (mutex);
}
int pthread_mutexattr_init (pthread_mutexattr_t *attribute)
{
  return g_libc.pthread_mutexattr_init_fn (attribute);
}
int pthread_mutexattr_destroy (pthread_mutexattr_t *attribute)
{
  return g_libc.pthread_mutexattr_destroy_fn (attribute);
}
int pthread_mutexattr_settype (pthread_mutexattr_t *attribute, int kind)
{
  return g_libc.pthread_mutexattr_settype_fn (attribute, kind);
}
int pthread_once (pthread_once_t *once_control, void (*init_routine)(void))
{
  return g_libc.pthread_once_fn (once_control, init_routine);
}
void *pthread_getspecific (pthread_key_t key)
{
  return g_libc.pthread_getspecific_fn (key);
}
int pthread_setspecific (pthread_key_t key, const void *value)
{
  return g_libc.pthread_setspecific_fn (key, value);
}
int pthread_key_create (pthread_key_t *key, void (*destructor)(void*))
{
  return g_libc.pthread_key_create_fn (key, destructor);
}
int pthread_key_delete (pthread_key_t key)
{
  return g_libc.pthread_key_delete_fn (key);
}
int pthread_kill (pthread_t thread, int sig)
{
  return g_libc.pthread_kill_fn (thread, sig);
}
int pthread_join (pthread_t thread_handle, void **value_ptr)
{
  return g_libc.pthread_join_fn (thread_handle, value_ptr);
}
int pthread_detach(pthread_t thread)
{
  return g_libc.pthread_detach_fn (thread);
}
int __xstat (int ver, const char *filename,
	     struct stat *stat_buf)
{
  return g_libc.xstat_fn (ver, filename, stat_buf);
}
int __fxstat (int ver, int fd,
	      struct stat *stat_buf)
{
  return g_libc.fxstat_fn (ver, fd, stat_buf);
}
int __lxstat (int ver, const char *pathname,
	      struct stat *stat_buf)
{
  return g_libc.lxstat_fn (ver, pathname, stat_buf);
}
int __xstat64 (int ver, const char *filename,
	       struct stat64 *stat_buf)
{
  return g_libc.xstat64_fn (ver, filename, stat_buf);
}
int __fxstat64 (int ver, int fd,
		struct stat64 *stat_buf)
{
  return g_libc.fxstat64_fn (ver, fd, stat_buf);
}
int __lxstat64 (int ver, const char *pathname,
		struct stat64 *stat_buf)
{
  return g_libc.lxstat64_fn (ver, pathname, stat_buf);
}

int open64 (const char *file, int oflag, ...)
{
  va_list list;
  va_start (list, oflag);
  mode_t mode = 0;
  if (oflag & O_CREAT)
    {
      mode = va_arg (list, mode_t);
    }
  int fd = g_libc.open_fn (file, oflag, mode);
  va_end (list);
  return fd;
}
void *
mmap64 (void *addr, size_t len, int prot, int flags, int fd, off64_t offset)
{
  return g_libc.mmap64_fn (addr, len, prot, flags, fd, offset);
}
void *
mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  return g_libc.mmap64_fn (addr, len, prot, flags, fd, offset);
}
int munmap(void *start, size_t length)
{
  return g_libc.munmap_fn (start, length);
}
int dup(int oldfd)
{
  return g_libc.dup_fn (oldfd);
}
int dup2(int oldfd, int newfd)
{
  return g_libc.dup2_fn (oldfd, newfd);
}
struct tm *localtime(const time_t *timep)
{
  return g_libc.localtime_fn (timep);
}
size_t strftime(char *s, size_t max, const char *format,
		const struct tm *tm)
{
  return g_libc.strftime_fn (s, max, format, tm);
}
const unsigned short int **__ctype_b_loc (void)
{
  return g_libc.__ctype_b_loc_fn ();
}
char *strerror (int errnum)
{
  return g_libc.strerror_fn (errnum);
}
int __xpg_strerror_r (int errnum, char *buf, size_t buflen)
{
  return g_libc.xsi_strerror_r_fn (errnum, buf, buflen);
}
char * strerror_r(int errnum, char *buf, size_t buflen)
{
  return g_libc.gnu_strerror_r_fn (errnum, buf, buflen);
}
void perror(const char *s)
{
  fprintf (stderr, "%s: %s\n", s, strerror (*(g_libc.__errno_location_fn ())));
}
struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
  return g_libc.gmtime_r_fn (timep, result);
}
struct tm *gmtime(const time_t *timep)
{
  static struct tm result;
  return g_libc.gmtime_r_fn (timep, &result);
}
int sem_init(sem_t *sem, int pshared, unsigned int value)
{
  return g_libc.sem_init_fn (sem, pshared, value);
}
int sem_destroy(sem_t *sem)
{
  return g_libc.sem_destroy_fn (sem);
}
int sem_post(sem_t *sem)
{
  return g_libc.sem_post_fn (sem);
}
int sem_wait(sem_t *sem)
{
  return g_libc.sem_wait_fn (sem);
}
int sem_trywait(sem_t *sem)
{
  return g_libc.sem_trywait_fn (sem);
}
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
  return g_libc.sem_timedwait_fn (sem, abs_timeout);
}
int sem_getvalue(sem_t *sem, int *sval)
{
  return g_libc.sem_getvalue_fn (sem, sval);
}
int pthread_cond_destroy (pthread_cond_t *cond)
{
  return g_libc.pthread_cond_destroy_fn (cond);
}
int pthread_cond_init (pthread_cond_t *cond,
		       const pthread_condattr_t *attr)
{
  return g_libc.pthread_cond_init_fn (cond, attr);
}
int pthread_cond_broadcast (pthread_cond_t *cond)
{
  return g_libc.pthread_cond_broadcast_fn (cond);
}
int pthread_cond_signal (pthread_cond_t *cond)
{
  return g_libc.pthread_cond_signal_fn (cond);
}
int pthread_cond_timedwait(pthread_cond_t * cond,
			   pthread_mutex_t * mutex,
			   const struct timespec * abstime)
{
  return g_libc.pthread_cond_timedwait_fn (cond, mutex, abstime);
}
int pthread_cond_wait(pthread_cond_t * cond,
		      pthread_mutex_t * mutex)
{
  return g_libc.pthread_cond_wait_fn (cond, mutex);
}
int pthread_condattr_destroy(pthread_condattr_t *attr)
{
  return g_libc.pthread_condattr_destroy_fn (attr);
}
int pthread_condattr_init(pthread_condattr_t *attr)
{
  return g_libc.pthread_condattr_init_fn (attr);
}

struct hostent *gethostbyname (const char *name)
{
  return g_libc.gethostbyname_fn (name);
}
struct hostent *gethostbyname2(const char *name, int af)
{
  return g_libc.gethostbyname2_fn (name, af);
}
int getaddrinfo(const char *node, const char *service,
		const struct addrinfo *hints,
		struct addrinfo **res)
{
  return g_libc.getaddrinfo_fn (node, service, hints, res);
}

void freeaddrinfo(struct addrinfo *res)
{
  return g_libc.freeaddrinfo_fn (res);
}

const char *gai_strerror(int errcode)
{
  return g_libc.strerror_fn (errcode);
}

char *getenv (const char *name)
{
  return g_libc.getenv_fn (name);
}
int putenv(char *string)
{
  return g_libc.putenv_fn (string);
}
int setenv(const char *name, const char *value, int overwrite)
{
  return g_libc.setenv_fn (name, value, overwrite);
}
int unsetenv(const char *name)
{
  return g_libc.unsetenv_fn (name);
}
int clearenv(void)
{
  return g_libc.clearenv_fn ();
}
int toupper(int c)
{
  return g_libc.toupper_fn (c);
}
int tolower(int c)
{
  return g_libc.tolower_fn (c);
}


int timerfd_create (int clockid, int flags)
{
  return g_libc.timerfd_create_fn (clockid, flags);
}
int timerfd_settime(int fd, int flags,
			 const struct itimerspec *new_value,
			 struct itimerspec *old_value)
{
  return g_libc.timerfd_settime_fn (fd, flags, new_value, old_value);
}
int timerfd_gettime (int fd, struct itimerspec *cur_value)
{
  return g_libc.timerfd_gettime_fn (fd, cur_value);
}

unsigned if_nametoindex (const char *ifname)
{
  return g_libc.if_nametoindex_fn (ifname);
}
pid_t fork (void)
{
  return g_libc.fork_fn ();
}

unsigned short int *seed48 (unsigned short int seed16v[3])
{
  return g_libc.seed48_fn (seed16v);
}

double drand48 (void)
{
  return g_libc.drand48_fn ();
}

long int nrand48 (unsigned short int xsubi[3])
{
  return g_libc.nrand48_fn (xsubi);
}

long int lrand48 (void)
{
  return g_libc.lrand48_fn ();
}

long int mrand48 (void)
{
  return g_libc.mrand48_fn ();
}

double erand48(unsigned short xsubi[3])
{
  return g_libc.erand48_fn (xsubi);
}

long int jrand48 (unsigned short int xsubi[3])
{
  return g_libc.jrand48_fn (xsubi);
}

void srand48(long int seedval)
{
  return g_libc.srand48_fn (seedval);
}

void lcong48(unsigned short param[7])
{
  return g_libc.lcong48_fn (param);
}

int drand48_r(struct drand48_data *buffer, double *result)
{
  return g_libc.drand48_r_fn (buffer, result);
}

int erand48_r(unsigned short xsubi[3], struct drand48_data *buffer, double *result)
{
  return g_libc.erand48_r_fn ( xsubi, buffer, result);
}

int lrand48_r(struct drand48_data *buffer, long int *result)
{
  return g_libc.lrand48_r_fn ( buffer, result);
}

int nrand48_r(unsigned short int xsubi[3], struct drand48_data *buffer, long int *result)
{
  return g_libc.nrand48_r_fn ( xsubi, buffer, result);
}

int mrand48_r(struct drand48_data *buffer,long int *result)
{
  return g_libc.mrand48_r_fn ( buffer, result);
}

int jrand48_r(unsigned short int xsubi[3], struct drand48_data *buffer, long int *result)
{
  return g_libc.jrand48_r_fn ( xsubi, buffer, result);
}

int srand48_r(long int seedval, struct drand48_data *buffer)
{
  return g_libc.srand48_r_fn ( seedval, buffer);
}

int seed48_r(unsigned short int seed16v[3], struct drand48_data *buffer)
{
  return g_libc.seed48_r_fn ( seed16v, buffer);
}

int lcong48_r(unsigned short int param[7], struct drand48_data *buffer)
{
  return g_libc.lcong48_r_fn( param, buffer);
}

char *strcat(char *dest, const char *src)
{
  return g_libc.strcat_fn ( dest, src);
}

char *strncat(char *dest, const char *src, size_t n)
{
  return g_libc.strncat_fn ( dest, src, n);
}

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *))
{
  g_libc.qsort_fn ( base, nmemb, size, compar);
}

mode_t umask (mode_t mask)
{
  return g_libc.umask_fn ( mask);
}

void abort(void)
{
  g_libc.abort_fn ();
  int a = 0;
  while (1)
    {
      // loop forever to quiet compiler warning:
      // warning: ‘noreturn’ function does return
      a++;
    }
}

int32_t * *
__ctype_tolower_loc (void)
{
  return g_libc.ctype_tolower_loc_fn ();
}

void LIBSETUP (const struct Libc *fn)
{
  /* The following assignment of fn to g_libc is a bit weird: we perform a copy of the data 
   * structures by hand rather than executing g_libc = fn. This is obviously done on purpose. 
   * The reason is that g_libc = fn would trigger a call to the memcpy function because the 
   * Libc structure is very big. The memcpy function is resolved through the dynamic loader's 
   * symbol lookup mechanism to the local memcpy function and that local memcpy function happens 
   * to be calling g_libc.memcpy_fn which is set to NULL before the data structure is initialized. 
   */
  const uint8_t *src = (const uint8_t *)fn;
  uint8_t *dst = (uint8_t *)&g_libc;
  uint32_t i;
  for (i = 0; i < sizeof (struct Libc); ++i)
    {
      *dst = *src;
      src++;
      dst++;
    }
  struct SimuGlobalVariables globals;
  globals.pstdin = &stdin;
  globals.pstdout = &stdout;
  globals.pstderr = &stderr;
  globals.penvp = &environ;
  g_libc.global_variables_setup (&globals);
}


