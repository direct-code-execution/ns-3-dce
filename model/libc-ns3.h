/**
 * \file This files defines list of redirected (NATIVE) or redefined
 * (DCE) libc methods
 *
 * There is also a variants NATIVE_WITH_ALIAS and DCE_WITH_ALIAS
 * macros that can be used to define two symbols: name and __name
 * pointing to the same function call.
 *
 * Macro DCE should be defined before including this macros.  If
 * NATIVE is not defined, it is defaulted to DCE.  NATIVE_WITH_ALIAS
 * is defaulted to NATIVE, DCE_WITH_ALIAS is defaulted to DCE.
 * DCE_WITH_ALIAS2 is similar to DCE_WITH_ALIAS but accepts two
 * parameter, name of the function (will be weak alias) and name of
 * the internal implementation
 */

#ifndef DCE
#error Macro DCE should be defined before including libc-ns3.h
#endif

#ifndef NATIVE
#define NATIVE DCE
#endif

#ifndef NATIVE_WITH_ALIAS
#define NATIVE_WITH_ALIAS NATIVE
#endif

#ifndef NATIVE_WITH_ALIAS2
#define NATIVE_WITH_ALIAS2(name,internal) NATIVE_WITH_ALIAS (name)
#endif

#ifndef DCE_WITH_ALIAS
#define DCE_WITH_ALIAS DCE
#endif

#ifndef DCE_WITH_ALIAS2
#define DCE_WITH_ALIAS2(name,internal) DCE_WITH_ALIAS (name)
#endif

#ifndef NATIVE_EXPLICIT
#define NATIVE_EXPLICIT(name,type) NATIVE (name)
#endif

// #ifndef ALIAS
// #define ALIAS(base_function, alias_name)
// #endif

// // not really a libc function, but we still need to get pointer from DCE to this function
NATIVE (dce_global_variables_setup)

// Not sure where it is defined and implemented
// NATIVE (__xpg_strerror_r)


DCE    (__cxa_finalize)
DCE    (__cxa_atexit)
// Not sure where it is defined and implemented
NATIVE (__gxx_personality_v0)

// STDLIB.H
DCE (atexit)
DCE (random)
DCE (srandom)
DCE (rand)
DCE (srand)
DCE (drand48)
DCE (erand48)
DCE (lrand48)
DCE (nrand48)
DCE (mrand48)
DCE (jrand48)
DCE (srand48)
DCE (seed48)
DCE (lcong48)
NATIVE (drand48_r)
NATIVE (erand48_r)
NATIVE (lrand48_r)
NATIVE (nrand48_r)
NATIVE (mrand48_r)
NATIVE (jrand48_r)
NATIVE (srand48_r)
NATIVE (seed48_r)
NATIVE (lcong48_r)
DCE (calloc)
DCE_WITH_ALIAS2 (malloc, valloc)
DCE (free)
DCE (realloc)
NATIVE (atoi)
NATIVE (atol)
NATIVE (atoll)
NATIVET (double, atof)
DCE_WITH_ALIAS2 (strtol, __strtol_internal)
DCET (long long int, strtoll)
DCE (strtoul)
DCE (strtoull)
NATIVE (strtof)
NATIVE (strtold)
DCET (double, strtod)
DCE (getenv)
#ifdef HAVE_SECURE_GETENV
NATIVE_WITH_ALIAS2 (secure_getenv, __secure_getenv)
#endif
DCE (putenv)
DCE (setenv)
DCE (unsetenv)
DCE (clearenv)
NATIVE (qsort)
DCE (abort)
DCE (__assert_fail)
DCE (__stack_chk_fail)
DCE (mkstemp)
DCE (tmpfile)
DCE (rename)
NATIVE (mblen)
NATIVE (mbstowcs)
NATIVE (mbtowc)
NATIVE (mbsrtowcs)
NATIVE (mkostemp)

