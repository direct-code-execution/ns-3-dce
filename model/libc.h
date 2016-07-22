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


// Generate struct
//#define GENERATE_LIBC

//on peut utiliser des templates pour recuperer les arguments 
#define DCE(rtype, name, args...) rtype (*name ## _fn) (args) ;
/* native => decltype(name) */
#define NATIVE(name, ...) decltype(&name) name ## _fn ;
#define NATIVE_EXPLICIT(name, type) decltype( (type) &name) name ## _fn ;
#define DCE_ALIAS(name, alias)

// NATIVE_EXPLICIT
/**
TODO we could get rid of that ?!!!
**/
struct Libc
{


/* #define DCET(rtype, name) DCE (name) */
/* #define NATIVET(rtype, name) NATIVE (name) */

/* #define DCE_EXPLICIT(name,rtype,...) rtype (*name ## _fn) (__VA_ARGS__); */
 #include "libc-ns3.generated.h"

  void (*dce_global_variables_setup_fn)(struct DceGlobalVariables *variables);
//  char* (*strstr_fn)(const char *a, const char *b);
//  int (*vsnprintf_fn)(char *str, size_t size, const char *format, va_list v);
};

#undef DCE
#undef NATIVE
#undef NATIVE_EXPLICIT
//#undef DCE_WITH_ALIAS
//#undef DCE_WITH_ALIAS2

//#undef GENERATE_LIBC

#endif /* LIBC_H */
