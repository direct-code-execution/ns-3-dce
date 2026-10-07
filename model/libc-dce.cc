#define _GNU_SOURCE 1
#undef __OPTIMIZE__
#define _LARGEFILE64_SOURCE 1

#include "libc-dce.h"
#include "libc.h"

#include "arpa/dce-inet.h"
#include "sys/dce-socket.h"
#include "sys/dce-time.h"
#include "sys/dce-ioctl.h"
#include "sys/dce-mman.h"
#include "sys/dce-stat.h"
#include "sys/dce-select.h"
#include "sys/dce-timerfd.h"
#include "dce-unistd.h"
#include "dce-netdb.h"
#include "dce-pthread.h"
#include "dce-stdio.h"
#include "dce-stdarg.h"
#include "dce-errno.h"
#include "dce-libc-private.h"
#include "dce-fcntl.h"
#include "dce-epoll.h"
#include "dce-futex.h"
#include "dce-compat.h"
#include "dce-sched.h"
#include "dce-poll.h"
#include "dce-signal.h"
#include "dce-stdlib.h"
#include "dce-time.h"
#include "dce-semaphore.h"
#include "dce-cxa.h"
#include "dce-string.h"
#include "dce-global-variables.h"
#include "dce-random.h"
#include "dce-umask.h"
#include "dce-misc.h"
#include "dce-wait.h"
#include "dce-locale.h"
#include "net/dce-if.h"
#include "dce-syslog.h"
#include "dce-pwd.h"
#include "dce-dirent.h"
#include "dce-vfs.h"
#include "dce-termio.h"
#include "dce-dl.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <ifaddrs.h>
#include <sys/uio.h>
#include <libgen.h>
#include <locale.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/auxv.h>
#include <sys/xattr.h>
#include <sys/statvfs.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <spawn.h>
#include <mntent.h>
#include <fts.h>
#include <resolv.h>
#include <netdb.h>
#include <sched.h>
#include <wchar.h>
#include <locale.h>
#include <libintl.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <pthread.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <errno.h>
#include <setjmp.h>
#include <libintl.h>
#include <pwd.h>
#include <inttypes.h>
#include <error.h>
#include <netinet/ether.h>
#include <search.h>
#include <fnmatch.h>
#include <langinfo.h>
#include <sys/vfs.h>
#include <termio.h>
#include <math.h>
#include <assert.h>
#include <dlfcn.h>
#include <link.h>
#include <execinfo.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/inotify.h>
#include <regex.h>
#include <iconv.h>
#include <glob.h>
#include <malloc.h>
#include <sys/shm.h>
#include <setjmp.h>
#include <ctype.h>
#include <libintl.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/random.h>
#include <grp.h>
#include <err.h>
#include <wchar.h>
#include <wctype.h>
#include <stdint.h>
#include <pwd.h>
#include <cstdarg>

extern void __cxa_finalize (void *d);
extern int __cxa_atexit (void (*func)(void *), void *arg, void *d);

extern int (*__gxx_personality_v0)(int a, int b,
                                   unsigned c,
                                   struct _Unwind_Exception *d,
                                   struct _Unwind_Context *e);

// extern int __gxx_personality_v0 (int a, int b,
//                                                               unsigned c, struct _Unwind_Exception *d, struct _Unwind_Context *e);
// extern int __xpg_strerror_r (int __errnum, char *__buf, size_t __buflen);
extern int __xpg_strerror_r (int __errnum, char *__buf, size_t __buflen);

// from glibc's string.h
extern char * __strcpy_chk (char *__restrict __dest,
                            const char *__restrict __src,
                            size_t __destlen);
extern void *__memcpy_chk (void *__restrict __dest,
                           const void *__restrict __src, size_t __len,
                           size_t __destlen) __THROW;

// from glibc's stdio.h
extern int __sprintf_chk (char *, int, size_t, const char *, ...) __THROW;
extern int __snprintf_chk (char *, size_t, int, size_t, const char *, ...)
__THROW;
extern int __vsprintf_chk (char *, int, size_t, const char *,
                           __gnuc_va_list) __THROW;
extern int __vsnprintf_chk (char *, size_t, int, size_t, const char *,
                            __gnuc_va_list) __THROW;
extern int __printf_chk (int, const char *, ...);
extern int __fprintf_chk (FILE *, int, const char *, ...);
extern int __vprintf_chk (int, const char *, __gnuc_va_list);
extern int __vfprintf_chk (FILE *, int, const char *, __gnuc_va_list);
extern char * __fgets_unlocked_chk (char *buf, size_t size, int n, FILE *fp);
extern char * __fgets_chk (char *buf, size_t size, int n, FILE *fp);
extern int __asprintf_chk (char **, int, const char *, ...) __THROW;
extern int __vasprintf_chk (char **, int, const char *, __gnuc_va_list) __THROW;
extern int __dprintf_chk (int, int, const char *, ...);
extern int __vdprintf_chk (int, int, const char *, __gnuc_va_list);
extern int __obstack_printf_chk (struct obstack *, int, const char *, ...)
__THROW;
extern int __obstack_vprintf_chk (struct obstack *, int, const char *,
                                  __gnuc_va_list) __THROW;