// STRING.H
NATIVE (bzero)
#ifdef HAVE___EXPLICIT_BZERO
NATIVE_WITH_ALIAS2 (explicit_bzero, __explicit_bzero_chk)
#endif	/* HAVE___EXPLICIT_BZERO */
NATIVE (strerror)
NATIVE (strerror_r)
NATIVE (strcoll)
NATIVE (memset)
NATIVE (memcpy)
DCE (__memcpy_chk)
NATIVE (bcopy)
NATIVE (memcmp)
NATIVE (memmove)
NATIVE_EXPLICIT (memchr, void * (*)(void *, int, size_t))
NATIVE_EXPLICIT (memrchr, void * (*)(void *, int, size_t))
NATIVE (strcpy)
NATIVE (strncpy)
NATIVE (__stpncpy)
NATIVE (strcat)
NATIVE (strncat)
NATIVE (strcmp)
NATIVE (strncmp)
NATIVE (strlen)
NATIVE (strnlen)
NATIVE (strcspn)
NATIVE (strspn)
NATIVE_EXPLICIT (strchr, char* (*)(char *, int))
NATIVE_EXPLICIT (strrchr, const char * (*)(const char *, int))
NATIVE (strcasecmp)
NATIVE (strncasecmp)
NATIVE_EXPLICIT (strcasestr, char *(*)(char *, const char *))
DCE_WITH_ALIAS (strdup) // because C++ defines both const and non-const functions
DCE (strndup)
NATIVE_EXPLICIT (index, char * (*)(char *, int))
NATIVE_EXPLICIT (rindex, char * (*)(char *, int))
NATIVE_EXPLICIT (strtok, char * (*)(char *, const char *))
NATIVE_EXPLICIT (strtok_r,  char * (*)(char *, const char *, char **))
NATIVE (strsep)
NATIVE (strsignal)


// LOCALE.H
DCE    (setlocale)
NATIVE_WITH_ALIAS (newlocale)
NATIVE_WITH_ALIAS (uselocale)
NATIVE_WITH_ALIAS (localeconv)

// WCHAR.H
NATIVE (wctob)
NATIVE (btowc)
NATIVE (mbrlen)
NATIVE (mbrtowc)
NATIVE (wcscmp)
NATIVE (wcscoll)
NATIVE (wcsrtombs)
NATIVE (wcslen)
NATIVE (wcsdup)
NATIVE (wcsncmp)
NATIVE_EXPLICIT (wcschr, wchar_t * (*)(wchar_t *, wchar_t))
NATIVE (wctype)
NATIVE (wcwidth)
NATIVE (wcrtomb)
NATIVE (iswalnum)
NATIVE (iswprint)
NATIVE (mbsinit)
NATIVE (towlower)
NATIVE (towupper)

// ARPA/INET.H
NATIVE (htonl)
NATIVE (htons)
NATIVE (ntohl)
NATIVE (ntohs)
NATIVE (lockf)
NATIVE (inet_aton)
NATIVE (inet_addr)
NATIVE (inet_network)
NATIVE (inet_ntoa)
NATIVE (inet_makeaddr)
NATIVE (inet_lnaof)
NATIVE (inet_netof)
DCE (inet_ntop)
NATIVE (inet_pton)
NATIVE (inet6_opt_find)
NATIVE (inet_nsap_ntoa)

// SYS/SOCKET.H
DCE (socket)
DCE (socketpair)
DCE (getsockname)
DCE (getpeername)
DCE (bind)
DCE (connect)
DCE (setsockopt)
DCE (getsockopt)
DCE (listen)
DCE (accept)
DCE (shutdown)
DCE (send)
DCE (sendto)
DCE (sendmsg)
/* XXX */
NATIVE_WITH_ALIAS (sendmmsg)
DCE (recv)
DCE (recvfrom)
DCE (recvmsg)
DCE (getnameinfo)


// UNISTD.H
DCE (read)
DCE (write)
DCE (sleep)
DCE (usleep)
DCE (getopt)
DCE (getopt_long)
DCE (getpid)
DCE (getppid)
DCE (getuid)
DCE (geteuid)
DCE (setuid)
DCE (setgid)
DCE (seteuid)
DCE (setegid)
DCE (setreuid)
DCE (setregid)
DCE (setresuid)
DCE (setresgid)
DCE (dup)
DCE (dup2)
DCE (close)
DCE (unlink)
DCE (rmdir)
DCE (select)
DCE (isatty)
DCE_WITH_ALIAS2 (exit, _exit)
DCE (getcwd)
DCE (getwd)
DCE (get_current_dir_name)
DCE (chdir)
DCE (fchdir)
DCE (fork)
DCE (execv)
DCE (execl)
DCE (execve)
DCE (execvp)
DCE (execlp)
DCE (execle)
DCE (truncate)
DCE (ftruncate)
DCE (ftruncate64)
NATIVE (sysconf)
DCE (ttyname)
DCE (sbrk)
DCE (getpagesize)
DCE (getgid)
DCE (getegid)
DCE (gethostname)
DCE (getpgrp)
NATIVE (setpgrp)
NATIVE (setpgid)
NATIVE (setsid)
NATIVE (tcgetpgrp)
NATIVE (tcsetpgrp)
NATIVE (tcflow)
DCE (lseek)
DCE (lseek64)
DCE (euidaccess)
DCE (eaccess)
DCE (access)
DCE (pipe)
NATIVE (pathconf)
NATIVE (getdtablesize)
DCE (pread)
DCE (pwrite)
DCE (daemon)
DCE (alarm)
DCE (readlink)
DCE (chown)
DCE (initgroups)
DCE (fsync)
NATIVE (chroot)
NATIVE (popen)
NATIVE (pclose)
NATIVE (getgroups)
NATIVE (setgroups)
NATIVE (confstr)
NATIVE (sync)
NATIVE (link)
NATIVE (symlink)
NATIVE (nice)
NATIVE (fchown)

