#undef __OPTIMIZE__
#include <stdio.h>
#include <netinet/in.h>
#include "libc-globals.h"

FILE *stdin;
FILE *stdout;
FILE *stderr;
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
char *optarg = 0;
int optind = 0, opterr = 0, optopt = 0;
const char *_libc_intl_domainname = "libc";
char **__environ = 0;
int h_errno;
__thread int errno;
__thread struct __res_state *__resp;
char *__progname = 0;
char *dce_proginvname = 0;
char *dce_proginvnameshort = 0;

#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)))

weak_alias (__progname, __progname_full);

weak_alias (__environ,environ);

weak_alias (dce_proginvname,  program_invocation_name);

weak_alias (dce_proginvnameshort, program_invocation_short_name);