extern void __stack_chk_fail (void);
// fortified string/memory/select helpers and C23 scanf variants of glibc,
// used by system libraries (X11) loaded by DCE applications
extern "C" {
extern long int __fdelt_chk (long int d);
extern void *__memmove_chk (void *dest, const void *src, size_t len, size_t destlen);
extern void *__memset_chk (void *dest, int c, size_t len, size_t destlen);
extern char *__strcat_chk (char *dest, const char *src, size_t destlen);
extern char *__strncpy_chk (char *dest, const char *src, size_t len, size_t destlen);
extern char *__strncat_chk (char *dest, const char *src, size_t len, size_t destlen);
extern char *__stpcpy_chk (char *dest, const char *src, size_t destlen);
extern ssize_t __read_chk (int fd, void *buf, size_t nbytes, size_t buflen);
extern int __isoc23_sscanf (const char *s, const char *format, ...);
extern int __isoc23_fscanf (FILE *stream, const char *format, ...);
extern int __isoc23_vsscanf (const char *s, const char *format, __gnuc_va_list arg);
extern long int __isoc23_strtol (const char *nptr, char **endptr, int base);
extern long long int __isoc23_strtoll (const char *nptr, char **endptr, int base);
extern unsigned long int __isoc23_strtoul (const char *nptr, char **endptr, int base);
extern unsigned long long int __isoc23_strtoull (const char *nptr, char **endptr, int base);
extern intmax_t __isoc23_strtoimax (const char *nptr, char **endptr, int base);
extern uintmax_t __isoc23_strtoumax (const char *nptr, char **endptr, int base);
extern size_t __mbstowcs_chk (wchar_t *dst, const char *src, size_t len, size_t dstlen);
extern int __register_atfork (void (*prepare) (void), void (*parent) (void), void (*child) (void), void *dso_handle);
extern size_t __fread_chk (void *ptr, size_t ptrlen, size_t size, size_t n, FILE *stream);
extern int __open_2 (const char *file, int oflag);
extern ssize_t __readlink_chk (const char *path, char *buf, size_t len, size_t buflen);
extern char *__realpath_chk (const char *path, char *resolved, size_t resolvedlen);
extern size_t __strlcpy_chk (char *dst, const char *src, size_t n, size_t dstlen);
extern void __longjmp_chk (jmp_buf env, int val) __attribute__ ((noreturn));
extern void __syslog_chk (int priority, int flag, const char *format, ...);
extern int __open64_2 (const char *file, int oflag);
extern int __openat_2 (int fd, const char *file, int oflag);
extern char *__getcwd_chk (char *buf, size_t size, size_t buflen);
extern int __getgroups_chk (int size, __gid_t list[], size_t listlen);
extern void *__mempcpy_chk (void *dest, const void *src, size_t len, size_t destlen);
extern long long int __isoc23_strtoll_l (const char *nptr, char **endptr, int base, locale_t loc);
extern unsigned long long int __isoc23_strtoull_l (const char *nptr, char **endptr, int base, locale_t loc);
extern int __isoc23_vfscanf (FILE *stream, const char *format, __gnuc_va_list arg);
extern int __uflow (FILE *);
}
extern int _IO_getc(_IO_FILE * __fp);
extern int _IO_putc(int __c, _IO_FILE * __fp);

typedef void (*func_t)(...);

struct dl_open_hook
{
  void *(*dlopen_mode) (const char *name, int mode);
  void *(*dlsym) (void *map, const char *name);
  int (*dlclose) (void *map);
};

void *private_dlopen (const char *name, int mode)
{
  return dlopen(name, RTLD_LAZY);
}

extern "C" {

static struct dl_open_hook dce_dl_open_hook =
  {
    .dlopen_mode = private_dlopen,
    .dlsym = dlsym,
    .dlclose = dlclose
  };

extern int __libc_start_main(int *(main) (int, char * *, char * *),
                             int argc, char * * ubp_av, void (*init) (void),
                             void (*fini) (void),
                             void (*rtld_fini) (void), void (* stack_end));

void libc_dce (struct Libc **libc)
{
  *libc = new Libc;

#define DCE(name) (*libc)->name ## _fn = (func_t)(__typeof (&name))dce_ ## name;
#define DCET(rtype,name) DCE (name)
#define DCE_EXPLICIT(name,rtype,...) (*libc)->name ## _fn = dce_ ## name;

#define NATIVE(name)                                                    \
  (*libc)->name ## _fn = (func_t)name;
#define NATIVET(rtype, name) NATIVE(name)

#define NATIVE_EXPLICIT(name, type)                             \
  (*libc)->name ## _fn = (func_t)((type)name);

#include "libc-ns3.h"

  (*libc)->strpbrk_fn = dce_strpbrk;
  (*libc)->strstr_fn = dce_strstr;
  (*libc)->vsnprintf_fn = dce_vsnprintf;

  // TODO:
  //extern struct dl_open_hook *_dl_open_hook;
  struct dl_open_hook *_dl_open_hook;
  _dl_open_hook = (struct dl_open_hook *)&dce_dl_open_hook;
}
} // extern "C"