// SYS/UIO.H
DCE (readv)
DCE (writev)

// STDIO.H
DCE_WITH_ALIAS2 (clearerr,clearerr_unlocked)
DCE (setvbuf)
DCE (setbuf)
DCE (setbuffer)
DCE (setlinebuf)
DCE (fseek)
DCE (ftell)
DCE (fseeko)
DCE (ftello)
DCE (rewind)
DCE (fgetpos)
DCE (fsetpos)
DCE (printf)
NATIVE (fprintf)
NATIVE (sprintf)
DCE (asprintf)
DCE (vasprintf)
NATIVE (dprintf)
NATIVE (vdprintf)
DCE_WITH_ALIAS2 (fgetc,fgetc_unlocked)
NATIVE (getc)
NATIVE (getc_unlocked)
DCE_WITH_ALIAS2 (getchar,getchar_unlocked)
DCE (_IO_getc)
DCE_WITH_ALIAS2 (fputc,fputc_unlocked)
NATIVE (putc)
NATIVE (putc_unlocked)
DCE_WITH_ALIAS2 (putchar, putchar_unlocked)
DCE (_IO_putc)
DCE_WITH_ALIAS2 (fgets, fgets_unlocked)
DCE_WITH_ALIAS2 (fputs, fputs_unlocked)
DCE (puts)
DCE (ungetc)
DCE (fclose)
DCE (fcloseall)
DCE (fopen)
DCE (fopen64)
DCE (freopen)
DCE (fdopen)
DCE_WITH_ALIAS2 (fread, fread_unlocked)
DCE_WITH_ALIAS2 (fwrite,fwrite_unlocked)
DCE_WITH_ALIAS2 (fflush,fflush_unlocked)
DCE_WITH_ALIAS2 (ferror,ferror_unlocked)
DCE_WITH_ALIAS2 (feof,feof_unlocked)
DCE_WITH_ALIAS2 (fileno,fileno_unlocked)
DCE (perror)
DCE (remove)
NATIVE_WITH_ALIAS2 (fscanf, __isoc99_fscanf)
NATIVE_WITH_ALIAS2 (sscanf, __isoc99_sscanf)
NATIVE (flockfile)
NATIVE (funlockfile)
NATIVE (getline)
NATIVE (__fsetlocking)

// STDARG.H
DCE (vprintf)
NATIVE (vfprintf)
NATIVE (vsprintf)

// FCNTL.H
DCE (fcntl)
DCE (open)
DCE (open64)
DCE (unlinkat)

// TIME.H
DCE (nanosleep)
DCE (asctime)
NATIVE (asctime_r)
DCE (ctime)
NATIVE (ctime_r)
DCE_WITH_ALIAS2 (gmtime, localtime)
NATIVE_WITH_ALIAS (gmtime_r)
NATIVE (mktime)
NATIVE (strftime)
NATIVE (strptime)
NATIVE (timegm)
NATIVE (timelocal)
DCE_EXPLICIT (clock_gettime, int, clockid_t, struct timespec *)
DCE_EXPLICIT (clock_getres, int, clockid_t, struct timespec *)

DCE (timer_create)
DCE (timer_settime)
DCE (timer_gettime)

// UTIME.H
DCE (utime)
DCE (tzset)

// SYS/TIME.H
DCE (gettimeofday)
DCE (time)
DCE (setitimer)
DCE (getitimer)

DCE (sysinfo)

// SYS/MAP.H
DCE (mmap)
DCE (mmap64)
DCE (munmap)

