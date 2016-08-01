#ifndef SIMU_MISC_H
#define SIMU_MISC_H


#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>

//#include "dce-guard.h"

namespace ns3 
{
  struct Thread;
  extern int dce_internalClosedir (DIR *dirp, struct ns3::Thread *cur);
}

#ifdef __cplusplus
extern "C" {
#endif


// Temporary while fixing generation bugs
#if 0
// add to dce-sigaction.h
int dce_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

// stdio.h
int fflush(FILE *stream);
 
// Add #include "dce-sigthread.h"

#include <sys/sysinfo.h>

int dce_sysinfo(struct sysinfo *info);
#endif


/*
Contains all functions not generated automatically.
Why do they exist ? freebsd compatibility or ?
Due to freeb
*/



void
CleanupPthreadKeys (void);

//int dce_creat (const char *path, mode_t mode);


// DCE Specific (not in libc)
extern int dce_fclose_unconditional (FILE *fp);
extern int dce_fclose_onexec (FILE *file);
int dce_fcloseall (void);


/*

// WCHAR.H
NATIVE (wctob, wint_t)
NATIVE(btowc, wint_t)
NATIVE(mbrlen, const char *__restrict __s, size_t __n, mbstate_t *__restrict __ps)

// INTTYPES.H
NATIVE (strtoimax, const char *nptr, char **endptr, int base)
NATIVE (strtoumax, const char *nptr, char **endptr, int base)

// NETINET/ETHER.H
NATIVE (ether_aton_r, const char *__asc, struct ether_addr *__addr)
NATIVE (ether_aton, const char *__asc)

// SEARCH.H
NATIVE (tsearch, const void *__key, void **__rootp, __compar_fn_t __compar)
NATIVE (tfind, const void *__key, void *const *__rootp,__compar_fn_t __compar)
NATIVE (tdelete, const void *__restrict __key, void **__restrict __rootp,__compar_fn_t __compar)
NATIVE (twalk, const void *__root, __action_fn_t __action)
NATIVE (tdestroy, void *__root, __free_fn_t __freefct)

// FNMATCH.H
NATIVE (fnmatch, const char *__pattern, const char *__name, int __flags)

// LANGINFO.H
NATIVE (nl_langinfo, nl_item __item)


// SETJMP.H
NATIVE (_setjmp, struct __jmp_buf_tag __env[1])
NATIVE (__sigsetjmp, struct __jmp_buf_tag __env[1], int __savemask)
NATIVE (siglongjmp, sigjmp_buf env, int val)

// LIBINTL.H
NATIVE (bindtextdomain, const char *__domainname, const char *__dirname)
NATIVE (textdomain, const char *__domainname)
NATIVE (gettext, const char *__msgid)
NATIVE (catopen,const char *__cat_name, int __flag)
NATIVE (catgets, nl_catd __catalog, int __set, int __number, const char *__string)

// LIBGEN.H
// REGRESSION
//NATIVE (basename, char *__path)
NATIVE (dirname, char *__path)

//NATIVE (__xpg_basename, char *__path)

// GRP.H
NATIVE (getgrnam, const char *__name)

// SYS/RESOURCE.H
NATIVE (getrusage, __rusage_who_t __who, struct rusage *__usage) // not sure if native call will give stats about the requested process..
NATIVE (getrlimit,__rlimit_resource_t __resource, struct rlimit *__rlimits)
NATIVE (setrlimit, __rlimit_resource_t __resource, const struct rlimit *__rlimits)

// ctype.h
NATIVE (toupper, int)
NATIVE (tolower, int)
NATIVE (isdigit, int)
NATIVE (isxdigit, int)
NATIVE (isalnum, int)
NATIVE (__ctype_b_loc)

// REGRESSION
//NATIVE_WITH_ALIAS (wctype_l) 
//NATIVE (__ctype_tolower_loc)


// link.h
NATIVE (dl_iterate_phdr, int (*__callback) (struct dl_phdr_info *, size_t, void *), void *__data)
*/

#ifdef __cplusplus
}
#endif

#endif /* SIMU_MISC_H */
