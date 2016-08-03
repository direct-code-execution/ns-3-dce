#ifndef LIBC_H
#define LIBC_H

#include <stdarg.h>
#define _SYS_SELECT_H
#include <sys/types.h>
#undef _SYS_SELECT_H

#undef __CORRECT_ISO_CPP_STRING_H_PROTO

#include <wchar.h>
#include <link.h>
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
#include <cstring>
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
#include <sys/uio.h>
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
#include <dirent.h>
#include <unistd.h>
#include <wctype.h>
#include <locale.h>
#include <utime.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <getopt.h>
#include <utime.h>


//on peut utiliser des templates pour recuperer les arguments 
//#define DCE(name, args...) rtype (*name ## _fn) (args) ;
/* native => decltype(name) */
#define DCE NATIVE
#define NATIVE(name, ...) decltype(&name) name ## _fn ;

#define ALIAS(name, ...)

#ifdef __cplusplus
extern "C" {
#endif
// In case it's not defined
extern void __cxa_finalize (void *d);
extern int __cxa_atexit (void (*func)(void *), void *arg, void *d);

#ifdef __cplusplus
}
#endif
/**
TODO we could get rid of that ?!!!

This structure holds function pointers that are then overriden
**/
struct Libc
{

 #include "libc-ns3.h"

  /* items that can't be found via libclang  (not exported etc...) */
//  void (*__cxa_finalize_fn) (void *d);
//  int (*__cxa_atexit_fn)(void (*func)(void *), void *arg, void *d) ;
  
 
  void (*dce_global_variables_setup_fn)(struct DceGlobalVariables *variables);
//  char* (*strstr_fn)(const char *a, const char *b);
//  int (*vsnprintf_fn)(char *str, size_t size, const char *format, va_list v);
};

#undef DCE
#undef NATIVE
#undef NATIVE_EXPLICIT


#endif /* LIBC_H */