// sys/mman.h
NATIVE (madvise)

// SYS/STAT/H
DCE (mkdir)
DCE (umask)
NATIVE (chmod)

// SYS/IOCTL.H
DCE (ioctl)

// SCHED.H
DCE (sched_yield)
NATIVE (unshare)
NATIVE (setns)

// POLL.H
//DCE (poll)
DCE_WITH_ALIAS2 (poll, __poll)
DCE_EXPLICIT (__poll_chk, int, struct pollfd *, long unsigned int, int, long unsigned int)

// SIGNAL.H
DCE (signal)
DCE (sigaction)
NATIVE (sigemptyset)
NATIVE (sigfillset)
NATIVE (sigaddset)
NATIVE (sigdelset)
NATIVE (sigismember)
DCE (sigprocmask)
DCE    (sigwait)
DCE    (kill)
NATIVE (sys_siglist)
NATIVE (killpg)

// PTHREAD.H
DCE (pthread_create)
DCE (pthread_exit)
DCE (pthread_self)
DCE_WITH_ALIAS (pthread_once)
DCE (pthread_getspecific)
DCE (pthread_setspecific)
DCE_WITH_ALIAS (pthread_key_create)
DCE (pthread_key_delete)
DCE (pthread_mutex_destroy)
DCE (pthread_mutex_init)
DCE_EXPLICIT (pthread_mutex_lock, int, pthread_mutex_t *)
DCE_EXPLICIT (pthread_mutex_unlock, int, pthread_mutex_t *)
DCE (pthread_mutex_trylock)
DCE (pthread_mutexattr_init)
DCE (pthread_mutexattr_destroy)
DCE (pthread_mutexattr_settype)
DCE (pthread_cancel)
DCE (pthread_kill)
DCE (pthread_join)
DCE (pthread_detach)
DCE (pthread_cond_destroy)
DCE (pthread_cond_init)
DCE (pthread_cond_broadcast)
DCE (pthread_cond_signal)
DCE_EXPLICIT (pthread_cond_timedwait, int, pthread_cond_t*, pthread_mutex_t*, const struct timespec *)
DCE_EXPLICIT (pthread_cond_wait, int, pthread_cond_t*, pthread_mutex_t*)
DCE (pthread_condattr_destroy)
DCE (pthread_condattr_init)
NATIVE (pthread_rwlock_init)
NATIVE (pthread_rwlock_unlock)
NATIVE (pthread_rwlock_wrlock)
NATIVE (pthread_rwlock_rdlock)
NATIVE (pthread_rwlock_destroy)
NATIVE (pthread_setcancelstate)
NATIVE (pthread_sigmask)
NATIVE (pthread_equal)
NATIVE (pthread_spin_init)
NATIVE (pthread_spin_lock)
NATIVE (pthread_spin_unlock)
NATIVE (pthread_spin_destroy)


// SEMAPHORE.H
DCE (sem_init)
DCE (sem_destroy)
DCE (sem_post)
DCE (sem_wait)
DCE (sem_timedwait)
DCE (sem_trywait)
DCE (sem_getvalue)

// NETDB.H
DCE (gethostbyname)
DCE (gethostbyname2)
DCE (getaddrinfo)
DCE (freeaddrinfo)
DCE (gai_strerror)
DCE (getifaddrs)
NATIVE (freeifaddrs)
NATIVE (gethostent)
NATIVE (sethostent)
NATIVE (endhostent)
DCE (herror)
NATIVE (hstrerror) // this could be replaced by DCE call
NATIVE (getprotoent)
NATIVE (getprotobyname)
NATIVE (getprotobynumber)
NATIVE (setprotoent)
NATIVE (endprotoent)
NATIVE (getservent)
NATIVE (getservbyname)
NATIVE (getservbyport)
NATIVE (setservent)
NATIVE (endservent)
NATIVE (gethostbyaddr)

// CTYPE.H
NATIVE (toupper)
NATIVE (tolower)
NATIVE (iswlower)
NATIVE (isdigit)
NATIVE (isxdigit)
NATIVE (isalnum)

// SYS/TIMERFD.H
DCE (timerfd_create)
DCE (timerfd_settime)
DCE (timerfd_gettime)

// NET/IF.H
DCE (if_nametoindex)
DCE (if_indextoname)
NATIVE (if_nameindex)
NATIVE (if_freenameindex)

