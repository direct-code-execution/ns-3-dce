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
#define NATIVE_WITH_ALIAS2(name,internal) NATIVE_WITH_ALIAS(name)
#endif

#ifndef DCE_WITH_ALIAS
#define DCE_WITH_ALIAS DCE
#endif

#ifndef DCE_WITH_ALIAS2
#define DCE_WITH_ALIAS2(name,internal) DCE_WITH_ALIAS(name)
#endif

#ifndef NATIVE_EXPLICIT
#define NATIVE_EXPLICIT(name,type) NATIVE(name)
#endif

// #ifndef ALIAS
// #define ALIAS(base_function, alias_name)
// #endif

// // not really a libc function, but we still need to get pointer from DCE to this function
NATIVE (dce_global_variables_setup)

NATIVE (strerror)
NATIVE (strerror_r)
// Not sure where it is defined and implemented
// NATIVE (__xpg_strerror_r) 

DCE    (atexit)
DCE    (__cxa_finalize)
DCE    (__cxa_atexit)
// Not sure where it is defined and implemented
NATIVE (__gxx_personality_v0)

DCE    (setlocale)
NATIVE (strcoll)
NATIVE_WITH_ALIAS (newlocale)
NATIVE_WITH_ALIAS (uselocale)

NATIVE (wctob)
NATIVE (btowc)

NATIVE (htonl)
NATIVE (htons)
NATIVE (ntohl)
NATIVE (ntohs)
DCE    (socket)
DCE    (getsockname)
DCE    (getpeername)
DCE    (bind)
DCE    (read)
DCE    (connect)
DCE    (write)
DCE    (writev)
NATIVE (memset)
NATIVE (memcpy)
NATIVE (bcopy)
NATIVE (memcmp)
NATIVE (memmove)
NATIVE_EXPLICIT (memchr, void * (*) (void *, int, size_t))
NATIVE (strcpy)
DCE    (__strcpy_chk)
NATIVE (strncpy)
NATIVE (strcat)
NATIVE (strncat)
NATIVE (strcmp)
NATIVE (strncmp)
NATIVE (strlen)
NATIVE (strcspn)
NATIVE (strspn)
NATIVE (strnlen)
// because C++ defines both const and non-const functions
NATIVE_EXPLICIT (strchr, char* (*) (char *, int))
NATIVE_EXPLICIT (strrchr, char * (*) (char *, int))
NATIVE (strcasecmp)
NATIVE (strncasecmp)

DCE_WITH_ALIAS (strdup)
DCE    (strndup)
DCE    (sleep)
DCE    (usleep)
DCE_WITH_ALIAS2 (clearerr,clearerr_unlocked)

// stdio
DCE    (setvbuf)
DCE    (setbuf)
DCE    (setbuffer)
DCE    (setlinebuf)
DCE    (rewind)

DCE    (printf)
NATIVE (fprintf)
NATIVE (sprintf)
NATIVE (snprintf)
NATIVE (asprintf)
NATIVE (dprintf)

DCE    (__printf_chk)
DCE    (__vfprintf_chk)
DCE    (__fprintf_chk)
DCE    (__snprintf_chk)

DCE_WITH_ALIAS2 (fgetc,fgetc_unlocked)
NATIVE (getc)
NATIVE (getc_unlocked)
DCE_WITH_ALIAS2 (getchar,getchar_unlocked)
DCE    (_IO_getc)
DCE_WITH_ALIAS2 (fputc,fputc_unlocked)
NATIVE (putc)
NATIVE (putc_unlocked)
DCE_WITH_ALIAS2 (putchar, putchar_unlocked)
DCE    (_IO_putc)
DCE_WITH_ALIAS2 (fgets, fgets_unlocked)
DCE_WITH_ALIAS2 (fputs, fputs_unlocked)
DCE    (puts)
DCE    (ungetc)

DCE    (fclose)
DCE_WITH_ALIAS2 (fflush,fflush_unlocked)
DCE    (fcloseall)
DCE    (fopen)
DCE    (fopen64)
DCE    (freopen)
DCE    (fdopen)

DCE_WITH_ALIAS2 (fread, fread_unlocked)
DCE_WITH_ALIAS2 (fwrite,fwrite_unlocked)
DCE    (fseek)
DCE    (ftell)
DCE    (fgetpos)
DCE    (fsetpos)
DCE_WITH_ALIAS2 (ferror,ferror_unlocked)
DCE_WITH_ALIAS2 (feof,feof_unlocked)
DCE_WITH_ALIAS2 (fileno,fileno_unlocked)

DCE    (perror)

