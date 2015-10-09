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
#include <xlocale.h>
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
// from glibc's stdio.h
extern int __sprintf_chk (char *, int, size_t, const char *, ...) __THROW;
extern int __snprintf_chk (char *, size_t, int, size_t, const char *, ...)
__THROW;
extern int __vsprintf_chk (char *, int, size_t, const char *,
                           _G_va_list) __THROW;
extern int __vsnprintf_chk (char *, size_t, int, size_t, const char *,
                            _G_va_list) __THROW;
extern int __printf_chk (int, const char *, ...);
extern int __fprintf_chk (FILE *, int, const char *, ...);
extern int __vprintf_chk (int, const char *, _G_va_list);
extern int __vfprintf_chk (FILE *, int, const char *, _G_va_list);
extern char * __fgets_unlocked_chk (char *buf, size_t size, int n, FILE *fp);
extern char * __fgets_chk (char *buf, size_t size, int n, FILE *fp);
extern int __asprintf_chk (char **, int, const char *, ...) __THROW;
extern int __vasprintf_chk (char **, int, const char *, _G_va_list) __THROW;
extern int __dprintf_chk (int, int, const char *, ...);
extern int __vdprintf_chk (int, int, const char *, _G_va_list);
extern int __obstack_printf_chk (struct obstack *, int, const char *, ...)
__THROW;
extern int __obstack_vprintf_chk (struct obstack *, int, const char *,
                                  _G_va_list) __THROW;
extern void __stack_chk_fail (void);

typedef void (*func_t)(...);

extern "C" {

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
}
} // extern "C"