// DIRENT.H
DCE (opendir)
DCE (fdopendir)
DCE (readdir)
DCE (readdir_r)
DCE (closedir)
DCE (dirfd)
DCE (rewinddir)
DCE (scandir)
NATIVE (alphasort)
NATIVE (alphasort64)
NATIVE (versionsort)

// SYS/UTSNAME.H
DCE (uname)

// SYS/WAIT.H
DCE (wait)
DCE (waitpid)

// LIBGEN.H
NATIVE (basename)
NATIVE (dirname)
NATIVE (__xpg_basename)

// GRP.H
NATIVE (getgrnam)
NATIVE (endgrent)
NATIVE (getgrent)
NATIVE (setgrent)

// SYS/RESOURCE.H
NATIVE (getrusage) // not sure if native call will give stats about the requested process..
NATIVE (getrlimit)
NATIVE (setrlimit)

// SYSLOG.H
DCE (openlog)
DCE (closelog)
DCE (setlogmask)
DCE (syslog)
DCE (vsyslog)

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
NATIVE (dgettext)
NATIVE (ngettext)

// PWD.H
NATIVE (getpwnam)
NATIVE (setpwent)
NATIVE (getpwent)
DCE (getpwuid)
DCE (endpwent)

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

// SYS/VFS.H
DCE (fstatfs)
DCE (fstatfs64)
DCE (statfs)
DCE (statfs64)

// SYS/STATVFS.H
DCE (statvfs)
DCE (fstatvfs)

// TERMIO.H
DCE (tcgetattr)
DCE (tcsetattr)
NATIVE (tcflush)
NATIVE (cfgetospeed)

///////////////////// END OF INVENTAIRE //////////////////////////////////////////////////

// ctype.h
NATIVE (__ctype_b_loc)
NATIVE_WITH_ALIAS (wctype_l)
NATIVE (__ctype_tolower_loc)
NATIVE (__ctype_toupper_loc)
NATIVE (iswctype)
NATIVE (iswupper)

// stdlib.h
NATIVE (__ctype_get_mb_cur_max)

// stdio.h
DCE (__fpurge)
DCE (__fpending)

DCE (__strcpy_chk)
DCE    (__printf_chk)
DCE    (__vfprintf_chk)
DCE    (__fprintf_chk)
DCE    (__snprintf_chk)
DCE    (__errno_location)
DCE    (__h_errno_location)
DCE    (__vsnprintf_chk)

DCE    (__xstat)
DCE    (__lxstat)
DCE    (__fxstat)
DCE    (__xstat64)
DCE    (__lxstat64)
DCE    (__fxstat64)
DCE    (__fxstatat)
NATIVE (__cmsg_nxthdr)

// math.h
NATIVE (lrintl)
NATIVE (llrintl)
NATIVE_EXPLICIT (ceil, double(*)(double))
NATIVE_EXPLICIT (floor, double(*)(double))
NATIVE_EXPLICIT (trunc, double(*)(double))
NATIVE_EXPLICIT (sqrt, double(*)(double))
NATIVE_EXPLICIT (exp, double(*)(double))
NATIVE_EXPLICIT (pow, double(*)(double, double))
NATIVE_EXPLICIT (log, double(*)(double))
NATIVE_EXPLICIT (log10, double(*)(double))

// dlfcn.h
DCE_WITH_ALIAS2 (dlopen, __dlopen)
DCE (dlsym)
DCE (dlclose)
NATIVE (dlerror)

// execinfo.h
NATIVE (backtrace)
NATIVE (backtrace_symbols)
NATIVE (backtrace_symbols_fd)

// sys/mount.h
NATIVE (mount)
NATIVE (umount2)

// sys/inotify.h
NATIVE (inotify_init)
NATIVE (inotify_add_watch)

// regex.h
NATIVE (regfree)
NATIVE (regcomp)
NATIVE (regexec)
NATIVE (regerror)

// iconv.h
NATIVE (iconv_open)

// glob.h
NATIVE (glob)
NATIVE (globfree)

// malloc.h
NATIVE (mallinfo)

// error.h
NATIVE (error)

NATIVE (dl_iterate_phdr)
NATIVE (__libc_start_main)

#undef DCE
#undef DCET
#undef DCE_EXPLICIT
#undef NATIVE
#undef NATIVET
#undef NATIVE_WITH_ALIAS
#undef NATIVE_WITH_ALIAS2
#undef NATIVE_EXPLICIT
#undef DCE_WITH_ALIAS
#undef DCE_WITH_ALIAS2