// stdarg
DCE    (vprintf)
NATIVE (vfprintf)
NATIVE (vsprintf)
NATIVE (vsnprintf)
NATIVE (vasprintf)
NATIVE (vdprintf)


DCE    (fcntl)
DCE    (nanosleep)

DCE    (random)
DCE    (rand)
DCE    (srandom)
DCE    (srand)
DCE    (seed48)
DCE    (drand48)
DCE    (nrand48)
DCE    (lrand48)
DCE    (mrand48)
DCE    (erand48)
DCE    (jrand48)
DCE    (srand48)
DCE    (lcong48)

NATIVE (drand48_r)
NATIVE (erand48_r)
NATIVE (lrand48_r)
NATIVE (nrand48_r)
NATIVE (mrand48_r)
NATIVE (jrand48_r)
NATIVE (srand48_r)
NATIVE (seed48_r)
NATIVE (lcong48_r)

DCE    (__errno_location)
DCE    (getopt)
DCE    (getopt_long)
DCE    (getpid)
DCE    (getppid)
DCE    (getuid)
DCE    (geteuid)
DCE    (setuid)
DCE    (setgid)
DCE    (seteuid)
DCE    (setegid)
DCE    (setreuid)
DCE    (setregid)
DCE    (setresuid)
DCE    (setresgid)
NATIVE (inet_aton)
NATIVE (inet_ntoa)
DCE    (inet_ntop)
NATIVE (inet_pton)
NATIVE (inet_network)
NATIVE (inet_makeaddr)
NATIVE (inet_lnaof)
NATIVE (inet_netof)
NATIVE (inet_addr)
DCE    (mmap)
DCE    (mmap64)
DCE    (munmap)
DCE    (__xstat)
DCE    (__lxstat)
DCE    (__fxstat)
DCE    (__xstat64)
DCE    (__lxstat64)
DCE    (__fxstat64)
DCE    (__fxstatat)
DCE    (dup)
DCE    (dup2)
DCE    (open)
DCE    (open64)
DCE    (close)
DCE    (unlink)
DCE    (remove)
DCE    (mkdir)
DCE    (rmdir)
DCE    (setsockopt)
DCE    (getsockopt)
DCE    (select)
DCE    (listen)
DCE    (accept)
DCE    (shutdown)
DCE    (calloc)
DCE    (malloc)
DCE    (free)
DCE    (realloc)
DCE    (gettimeofday)
DCE    (time)
DCE    (isatty)
DCE    (send)
DCE    (sendto)
DCE    (sendmsg)
DCE    (recv)
DCE    (recvfrom)
DCE    (recvmsg)
DCE    (ioctl)
DCE    (sched_yield)
DCE    (poll)
NATIVE (__cmsg_nxthdr)
NATIVE (atoi)
NATIVE (atol)
NATIVE (atoll)
NATIVE (atof)
DCE    (setitimer)
DCE    (getitimer)
DCE    (exit)
DCE    (signal)
DCE    (sigaction)
NATIVE (sigemptyset)
NATIVE (sigfillset)
NATIVE (sigaddset)
NATIVE (sigdelset)
NATIVE (sigismember)
DCE_WITH_ALIAS2(strtol, __strtol_internal)
DCET    (long long int, strtoll)
DCE    (strtoul)
DCE    (strtoull)
DCET   (double, strtod)
DCE    (getcwd)
DCE    (getwd)
DCE    (get_current_dir_name)
DCE    (chdir)
DCE    (fchdir)

// time.h
DCE    (asctime)
NATIVE (asctime_r)
DCE    (ctime)
NATIVE (ctime_r)
DCE_WITH_ALIAS2 (gmtime, localtime)
NATIVE_WITH_ALIAS2 (gmtime_r, localtime_r)
// DCE    (localtime)
// NATIVE (localtime_r)
NATIVE (mktime)
NATIVE (strftime)

// ctype.h
NATIVE (__ctype_b_loc)
NATIVE_WITH_ALIAS (wctype_l)
NATIVE (__ctype_tolower_loc)

