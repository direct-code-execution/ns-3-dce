#ifndef SIMU_MISC_H
#define SIMU_MISC_H


#include <sys/utsname.h>
#include <sys/types.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int , uname, (struct utsname *buf));
DCE(int , gethostname, (char *name, size_t len));

// WCHAR.H
NATIVE (wctob)
NATIVE(btowc)
NATIVE(mbrlen)

// INTTYPES.H
NATIVE (strtoimax)
NATIVE (strtoumax)

// NETINET/ETHER.H
NATIVE (ether_aton_r)
NATIVE (ether_aton)

// SEARCH.H
NATIVE (tsearch)
NATIVE (tfind)
NATIVE (tdelete)
NATIVE (twalk)
NATIVE (tdestroy)

// FNMATCH.H
NATIVE (fnmatch)

// LANGINFO.H
NATIVE (nl_langinfo)


// SETJMP.H
NATIVE (_setjmp)
NATIVE (__sigsetjmp)
NATIVE (siglongjmp)

// LIBINTL.H
NATIVE (bindtextdomain)
NATIVE (textdomain)
NATIVE (gettext)
NATIVE (catopen)
NATIVE (catgets)
// LIBGEN.H
NATIVE (basename)
NATIVE (dirname)
NATIVE (__xpg_basename)

// GRP.H
NATIVE (getgrnam)

// SYS/RESOURCE.H
NATIVE (getrusage) // not sure if native call will give stats about the requested process..
NATIVE (getrlimit)
NATIVE (setrlimit)

// ctype.h
NATIVE (toupper)
NATIVE (tolower)
NATIVE (isdigit)
NATIVE (isxdigit)
NATIVE (isalnum)
NATIVE (__ctype_b_loc)

// REGRESSION
//NATIVE_WITH_ALIAS (wctype_l) 
//NATIVE (__ctype_tolower_loc)


// link.h
NATIVE (dl_iterate_phdr)


#ifdef __cplusplus
}
#endif

#endif /* SIMU_MISC_H */