// pthread.h
DCE    (pthread_create)
DCE    (pthread_exit)
DCE    (pthread_self)
DCE_WITH_ALIAS    (pthread_once)
DCE    (pthread_getspecific)
DCE    (pthread_setspecific)
DCE    (pthread_key_create)
DCE    (pthread_key_delete)
DCE    (pthread_mutex_destroy)
DCE    (pthread_mutex_init)
DCE    (pthread_mutex_lock)
DCE    (pthread_mutex_unlock)
DCE    (pthread_mutex_trylock)
DCE    (pthread_mutexattr_init)
DCE    (pthread_mutexattr_destroy)
DCE    (pthread_mutexattr_settype)
DCE    (pthread_cancel)
DCE    (pthread_kill)
DCE    (pthread_join)
DCE    (pthread_detach)
DCE    (sem_init)
DCE    (sem_destroy)
DCE    (sem_post)
DCE    (sem_wait)
DCE    (sem_timedwait)
DCE    (sem_trywait)
DCE    (sem_getvalue)
DCE    (pthread_cond_destroy)
DCE    (pthread_cond_init)
DCE    (pthread_cond_broadcast)
DCE    (pthread_cond_signal)
DCE    (pthread_cond_timedwait)
DCE    (pthread_cond_wait)
DCE    (pthread_condattr_destroy)
DCE    (pthread_condattr_init)

// netdb.h
DCE    (gethostbyname)
DCE    (gethostbyname2)
DCE    (getaddrinfo)
// these three calls will effectively use /etc/passwd on the base system 
NATIVE (gethostent)
NATIVE (sethostent)
NATIVE (endhostent)
DCE    (freeaddrinfo)
DCE    (gai_strerror)
DCE    (herror)
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

DCE    (getenv)
DCE    (putenv)
DCE    (setenv)
DCE    (unsetenv)
DCE    (clearenv)

NATIVE (toupper)
NATIVE (tolower)

DCE    (timerfd_create)
DCE    (timerfd_settime)
DCE    (timerfd_gettime)

DCE    (if_nametoindex)
DCE    (fork)

NATIVE (qsort)
NATIVE (alphasort64)
DCE    (umask)
DCE    (abort)
NATIVE_EXPLICIT (index, char * (*) (char *, int))
NATIVE_EXPLICIT (rindex, char * (*) (char *, int))
NATIVE (strtok)
NATIVE (strtok_r)
DCE    (getnameinfo)
DCE    (uname)
NATIVE (sscanf)
DCE    (execv)
DCE    (execl)
DCE    (execve)
DCE    (execvp)
DCE    (execlp)
DCE    (execle)

DCE    (wait)
DCE    (waitpid)

NATIVE (basename)
NATIVE (dirname)
NATIVE (__xpg_basename)

DCE    (truncate)
DCE    (ftruncate)

// pwd.h
//NATIVE (getpwnam)
// grp.h
NATIVE (getgrnam)

// sys/resource.h
NATIVE (getrusage) // not sure if native call will give stats about the requested process..

// syslog.h
DCE    (openlog)
DCE    (closelog)
DCE    (setlogmask)
DCE    (syslog)
DCE    (vsyslog)

// unistd.h
NATIVE (sysconf)
DCE (ttyname)
DCE (sbrk)
DCE (getpagesize)
DCE (getgid)
DCE (getegid)
DCE (gethostname)
DCE (getpgrp)
DCE (lseek)
DCE (euidaccess)
DCE (eaccess)
DCE (pipe)
NATIVE (pathconf)

// this is wrong. clock should be changed to DCE implementation
//DCE    (__vdso_clock_gettime)
NATIVE_WITH_ALIAS2 (clock_gettime, __vdso_clock_gettime)

// setjmp.h
NATIVE (__sigsetjmp)
NATIVE (siglongjmp)

// libintl.h
NATIVE (bindtextdomain)
NATIVE (textdomain)
NATIVE (gettext)

// signal.h
DCE (sigprocmask)

// stdlib.h
NATIVE (__ctype_get_mb_cur_max)

// pwd.h
DCE (getpwnam)
DCE (getpwuid)
DCE (endpwent)

// wchar.h
NATIVE (mbrlen)

// stdio.h
DCE (__fpurge)
DCE (__fpending)

// inttypes.h
NATIVE (strtoimax)
NATIVE (strtoumax)

// dirent.h
DCE (opendir)
DCE (fdopendir)
DCE (readdir)
DCE (readdir_r)
DCE (closedir)
DCE (dirfd)
DCE (rewinddir)
DCE (scandir)
NATIVE (alphasort)
NATIVE (versionsort)

// fcntl.h
DCE (unlinkat)

// unistd.h
NATIVE (getdtablesize)

DCE (pread)
DCE (pwrite)

// ether.h
NATIVE (ether_aton_r)


#undef DCE
#undef DCET
#undef NATIVE
#undef NATIVE_WITH_ALIAS
#undef NATIVE_WITH_ALIAS2
#undef NATIVE_EXPLICIT
#undef DCE_WITH_ALIAS
#undef DCE_WITH_ALIAS2

