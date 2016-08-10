 void __cxa_finalize (void *d)  {
                                     dce___cxa_finalize (d);
                                    }
                                     int __cxa_atexit (void (*func)(void *), void *arg, void *d)  {
                                    return dce___cxa_atexit (func, arg, d);
                                    }
                                     int atexit (void (*)(  ) ) noexcept {
                                    return dce_atexit (__func);
                                    }
                                     long int random () noexcept {
                                    return dce_random ();
                                    }
                                     void srandom (unsigned int __seed) noexcept {
                                     dce_srandom (__seed);
                                    }
                                     int rand () noexcept {
                                    return dce_rand ();
                                    }
                                     void srand (unsigned int __seed) noexcept {
                                     dce_srand (__seed);
                                    }
                                     double drand48 () noexcept {
                                    return dce_drand48 ();
                                    }
                                     double erand48 (short unsigned int * __xsubi) noexcept {
                                    return dce_erand48 (__xsubi);
                                    }
                                     long int lrand48 () noexcept {
                                    return dce_lrand48 ();
                                    }
                                     long int nrand48 (short unsigned int * __xsubi) noexcept {
                                    return dce_nrand48 (__xsubi);
                                    }
                                     long int mrand48 () noexcept {
                                    return dce_mrand48 ();
                                    }
                                     long int jrand48 (short unsigned int * __xsubi) noexcept {
                                    return dce_jrand48 (__xsubi);
                                    }
                                     void srand48 (long int __seedval) noexcept {
                                     dce_srand48 (__seedval);
                                    }
                                     short unsigned int * seed48 (short unsigned int * __seed16v) noexcept {
                                    return dce_seed48 (__seed16v);
                                    }
                                     void lcong48 (short unsigned int * __param) noexcept {
                                     dce_lcong48 (__param);
                                    }
                                     int drand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ double * __result) noexcept {
                                    return g_libc.drand48_r_fn (__buffer,__result);
                                    }
                                     int erand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ double * __result) noexcept {
                                    return g_libc.erand48_r_fn (__xsubi,__buffer,__result);
                                    }
                                     int lrand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) noexcept {
                                    return g_libc.lrand48_r_fn (__buffer,__result);
                                    }
                                     int nrand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) noexcept {
                                    return g_libc.nrand48_r_fn (__xsubi,__buffer,__result);
                                    }
                                     int mrand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) noexcept {
                                    return g_libc.mrand48_r_fn (__buffer,__result);
                                    }
                                     int jrand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) noexcept {
                                    return g_libc.jrand48_r_fn (__xsubi,__buffer,__result);
                                    }
                                     int srand48_r (long int __seedval,drand48_data * __buffer) noexcept {
                                    return g_libc.srand48_r_fn (__seedval,__buffer);
                                    }
                                     int seed48_r (short unsigned int * __seed16v,drand48_data * __buffer) noexcept {
                                    return g_libc.seed48_r_fn (__seed16v,__buffer);
                                    }
                                     int lcong48_r (short unsigned int * __param,drand48_data * __buffer) noexcept {
                                    return g_libc.lcong48_r_fn (__param,__buffer);
                                    }
                                     void * calloc (size_t __nmemb,size_t __size) noexcept {
                                    return dce_calloc (__nmemb,__size);
                                    }
                                     void * malloc (size_t __size) noexcept {
                                    return dce_malloc (__size);
                                    }
                                    decltype (malloc) valloc __attribute__ ((weak, alias ("malloc")));
 void free (void * __ptr) noexcept {
                                     dce_free (__ptr);
                                    }
                                     void * realloc (void * __ptr,size_t __size) noexcept {
                                    return dce_realloc (__ptr,__size);
                                    }
                                     int atoi (char const * __nptr) noexcept {
                                    return g_libc.atoi_fn (__nptr);
                                    }
                                     long int atol (char const * __nptr) noexcept {
                                    return g_libc.atol_fn (__nptr);
                                    }
                                     long long int atoll (char const * __nptr) noexcept {
                                    return g_libc.atoll_fn (__nptr);
                                    }
                                     double atof (char const * __nptr) noexcept {
                                    return g_libc.atof_fn (__nptr);
                                    }
                                     long int strtol (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept {
                                    return dce_strtol (__nptr,__endptr,__base);
                                    }
                                    decltype (strtol) __strtol_internal __attribute__ ((weak, alias ("strtol")));
 long long int strtoll (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept {
                                    return dce_strtoll (__nptr,__endptr,__base);
                                    }
                                     long unsigned int strtoul (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept {
                                    return dce_strtoul (__nptr,__endptr,__base);
                                    }
                                     long long unsigned int strtoull (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept {
                                    return dce_strtoull (__nptr,__endptr,__base);
                                    }
                                     double strtod (__restrict__ char const * __nptr,__restrict__ char * * __endptr) noexcept {
                                    return dce_strtod (__nptr,__endptr);
                                    }
                                     char * getenv (char const * __name) noexcept {
                                    return dce_getenv (__name);
                                    }
                                     int putenv (char * __string) noexcept {
                                    return dce_putenv (__string);
                                    }
                                     int setenv (char const * __name,char const * __value,int __replace) noexcept {
                                    return dce_setenv (__name,__value,__replace);
                                    }
                                     int unsetenv (char const * __name) noexcept {
                                    return dce_unsetenv (__name);
                                    }
                                     int clearenv () noexcept {
                                    return dce_clearenv ();
                                    }
                                     void qsort (void * __base,size_t __nmemb,size_t __size,__compar_fn_t __compar)  {
                                     g_libc.qsort_fn (__base,__nmemb,__size,__compar);
                                    }
                                     void abort (void) noexcept __attribute__ ((__noreturn__)) {
                                     dce_abort ();
                                    }
                                     int mkstemp (char * __template)  {
                                    return dce_mkstemp (__template);
                                    }
                                     FILE * tmpfile ()  {
                                    return dce_tmpfile ();
                                    }
                                     int rename (char const * __old,char const * __new) noexcept {
                                    return dce_rename (__old,__new);
                                    }
                                     void bzero (void * __s,size_t __n) noexcept {
                                     g_libc.bzero_fn (__s,__n);
                                    }
                                     char * strerror (int __errnum) noexcept {
                                    return g_libc.strerror_fn (__errnum);
                                    }
                                     char * strerror_r (int __errnum,char * __buf,size_t __buflen) noexcept {
                                    return g_libc.strerror_r_fn (__errnum,__buf,__buflen);
                                    }
                                     int strcoll (char const * __s1,char const * __s2) noexcept {
                                    return g_libc.strcoll_fn (__s1,__s2);
                                    }
                                     void * memset (void * __s,int __c,size_t __n) noexcept {
                                    return g_libc.memset_fn (__s,__c,__n);
                                    }
                                     void * memcpy (__restrict__ void * __dest,__restrict__ void const * __src,size_t __n) noexcept {
                                    return g_libc.memcpy_fn (__dest,__src,__n);
                                    }
                                     void bcopy (void const * __src,void * __dest,size_t __n) noexcept {
                                     g_libc.bcopy_fn (__src,__dest,__n);
                                    }
                                     int memcmp (void const * __s1,void const * __s2,size_t __n) noexcept {
                                    return g_libc.memcmp_fn (__s1,__s2,__n);
                                    }
                                     void * memmove (void * __dest,void const * __src,size_t __n) noexcept {
                                    return g_libc.memmove_fn (__dest,__src,__n);
                                    }
                                     char * strcpy (__restrict__ char * __dest,__restrict__ char const * __src) noexcept {
                                    return g_libc.strcpy_fn (__dest,__src);
                                    }
                                     char * strncpy (__restrict__ char * __dest,__restrict__ char const * __src,size_t __n) noexcept {
                                    return g_libc.strncpy_fn (__dest,__src,__n);
                                    }
                                     char * strcat (__restrict__ char * __dest,__restrict__ char const * __src) noexcept {
                                    return g_libc.strcat_fn (__dest,__src);
                                    }
                                     char * strncat (__restrict__ char * __dest,__restrict__ char const * __src,size_t __n) noexcept {
                                    return g_libc.strncat_fn (__dest,__src,__n);
                                    }
                                     int strcmp (char const * __s1,char const * __s2) noexcept {
                                    return g_libc.strcmp_fn (__s1,__s2);
                                    }
                                     int strncmp (char const * __s1,char const * __s2,size_t __n) noexcept {
                                    return g_libc.strncmp_fn (__s1,__s2,__n);
                                    }
                                     size_t strlen (char const * __s) noexcept {
                                    return g_libc.strlen_fn (__s);
                                    }
                                     size_t strnlen (char const * __string,size_t __maxlen) noexcept {
                                    return g_libc.strnlen_fn (__string,__maxlen);
                                    }
                                     size_t strcspn (char const * __s,char const * __reject) noexcept {
                                    return g_libc.strcspn_fn (__s,__reject);
                                    }
                                     size_t strspn (char const * __s,char const * __accept) noexcept {
                                    return g_libc.strspn_fn (__s,__accept);
                                    }
                                     int strcasecmp (char const * __s1,char const * __s2) noexcept {
                                    return g_libc.strcasecmp_fn (__s1,__s2);
                                    }
                                     int strncasecmp (char const * __s1,char const * __s2,size_t __n) noexcept {
                                    return g_libc.strncasecmp_fn (__s1,__s2,__n);
                                    }
                                     char * strdup (char const * __s) noexcept {
                                    return dce_strdup (__s);
                                    }
                                     char * strndup (char const * __string,size_t __n) noexcept {
                                    return dce_strndup (__string,__n);
                                    }
                                     char * strsep (__restrict__ char * * __stringp,__restrict__ char const * __delim) noexcept {
                                    return g_libc.strsep_fn (__stringp,__delim);
                                    }
                                     char * setlocale (int __category,char const * __locale) noexcept {
                                    return dce_setlocale (__category,__locale);
                                    }
                                     __locale_t newlocale (int __category_mask,char const * __locale,__locale_t __base) noexcept {
                                    return g_libc.newlocale_fn (__category_mask,__locale,__base);
                                    }
                                    decltype (newlocale) __newlocale __attribute__ ((weak, alias ("newlocale")));
 __locale_t uselocale (__locale_t __dataset) noexcept {
                                    return g_libc.uselocale_fn (__dataset);
                                    }
                                    decltype (uselocale) __uselocale __attribute__ ((weak, alias ("uselocale")));
 int wctob (wint_t __c) noexcept {
                                    return g_libc.wctob_fn (__c);
                                    }
                                     wint_t btowc (int __c) noexcept {
                                    return g_libc.btowc_fn (__c);
                                    }
                                     size_t mbrlen (__restrict__ char const * __s,size_t __n,__restrict__ ::mbstate_t * __ps) noexcept {
                                    return g_libc.mbrlen_fn (__s,__n,__ps);
                                    }
                                     uint32_t htonl (uint32_t __hostlong) noexcept {
                                    return g_libc.htonl_fn (__hostlong);
                                    }
                                     uint16_t htons (uint16_t __hostshort) noexcept {
                                    return g_libc.htons_fn (__hostshort);
                                    }
                                     uint32_t ntohl (uint32_t __netlong) noexcept {
                                    return g_libc.ntohl_fn (__netlong);
                                    }
                                     uint16_t ntohs (uint16_t __netshort) noexcept {
                                    return g_libc.ntohs_fn (__netshort);
                                    }
                                     int lockf (int __fd,int __cmd,off_t __len)  {
                                    return g_libc.lockf_fn (__fd,__cmd,__len);
                                    }
                                     int inet_aton (char const * __cp,in_addr * __inp) noexcept {
                                    return g_libc.inet_aton_fn (__cp,__inp);
                                    }
                                     in_addr_t inet_addr (char const * __cp) noexcept {
                                    return g_libc.inet_addr_fn (__cp);
                                    }
                                     in_addr_t inet_network (char const * __cp) noexcept {
                                    return g_libc.inet_network_fn (__cp);
                                    }
                                     char * inet_ntoa (in_addr __in) noexcept {
                                    return g_libc.inet_ntoa_fn (__in);
                                    }
                                     in_addr inet_makeaddr (in_addr_t __net,in_addr_t __host) noexcept {
                                    return g_libc.inet_makeaddr_fn (__net,__host);
                                    }
                                     in_addr_t inet_lnaof (in_addr __in) noexcept {
                                    return g_libc.inet_lnaof_fn (__in);
                                    }
                                     in_addr_t inet_netof (in_addr __in) noexcept {
                                    return g_libc.inet_netof_fn (__in);
                                    }
                                     char const * inet_ntop (int __af,__restrict__ void const * __cp,__restrict__ char * __buf,socklen_t __len) noexcept {
                                    return dce_inet_ntop (__af,__cp,__buf,__len);
                                    }
                                     int inet_pton (int __af,__restrict__ char const * __cp,__restrict__ void * __buf) noexcept {
                                    return g_libc.inet_pton_fn (__af,__cp,__buf);
                                    }
                                     int inet6_opt_find (void * __extbuf,socklen_t __extlen,int __offset,uint8_t __type,socklen_t * __lenp,void * * __databufp) noexcept {
                                    return g_libc.inet6_opt_find_fn (__extbuf,__extlen,__offset,__type,__lenp,__databufp);
                                    }
                                     int socket (int __domain,int __type,int __protocol) noexcept {
                                    return dce_socket (__domain,__type,__protocol);
                                    }
                                     int socketpair (int __domain,int __type,int __protocol,int * __fds) noexcept {
                                    return dce_socketpair (__domain,__type,__protocol,__fds);
                                    }
                                     int getsockname (int __fd,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __len) noexcept {
                                    return dce_getsockname (__fd,__addr,__len);
                                    }
                                     int getpeername (int __fd,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __len) noexcept {
                                    return dce_getpeername (__fd,__addr,__len);
                                    }
                                     int bind (int __fd,sockaddr const * __addr,socklen_t __len) noexcept {
                                    return dce_bind (__fd,__addr,__len);
                                    }
                                     int connect (int __fd,sockaddr const * __addr,socklen_t __len)  {
                                    return dce_connect (__fd,__addr,__len);
                                    }
                                     int setsockopt (int __fd,int __level,int __optname,void const * __optval,socklen_t __optlen) noexcept {
                                    return dce_setsockopt (__fd,__level,__optname,__optval,__optlen);
                                    }
                                     int getsockopt (int __fd,int __level,int __optname,__restrict__ void * __optval,__restrict__ ::socklen_t * __optlen) noexcept {
                                    return dce_getsockopt (__fd,__level,__optname,__optval,__optlen);
                                    }
                                     int listen (int __fd,int __n) noexcept {
                                    return dce_listen (__fd,__n);
                                    }
                                     int accept (int __fd,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __addr_len)  {
                                    return dce_accept (__fd,__addr,__addr_len);
                                    }
                                     int shutdown (int __fd,int __how) noexcept {
                                    return dce_shutdown (__fd,__how);
                                    }
                                     ssize_t send (int __fd,void const * __buf,size_t __n,int __flags)  {
                                    return dce_send (__fd,__buf,__n,__flags);
                                    }
                                     ssize_t sendto (int __fd,void const * __buf,size_t __n,int __flags,sockaddr const * __addr,socklen_t __addr_len)  {
                                    return dce_sendto (__fd,__buf,__n,__flags,__addr,__addr_len);
                                    }
                                     ssize_t sendmsg (int __fd,msghdr const * __message,int __flags)  {
                                    return dce_sendmsg (__fd,__message,__flags);
                                    }
                                     ssize_t recv (int __fd,void * __buf,size_t __n,int __flags)  {
                                    return dce_recv (__fd,__buf,__n,__flags);
                                    }
                                     ssize_t recvfrom (int __fd,__restrict__ void * __buf,size_t __n,int __flags,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __addr_len)  {
                                    return dce_recvfrom (__fd,__buf,__n,__flags,__addr,__addr_len);
                                    }
                                     ssize_t recvmsg (int __fd,msghdr * __message,int __flags)  {
                                    return dce_recvmsg (__fd,__message,__flags);
                                    }
                                     int getnameinfo (__restrict__ ::sockaddr const * __sa,socklen_t __salen,__restrict__ char * __host,socklen_t __hostlen,__restrict__ char * __serv,socklen_t __servlen,int __flags)  {
                                    return dce_getnameinfo (__sa,__salen,__host,__hostlen,__serv,__servlen,__flags);
                                    }
                                     ssize_t read (int __fd,void * __buf,size_t __nbytes)  {
                                    return dce_read (__fd,__buf,__nbytes);
                                    }
                                     ssize_t write (int __fd,void const * __buf,size_t __n)  {
                                    return dce_write (__fd,__buf,__n);
                                    }
                                     unsigned int sleep (unsigned int __seconds)  {
                                    return dce_sleep (__seconds);
                                    }
                                     int usleep (__useconds_t __useconds)  {
                                    return dce_usleep (__useconds);
                                    }
                                     int getopt (int ___argc,char * const * ___argv,char const * __shortopts) noexcept {
                                    return dce_getopt (___argc,___argv,__shortopts);
                                    }
                                     int getopt_long (int ___argc,char * const * ___argv,char const * __shortopts,option const * __longopts,int * __longind) noexcept {
                                    return dce_getopt_long (___argc,___argv,__shortopts,__longopts,__longind);
                                    }
                                     __pid_t getpid () noexcept {
                                    return dce_getpid ();
                                    }
                                     __pid_t getppid () noexcept {
                                    return dce_getppid ();
                                    }
                                     __uid_t getuid () noexcept {
                                    return dce_getuid ();
                                    }
                                     __uid_t geteuid () noexcept {
                                    return dce_geteuid ();
                                    }
                                     int setuid (__uid_t __uid) noexcept {
                                    return dce_setuid (__uid);
                                    }
                                     int setgid (__gid_t __gid) noexcept {
                                    return dce_setgid (__gid);
                                    }
                                     int seteuid (__uid_t __uid) noexcept {
                                    return dce_seteuid (__uid);
                                    }
                                     int setegid (__gid_t __gid) noexcept {
                                    return dce_setegid (__gid);
                                    }
                                     int setreuid (__uid_t __ruid,__uid_t __euid) noexcept {
                                    return dce_setreuid (__ruid,__euid);
                                    }
                                     int setregid (__gid_t __rgid,__gid_t __egid) noexcept {
                                    return dce_setregid (__rgid,__egid);
                                    }
                                     int setresuid (__uid_t __ruid,__uid_t __euid,__uid_t __suid) noexcept {
                                    return dce_setresuid (__ruid,__euid,__suid);
                                    }
                                     int setresgid (__gid_t __rgid,__gid_t __egid,__gid_t __sgid) noexcept {
                                    return dce_setresgid (__rgid,__egid,__sgid);
                                    }
                                     int dup (int __fd) noexcept {
                                    return dce_dup (__fd);
                                    }
                                     int dup2 (int __fd,int __fd2) noexcept {
                                    return dce_dup2 (__fd,__fd2);
                                    }
                                     int close (int __fd)  {
                                    return dce_close (__fd);
                                    }
                                     int unlink (char const * __name) noexcept {
                                    return dce_unlink (__name);
                                    }
                                     int rmdir (char const * __path) noexcept {
                                    return dce_rmdir (__path);
                                    }
                                     int select (int __nfds,__restrict__ ::fd_set * __readfds,__restrict__ ::fd_set * __writefds,__restrict__ ::fd_set * __exceptfds,__restrict__ ::timeval * __timeout)  {
                                    return dce_select (__nfds,__readfds,__writefds,__exceptfds,__timeout);
                                    }
                                     int isatty (int __fd) noexcept {
                                    return dce_isatty (__fd);
                                    }
                                     void exit (int status) noexcept __attribute__ ((__noreturn__)) {
                                     dce_exit (status);
                                    }
                                     char * getcwd (char * __buf,size_t __size) noexcept {
                                    return dce_getcwd (__buf,__size);
                                    }
                                     char * getwd (char * __buf) noexcept {
                                    return dce_getwd (__buf);
                                    }
                                     char * get_current_dir_name () noexcept {
                                    return dce_get_current_dir_name ();
                                    }
                                     int chdir (char const * __path) noexcept {
                                    return dce_chdir (__path);
                                    }
                                     int fchdir (int __fd) noexcept {
                                    return dce_fchdir (__fd);
                                    }
                                     __pid_t fork () noexcept {
                                    return dce_fork ();
                                    }
                                     int execv (char const * __path,char * const * __argv) noexcept {
                                    return dce_execv (__path,__argv);
                                    }
                                     int execl (char const * __path,char const * __arg,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __arg);
        auto ret = dce_execl_v ( __path,__arg, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int execve (char const * __path,char * const * __argv,char * const * __envp) noexcept {
                                    return dce_execve (__path,__argv,__envp);
                                    }
                                     int execvp (char const * __file,char * const * __argv) noexcept {
                                    return dce_execvp (__file,__argv);
                                    }
                                     int execlp (char const * __file,char const * __arg,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __arg);
        auto ret = dce_execlp_v ( __file,__arg, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int execle (char const * __path,char const * __arg,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __arg);
        auto ret = dce_execle_v ( __path,__arg, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int truncate (char const * __file,__off_t __length) noexcept {
                                    return dce_truncate (__file,__length);
                                    }
                                     int ftruncate (int __fd,__off_t __length) noexcept {
                                    return dce_ftruncate (__fd,__length);
                                    }
                                     int ftruncate64 (int __fd,__off64_t __length) noexcept {
                                    return dce_ftruncate64 (__fd,__length);
                                    }
                                     long int sysconf (int __name) noexcept {
                                    return g_libc.sysconf_fn (__name);
                                    }
                                     char * ttyname (int __fd) noexcept {
                                    return dce_ttyname (__fd);
                                    }
                                     void * sbrk (intptr_t __delta) noexcept {
                                    return dce_sbrk (__delta);
                                    }
                                     int getpagesize () noexcept {
                                    return dce_getpagesize ();
                                    }
                                     __gid_t getgid () noexcept {
                                    return dce_getgid ();
                                    }
                                     __gid_t getegid () noexcept {
                                    return dce_getegid ();
                                    }
                                     int gethostname (char * __name,size_t __len) noexcept {
                                    return dce_gethostname (__name,__len);
                                    }
                                     __pid_t getpgrp () noexcept {
                                    return dce_getpgrp ();
                                    }
                                     __off_t lseek (int __fd,__off_t __offset,int __whence) noexcept {
                                    return dce_lseek (__fd,__offset,__whence);
                                    }
                                     __off64_t lseek64 (int __fd,__off64_t __offset,int __whence) noexcept {
                                    return dce_lseek64 (__fd,__offset,__whence);
                                    }
                                     int euidaccess (char const * __name,int __type) noexcept {
                                    return dce_euidaccess (__name,__type);
                                    }
                                     int eaccess (char const * __name,int __type) noexcept {
                                    return dce_eaccess (__name,__type);
                                    }
                                     int access (char const * __name,int __type) noexcept {
                                    return dce_access (__name,__type);
                                    }
                                     int pipe (int * __pipedes) noexcept {
                                    return dce_pipe (__pipedes);
                                    }
                                     long int pathconf (char const * __path,int __name) noexcept {
                                    return g_libc.pathconf_fn (__path,__name);
                                    }
                                     int getdtablesize () noexcept {
                                    return g_libc.getdtablesize_fn ();
                                    }
                                     ssize_t pread (int __fd,void * __buf,size_t __nbytes,__off_t __offset)  {
                                    return dce_pread (__fd,__buf,__nbytes,__offset);
                                    }
                                     ssize_t pwrite (int __fd,void const * __buf,size_t __n,__off_t __offset)  {
                                    return dce_pwrite (__fd,__buf,__n,__offset);
                                    }
                                     int daemon (int __nochdir,int __noclose) noexcept {
                                    return dce_daemon (__nochdir,__noclose);
                                    }
                                     unsigned int alarm (unsigned int __seconds) noexcept {
                                    return dce_alarm (__seconds);
                                    }
                                     ssize_t readlink (__restrict__ char const * __path,__restrict__ char * __buf,size_t __len) noexcept {
                                    return dce_readlink (__path,__buf,__len);
                                    }
                                     int chown (char const * __file,__uid_t __owner,__gid_t __group) noexcept {
                                    return dce_chown (__file,__owner,__group);
                                    }
                                     int initgroups (char const * __user,__gid_t __group)  {
                                    return dce_initgroups (__user,__group);
                                    }
                                     int fsync (int __fd)  {
                                    return dce_fsync (__fd);
                                    }
                                     ssize_t readv (int __fd,iovec const * __iovec,int __count)  {
                                    return dce_readv (__fd,__iovec,__count);
                                    }
                                     ssize_t writev (int __fd,iovec const * __iovec,int __count)  {
                                    return dce_writev (__fd,__iovec,__count);
                                    }
                                     void clearerr (FILE * __stream) noexcept {
                                     dce_clearerr (__stream);
                                    }
                                    decltype (clearerr) clearerr_unlocked __attribute__ ((weak, alias ("clearerr")));
decltype (clearerr)  __clearerr_unlocked __attribute__ ((weak, alias ("clearerr")));
 int setvbuf (__restrict__ ::FILE * __stream,__restrict__ char * __buf,int __modes,size_t __n) noexcept {
                                    return dce_setvbuf (__stream,__buf,__modes,__n);
                                    }
                                     void setbuf (__restrict__ ::FILE * __stream,__restrict__ char * __buf) noexcept {
                                     dce_setbuf (__stream,__buf);
                                    }
                                     void setbuffer (__restrict__ ::FILE * __stream,__restrict__ char * __buf,size_t __size) noexcept {
                                     dce_setbuffer (__stream,__buf,__size);
                                    }
                                     void setlinebuf (FILE * __stream) noexcept {
                                     dce_setlinebuf (__stream);
                                    }
                                     int fseek (FILE * __stream,long int __off,int __whence)  {
                                    return dce_fseek (__stream,__off,__whence);
                                    }
                                     long int ftell (FILE * __stream)  {
                                    return dce_ftell (__stream);
                                    }
                                     int fseeko (FILE * __stream,__off_t __off,int __whence)  {
                                    return dce_fseeko (__stream,__off,__whence);
                                    }
                                     __off_t ftello (FILE * __stream)  {
                                    return dce_ftello (__stream);
                                    }
                                     void rewind (FILE * __stream)  {
                                     dce_rewind (__stream);
                                    }
                                     int fgetpos (__restrict__ ::FILE * __stream,__restrict__ ::fpos_t * __pos)  {
                                    return dce_fgetpos (__stream,__pos);
                                    }
                                     int fsetpos (FILE * __stream,fpos_t const * __pos)  {
                                    return dce_fsetpos (__stream,__pos);
                                    }
                                     int printf (__restrict__ char const * __format,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __format);
        auto ret = dce_printf_v ( __format, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int fprintf (__restrict__ ::FILE * __stream,__restrict__ char const * __format,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __format);
        auto ret = vfprintf ( __stream,__format, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int sprintf (__restrict__ char * __s,__restrict__ char const * __format,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __format);
        auto ret = vsprintf ( __s,__format, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int asprintf (__restrict__ char * * __ptr,__restrict__ char const * __fmt,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __fmt);
        auto ret = dce_asprintf_v ( __ptr,__fmt, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int vasprintf (__restrict__ char * * __ptr,__restrict__ char const * __f,va_list __arg) noexcept {
                                    return dce_vasprintf (__ptr,__f,__arg);
                                    }
                                     int dprintf (int __fd,__restrict__ char const * __fmt,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __fmt);
        auto ret = vdprintf ( __fd,__fmt, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int vdprintf (int __fd,__restrict__ char const * __fmt,va_list __arg)  {
                                    return g_libc.vdprintf_fn (__fd,__fmt,__arg);
                                    }
                                     int fgetc (FILE * __stream)  {
                                    return dce_fgetc (__stream);
                                    }
                                    decltype (fgetc) fgetc_unlocked __attribute__ ((weak, alias ("fgetc")));
 int getc (FILE * __stream)  {
                                    return g_libc.getc_fn (__stream);
                                    }
                                     int getc_unlocked (FILE * __stream)  {
                                    return g_libc.getc_unlocked_fn (__stream);
                                    }
                                     int getchar ()  {
                                    return dce_getchar ();
                                    }
                                    decltype (getchar) getchar_unlocked __attribute__ ((weak, alias ("getchar")));
 int _IO_getc (_IO_FILE * __fp)  {
                                    return dce__IO_getc (__fp);
                                    }
                                     int fputc (int __c,FILE * __stream)  {
                                    return dce_fputc (__c,__stream);
                                    }
                                    decltype (fputc) fputc_unlocked __attribute__ ((weak, alias ("fputc")));
 int putc (int __c,FILE * __stream)  {
                                    return g_libc.putc_fn (__c,__stream);
                                    }
                                     int putc_unlocked (int __c,FILE * __stream)  {
                                    return g_libc.putc_unlocked_fn (__c,__stream);
                                    }
                                     int putchar (int __c)  {
                                    return dce_putchar (__c);
                                    }
                                    decltype (putchar) putchar_unlocked __attribute__ ((weak, alias ("putchar")));
 int _IO_putc (int __c,_IO_FILE * __fp)  {
                                    return dce__IO_putc (__c,__fp);
                                    }
                                     char * fgets (__restrict__ char * __s,int __n,__restrict__ ::FILE * __stream)  {
                                    return dce_fgets (__s,__n,__stream);
                                    }
                                    decltype (fgets) fgets_unlocked __attribute__ ((weak, alias ("fgets")));
 int fputs (__restrict__ char const * __s,__restrict__ ::FILE * __stream)  {
                                    return dce_fputs (__s,__stream);
                                    }
                                    decltype (fputs) fputs_unlocked __attribute__ ((weak, alias ("fputs")));
 int puts (char const * __s)  {
                                    return dce_puts (__s);
                                    }
                                     int ungetc (int __c,FILE * __stream)  {
                                    return dce_ungetc (__c,__stream);
                                    }
                                     int fclose (FILE * __stream)  {
                                    return dce_fclose (__stream);
                                    }
                                     int fcloseall ()  {
                                    return dce_fcloseall ();
                                    }
                                     FILE * fopen (__restrict__ char const * __filename,__restrict__ char const * __modes)  {
                                    return dce_fopen (__filename,__modes);
                                    }
                                     FILE * fopen64 (__restrict__ char const * __filename,__restrict__ char const * __modes)  {
                                    return dce_fopen64 (__filename,__modes);
                                    }
                                     FILE * freopen (__restrict__ char const * __filename,__restrict__ char const * __modes,__restrict__ ::FILE * __stream)  {
                                    return dce_freopen (__filename,__modes,__stream);
                                    }
                                     FILE * fdopen (int __fd,char const * __modes) noexcept {
                                    return dce_fdopen (__fd,__modes);
                                    }
                                     size_t fread (__restrict__ void * __ptr,size_t __size,size_t __n,__restrict__ ::FILE * __stream)  {
                                    return dce_fread (__ptr,__size,__n,__stream);
                                    }
                                    decltype (fread) fread_unlocked __attribute__ ((weak, alias ("fread")));
 size_t fwrite (__restrict__ void const * __ptr,size_t __size,size_t __n,__restrict__ ::FILE * __s)  {
                                    return dce_fwrite (__ptr,__size,__n,__s);
                                    }
                                    decltype (fwrite) fwrite_unlocked __attribute__ ((weak, alias ("fwrite")));
 int fflush (FILE * __stream)  {
                                    return dce_fflush (__stream);
                                    }
                                    decltype (fflush) fflush_unlocked __attribute__ ((weak, alias ("fflush")));
 int ferror (FILE * __stream) noexcept {
                                    return dce_ferror (__stream);
                                    }
                                    decltype (ferror) ferror_unlocked __attribute__ ((weak, alias ("ferror")));
 int feof (FILE * __stream) noexcept {
                                    return dce_feof (__stream);
                                    }
                                    decltype (feof) feof_unlocked __attribute__ ((weak, alias ("feof")));
 int fileno (FILE * __stream) noexcept {
                                    return dce_fileno (__stream);
                                    }
                                    decltype (fileno) fileno_unlocked __attribute__ ((weak, alias ("fileno")));
 void perror (char const * __s)  {
                                     dce_perror (__s);
                                    }
                                     int remove (char const * __filename) noexcept {
                                    return dce_remove (__filename);
                                    }
                                     int sscanf (__restrict__ char const * __s,__restrict__ char const * __format,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __format);
        auto ret = vsscanf ( __s,__format, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

            decltype (sscanf) __isoc99_sscanf __attribute__ ((weak, alias ("sscanf")));
 void flockfile (FILE * __stream) noexcept {
                                     g_libc.flockfile_fn (__stream);
                                    }
                                     void funlockfile (FILE * __stream) noexcept {
                                     g_libc.funlockfile_fn (__stream);
                                    }
                                     int vprintf (__restrict__ char const * __format,va_list __arg)  {
                                    return dce_vprintf (__format,__arg);
                                    }
                                     int vfprintf (__restrict__ ::FILE * __s,__restrict__ char const * __format,va_list __arg)  {
                                    return g_libc.vfprintf_fn (__s,__format,__arg);
                                    }
                                     int vsprintf (__restrict__ char * __s,__restrict__ char const * __format,va_list __arg) noexcept {
                                    return g_libc.vsprintf_fn (__s,__format,__arg);
                                    }
                                     int fcntl (int __fd,int __cmd,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __cmd);
        auto ret = dce_fcntl_v ( __fd,__cmd, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int open (char const * __file,int __oflag,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __oflag);
        auto ret = dce_open_v ( __file,__oflag, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int open64 (char const * __file,int __oflag,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __oflag);
        auto ret = dce_open64_v ( __file,__oflag, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int unlinkat (int __fd,char const * __name,int __flag) noexcept {
                                    return dce_unlinkat (__fd,__name,__flag);
                                    }
                                     int nanosleep (timespec const * __requested_time,timespec * __remaining)  {
                                    return dce_nanosleep (__requested_time,__remaining);
                                    }
                                     char * asctime (tm const * __tp) noexcept {
                                    return dce_asctime (__tp);
                                    }
                                     char * asctime_r (__restrict__ ::tm const * __tp,__restrict__ char * __buf) noexcept {
                                    return g_libc.asctime_r_fn (__tp,__buf);
                                    }
                                     char * ctime (time_t const * __timer) noexcept {
                                    return dce_ctime (__timer);
                                    }
                                     char * ctime_r (__restrict__ ::time_t const * __timer,__restrict__ char * __buf) noexcept {
                                    return g_libc.ctime_r_fn (__timer,__buf);
                                    }
                                     tm * gmtime (time_t const * __timer) noexcept {
                                    return dce_gmtime (__timer);
                                    }
                                    decltype (gmtime) localtime __attribute__ ((weak, alias ("gmtime")));
 tm * gmtime_r (__restrict__ ::time_t const * __timer,__restrict__ ::tm * __tp) noexcept {
                                    return g_libc.gmtime_r_fn (__timer,__tp);
                                    }
                                    decltype (gmtime_r) localtime_r __attribute__ ((weak, alias ("gmtime_r")));
 time_t mktime (tm * __tp) noexcept {
                                    return g_libc.mktime_fn (__tp);
                                    }
                                     size_t strftime (__restrict__ char * __s,size_t __maxsize,__restrict__ char const * __format,__restrict__ ::tm const * __tp) noexcept {
                                    return g_libc.strftime_fn (__s,__maxsize,__format,__tp);
                                    }
                                     char * strptime (__restrict__ char const * __s,__restrict__ char const * __fmt,tm * __tp) noexcept {
                                    return g_libc.strptime_fn (__s,__fmt,__tp);
                                    }
                                     time_t timegm (tm * __tp) noexcept {
                                    return g_libc.timegm_fn (__tp);
                                    }
                                     time_t timelocal (tm * __tp) noexcept {
                                    return g_libc.timelocal_fn (__tp);
                                    }
                                     int clock_gettime (clockid_t __clock_id,timespec * __tp) noexcept {
                                    return dce_clock_gettime (__clock_id,__tp);
                                    }
                                     int clock_getres (clockid_t __clock_id,timespec * __res) noexcept {
                                    return dce_clock_getres (__clock_id,__res);
                                    }
                                     int timer_create (clockid_t __clock_id,__restrict__ ::sigevent * __evp,__restrict__ ::timer_t * __timerid) noexcept {
                                    return dce_timer_create (__clock_id,__evp,__timerid);
                                    }
                                     int timer_settime (timer_t __timerid,int __flags,__restrict__ ::itimerspec const * __value,__restrict__ ::itimerspec * __ovalue) noexcept {
                                    return dce_timer_settime (__timerid,__flags,__value,__ovalue);
                                    }
                                     int timer_gettime (timer_t __timerid,itimerspec * __value) noexcept {
                                    return dce_timer_gettime (__timerid,__value);
                                    }
                                     int utime (char const * __file,utimbuf const * __file_times) noexcept {
                                    return dce_utime (__file,__file_times);
                                    }
                                     void tzset () noexcept {
                                     dce_tzset ();
                                    }
                                     int gettimeofday (__restrict__ ::timeval * __tv,__timezone_ptr_t __tz) noexcept {
                                    return dce_gettimeofday (__tv,__tz);
                                    }
                                     time_t time (time_t * __timer) noexcept {
                                    return dce_time (__timer);
                                    }
                                     int setitimer (__itimer_which_t __which,__restrict__ ::itimerval const * __new,__restrict__ ::itimerval * __old) noexcept {
                                    return dce_setitimer (__which,__new,__old);
                                    }
                                     int getitimer (__itimer_which_t __which,itimerval * __value) noexcept {
                                    return dce_getitimer (__which,__value);
                                    }
                                     int sysinfo (struct sysinfo *info) noexcept {
                                    return dce_sysinfo (info);
                                    }
                                     void * mmap (void * __addr,size_t __len,int __prot,int __flags,int __fd,__off_t __offset) noexcept {
                                    return dce_mmap (__addr,__len,__prot,__flags,__fd,__offset);
                                    }
                                     void * mmap64 (void * __addr,size_t __len,int __prot,int __flags,int __fd,__off64_t __offset) noexcept {
                                    return dce_mmap64 (__addr,__len,__prot,__flags,__fd,__offset);
                                    }
                                     int munmap (void * __addr,size_t __len) noexcept {
                                    return dce_munmap (__addr,__len);
                                    }
                                     int mkdir (char const * __path,__mode_t __mode) noexcept {
                                    return dce_mkdir (__path,__mode);
                                    }
                                     __mode_t umask (__mode_t __mask) noexcept {
                                    return dce_umask (__mask);
                                    }
                                     int ioctl (int __fd,long unsigned int __request,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __request);
        auto ret = dce_ioctl_v ( __fd,__request, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int sched_yield () noexcept {
                                    return dce_sched_yield ();
                                    }
                                     int poll (pollfd * __fds,nfds_t __nfds,int __timeout)  {
                                    return dce_poll (__fds,__nfds,__timeout);
                                    }
                                     __sighandler_t signal (int __sig,__sighandler_t __handler) noexcept {
                                    return dce_signal (__sig,__handler);
                                    }
                                     int sigaction (int signum,const struct sigaction *act,struct sigaction *oldact) noexcept {
                                    return dce_sigaction (signum, act, oldact);
                                    }
                                     int sigemptyset (sigset_t * __set) noexcept {
                                    return g_libc.sigemptyset_fn (__set);
                                    }
                                     int sigfillset (sigset_t * __set) noexcept {
                                    return g_libc.sigfillset_fn (__set);
                                    }
                                     int sigaddset (sigset_t * __set,int __signo) noexcept {
                                    return g_libc.sigaddset_fn (__set,__signo);
                                    }
                                     int sigdelset (sigset_t * __set,int __signo) noexcept {
                                    return g_libc.sigdelset_fn (__set,__signo);
                                    }
                                     int sigismember (sigset_t const * __set,int __signo) noexcept {
                                    return g_libc.sigismember_fn (__set,__signo);
                                    }
                                     int sigprocmask (int __how,__restrict__ ::sigset_t const * __set,__restrict__ ::sigset_t * __oset) noexcept {
                                    return dce_sigprocmask (__how,__set,__oset);
                                    }
                                     int sigwait (__restrict__ ::sigset_t const * __set,__restrict__ int * __sig)  {
                                    return dce_sigwait (__set,__sig);
                                    }
                                     int kill (__pid_t __pid,int __sig) noexcept {
                                    return dce_kill (__pid,__sig);
                                    }
                                     int pthread_create (__restrict__ ::pthread_t * __newthread,__restrict__ ::pthread_attr_t const * __attr,void * (*)( void * ) ,__restrict__ void * __arg) noexcept {
                                    return dce_pthread_create (__newthread,__attr,__start_routine,__arg);
                                    }
                                     void pthread_exit (void *retval) __attribute__ ((__noreturn__)) {
                                     dce_pthread_exit (retval);
                                    }
                                     pthread_t pthread_self () noexcept {
                                    return dce_pthread_self ();
                                    }
                                     int pthread_once (pthread_once_t * __once_control,void (*)(  ) )  {
                                    return dce_pthread_once (__once_control,__init_routine);
                                    }
                                    decltype (pthread_once) __pthread_once __attribute__ ((weak, alias ("pthread_once")));
 void * pthread_getspecific (pthread_key_t __key) noexcept {
                                    return dce_pthread_getspecific (__key);
                                    }
                                     int pthread_setspecific (pthread_key_t __key,void const * __pointer) noexcept {
                                    return dce_pthread_setspecific (__key,__pointer);
                                    }
                                     int pthread_key_create (pthread_key_t * __key,void (*)( void * ) ) noexcept {
                                    return dce_pthread_key_create (__key,__destr_function);
                                    }
                                    decltype (pthread_key_create) __pthread_key_create __attribute__ ((weak, alias ("pthread_key_create")));
 int pthread_key_delete (pthread_key_t __key) noexcept {
                                    return dce_pthread_key_delete (__key);
                                    }
                                     int pthread_mutex_destroy (pthread_mutex_t * __mutex) noexcept {
                                    return dce_pthread_mutex_destroy (__mutex);
                                    }
                                     int pthread_mutex_init (pthread_mutex_t * __mutex,pthread_mutexattr_t const * __mutexattr) noexcept {
                                    return dce_pthread_mutex_init (__mutex,__mutexattr);
                                    }
                                     int pthread_mutex_lock (pthread_mutex_t * __mutex) noexcept {
                                    return dce_pthread_mutex_lock (__mutex);
                                    }
                                     int pthread_mutex_unlock (pthread_mutex_t * __mutex) noexcept {
                                    return dce_pthread_mutex_unlock (__mutex);
                                    }
                                     int pthread_mutex_trylock (pthread_mutex_t * __mutex) noexcept {
                                    return dce_pthread_mutex_trylock (__mutex);
                                    }
                                     int pthread_mutexattr_init (pthread_mutexattr_t * __attr) noexcept {
                                    return dce_pthread_mutexattr_init (__attr);
                                    }
                                     int pthread_mutexattr_destroy (pthread_mutexattr_t * __attr) noexcept {
                                    return dce_pthread_mutexattr_destroy (__attr);
                                    }
                                     int pthread_mutexattr_settype (pthread_mutexattr_t * __attr,int __kind) noexcept {
                                    return dce_pthread_mutexattr_settype (__attr,__kind);
                                    }
                                     int pthread_cancel (pthread_t __th)  {
                                    return dce_pthread_cancel (__th);
                                    }
                                     int pthread_kill (pthread_t thread, int sig) noexcept {
                                    return dce_pthread_kill (thread,sig);
                                    }
                                     int pthread_join (pthread_t __th,void * * __thread_return)  {
                                    return dce_pthread_join (__th,__thread_return);
                                    }
                                     int pthread_detach (pthread_t __th) noexcept {
                                    return dce_pthread_detach (__th);
                                    }
                                     int pthread_cond_destroy (pthread_cond_t * __cond) noexcept {
                                    return dce_pthread_cond_destroy (__cond);
                                    }
                                     int pthread_cond_init (__restrict__ ::pthread_cond_t * __cond,__restrict__ ::pthread_condattr_t const * __cond_attr) noexcept {
                                    return dce_pthread_cond_init (__cond,__cond_attr);
                                    }
                                     int pthread_cond_broadcast (pthread_cond_t * __cond) noexcept {
                                    return dce_pthread_cond_broadcast (__cond);
                                    }
                                     int pthread_cond_signal (pthread_cond_t * __cond) noexcept {
                                    return dce_pthread_cond_signal (__cond);
                                    }
                                     int pthread_cond_timedwait (__restrict__ ::pthread_cond_t * __cond,__restrict__ ::pthread_mutex_t * __mutex,__restrict__ ::timespec const * __abstime)  {
                                    return dce_pthread_cond_timedwait (__cond,__mutex,__abstime);
                                    }
                                     int pthread_cond_wait (__restrict__ ::pthread_cond_t * __cond,__restrict__ ::pthread_mutex_t * __mutex)  {
                                    return dce_pthread_cond_wait (__cond,__mutex);
                                    }
                                     int pthread_condattr_destroy (pthread_condattr_t * __attr) noexcept {
                                    return dce_pthread_condattr_destroy (__attr);
                                    }
                                     int pthread_condattr_init (pthread_condattr_t * __attr) noexcept {
                                    return dce_pthread_condattr_init (__attr);
                                    }
                                     int pthread_rwlock_init (__restrict__ ::pthread_rwlock_t * __rwlock,__restrict__ ::pthread_rwlockattr_t const * __attr) noexcept {
                                    return g_libc.pthread_rwlock_init_fn (__rwlock,__attr);
                                    }
                                     int pthread_rwlock_unlock (pthread_rwlock_t * __rwlock) noexcept {
                                    return g_libc.pthread_rwlock_unlock_fn (__rwlock);
                                    }
                                     int pthread_rwlock_wrlock (pthread_rwlock_t * __rwlock) noexcept {
                                    return g_libc.pthread_rwlock_wrlock_fn (__rwlock);
                                    }
                                     int pthread_rwlock_rdlock (pthread_rwlock_t * __rwlock) noexcept {
                                    return g_libc.pthread_rwlock_rdlock_fn (__rwlock);
                                    }
                                     int pthread_rwlock_destroy (pthread_rwlock_t * __rwlock) noexcept {
                                    return g_libc.pthread_rwlock_destroy_fn (__rwlock);
                                    }
                                     int pthread_setcancelstate (int __state,int * __oldstate)  {
                                    return g_libc.pthread_setcancelstate_fn (__state,__oldstate);
                                    }
                                     int pthread_sigmask (int __how,__restrict__ ::__sigset_t const * __newmask,__restrict__ ::__sigset_t * __oldmask) noexcept {
                                    return g_libc.pthread_sigmask_fn (__how,__newmask,__oldmask);
                                    }
                                     int pthread_equal (pthread_t __thread1,pthread_t __thread2) noexcept {
                                    return g_libc.pthread_equal_fn (__thread1,__thread2);
                                    }
                                     int pthread_spin_init (pthread_spinlock_t * __lock,int __pshared) noexcept {
                                    return g_libc.pthread_spin_init_fn (__lock,__pshared);
                                    }
                                     int pthread_spin_lock (pthread_spinlock_t * __lock) noexcept {
                                    return g_libc.pthread_spin_lock_fn (__lock);
                                    }
                                     int pthread_spin_unlock (pthread_spinlock_t * __lock) noexcept {
                                    return g_libc.pthread_spin_unlock_fn (__lock);
                                    }
                                     int pthread_spin_destroy (pthread_spinlock_t * __lock) noexcept {
                                    return g_libc.pthread_spin_destroy_fn (__lock);
                                    }
                                     int sem_init (sem_t * __sem,int __pshared,unsigned int __value) noexcept {
                                    return dce_sem_init (__sem,__pshared,__value);
                                    }
                                     int sem_destroy (sem_t * __sem) noexcept {
                                    return dce_sem_destroy (__sem);
                                    }
                                     int sem_post (sem_t * __sem) noexcept {
                                    return dce_sem_post (__sem);
                                    }
                                     int sem_wait (sem_t * __sem)  {
                                    return dce_sem_wait (__sem);
                                    }
                                     int sem_timedwait (__restrict__ ::sem_t * __sem,__restrict__ ::timespec const * __abstime)  {
                                    return dce_sem_timedwait (__sem,__abstime);
                                    }
                                     int sem_trywait (sem_t * __sem) noexcept {
                                    return dce_sem_trywait (__sem);
                                    }
                                     int sem_getvalue (__restrict__ ::sem_t * __sem,__restrict__ int * __sval) noexcept {
                                    return dce_sem_getvalue (__sem,__sval);
                                    }
                                     hostent * gethostbyname (char const * __name)  {
                                    return dce_gethostbyname (__name);
                                    }
                                     hostent * gethostbyname2 (char const * __name,int __af)  {
                                    return dce_gethostbyname2 (__name,__af);
                                    }
                                     int getaddrinfo (__restrict__ char const * __name,__restrict__ char const * __service,__restrict__ ::addrinfo const * __req,__restrict__ ::addrinfo * * __pai)  {
                                    return dce_getaddrinfo (__name,__service,__req,__pai);
                                    }
                                     void freeaddrinfo (addrinfo * __ai) noexcept {
                                     dce_freeaddrinfo (__ai);
                                    }
                                     char const * gai_strerror (int __ecode) noexcept {
                                    return dce_gai_strerror (__ecode);
                                    }
                                     int getifaddrs (ifaddrs * * __ifap) noexcept {
                                    return dce_getifaddrs (__ifap);
                                    }
                                     void freeifaddrs (ifaddrs * __ifa) noexcept {
                                     g_libc.freeifaddrs_fn (__ifa);
                                    }
                                     hostent * gethostent ()  {
                                    return g_libc.gethostent_fn ();
                                    }
                                     void sethostent (int __stay_open)  {
                                     g_libc.sethostent_fn (__stay_open);
                                    }
                                     void endhostent ()  {
                                     g_libc.endhostent_fn ();
                                    }
                                     void herror (char const * __str) noexcept {
                                     dce_herror (__str);
                                    }
                                     char const * hstrerror (int __err_num) noexcept {
                                    return g_libc.hstrerror_fn (__err_num);
                                    }
                                     protoent * getprotoent ()  {
                                    return g_libc.getprotoent_fn ();
                                    }
                                     protoent * getprotobyname (char const * __name)  {
                                    return g_libc.getprotobyname_fn (__name);
                                    }
                                     protoent * getprotobynumber (int __proto)  {
                                    return g_libc.getprotobynumber_fn (__proto);
                                    }
                                     void setprotoent (int __stay_open)  {
                                     g_libc.setprotoent_fn (__stay_open);
                                    }
                                     void endprotoent ()  {
                                     g_libc.endprotoent_fn ();
                                    }
                                     servent * getservent ()  {
                                    return g_libc.getservent_fn ();
                                    }
                                     servent * getservbyname (char const * __name,char const * __proto)  {
                                    return g_libc.getservbyname_fn (__name,__proto);
                                    }
                                     servent * getservbyport (int __port,char const * __proto)  {
                                    return g_libc.getservbyport_fn (__port,__proto);
                                    }
                                     void setservent (int __stay_open)  {
                                     g_libc.setservent_fn (__stay_open);
                                    }
                                     void endservent ()  {
                                     g_libc.endservent_fn ();
                                    }
                                     int toupper (int __c) noexcept {
                                    return g_libc.toupper_fn (__c);
                                    }
                                     int tolower (int __c) noexcept {
                                    return g_libc.tolower_fn (__c);
                                    }
                                     int isdigit (int arg0) noexcept {
                                    return g_libc.isdigit_fn (arg0);
                                    }
                                     int isxdigit (int arg0) noexcept {
                                    return g_libc.isxdigit_fn (arg0);
                                    }
                                     int isalnum (int arg0) noexcept {
                                    return g_libc.isalnum_fn (arg0);
                                    }
                                     int timerfd_create (clockid_t __clock_id,int __flags) noexcept {
                                    return dce_timerfd_create (__clock_id,__flags);
                                    }
                                     int timerfd_settime (int __ufd,int __flags,itimerspec const * __utmr,itimerspec * __otmr) noexcept {
                                    return dce_timerfd_settime (__ufd,__flags,__utmr,__otmr);
                                    }
                                     int timerfd_gettime (int __ufd,itimerspec * __otmr) noexcept {
                                    return dce_timerfd_gettime (__ufd,__otmr);
                                    }
                                     unsigned int if_nametoindex (char const * __ifname) noexcept {
                                    return dce_if_nametoindex (__ifname);
                                    }
                                     char * if_indextoname (unsigned int __ifindex,char * __ifname) noexcept {
                                    return dce_if_indextoname (__ifindex,__ifname);
                                    }
                                     DIR * opendir (char const * __name)  {
                                    return dce_opendir (__name);
                                    }
                                     DIR * fdopendir (int __fd)  {
                                    return dce_fdopendir (__fd);
                                    }
                                     dirent * readdir (DIR * __dirp)  {
                                    return dce_readdir (__dirp);
                                    }
                                     int readdir_r (__restrict__ ::DIR * __dirp,__restrict__ ::dirent * __entry,__restrict__ ::dirent * * __result)  {
                                    return dce_readdir_r (__dirp,__entry,__result);
                                    }
                                     int closedir (DIR * __dirp)  {
                                    return dce_closedir (__dirp);
                                    }
                                     int dirfd (DIR * __dirp) noexcept {
                                    return dce_dirfd (__dirp);
                                    }
                                     void rewinddir (DIR * __dirp) noexcept {
                                     dce_rewinddir (__dirp);
                                    }
                                     int scandir (__restrict__ char const * __dir,__restrict__ ::dirent * * * __namelist,int (*)( ::dirent const * ) ,int (*)( ::dirent const * *,::dirent const * * ) )  {
                                    return dce_scandir (__dir,__namelist,__selector,__cmp);
                                    }
                                     int alphasort (dirent const * * __e1,dirent const * * __e2) noexcept {
                                    return g_libc.alphasort_fn (__e1,__e2);
                                    }
                                     int alphasort64 (dirent64 const * * __e1,dirent64 const * * __e2) noexcept {
                                    return g_libc.alphasort64_fn (__e1,__e2);
                                    }
                                     int versionsort (dirent const * * __e1,dirent const * * __e2) noexcept {
                                    return g_libc.versionsort_fn (__e1,__e2);
                                    }
                                     int uname (struct utsname *__name) noexcept {
                                    return dce_uname (__name);
                                    }
                                     pid_t wait (void *stat_loc)  {
                                    return dce_wait (stat_loc);
                                    }
                                     __pid_t waitpid (__pid_t __pid,int * __stat_loc,int __options)  {
                                    return dce_waitpid (__pid,__stat_loc,__options);
                                    }
                                     char * dirname (char * __path) noexcept {
                                    return g_libc.dirname_fn (__path);
                                    }
                                     char * __xpg_basename (char * __path) noexcept {
                                    return g_libc.__xpg_basename_fn (__path);
                                    }
                                     group * getgrnam (char const * __name)  {
                                    return g_libc.getgrnam_fn (__name);
                                    }
                                     int getrusage (__rusage_who_t __who,rusage * __usage) noexcept {
                                    return g_libc.getrusage_fn (__who,__usage);
                                    }
                                     int getrlimit (__rlimit_resource_t __resource,rlimit * __rlimits) noexcept {
                                    return g_libc.getrlimit_fn (__resource,__rlimits);
                                    }
                                     int setrlimit (__rlimit_resource_t __resource,rlimit const * __rlimits) noexcept {
                                    return g_libc.setrlimit_fn (__resource,__rlimits);
                                    }
                                     void openlog (char const * __ident,int __option,int __facility)  {
                                     dce_openlog (__ident,__option,__facility);
                                    }
                                     void closelog ()  {
                                     dce_closelog ();
                                    }
                                     int setlogmask (int __mask) noexcept {
                                    return dce_setlogmask (__mask);
                                    }
                                     void syslog (int __pri,char const * __fmt,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __fmt);
         dce_syslog_v ( __pri,__fmt, __dce_va_list);
        va_end (__dce_va_list);
        
            };

             void vsyslog (int __pri,char const * __fmt,va_list __ap)  {
                                     dce_vsyslog (__pri,__fmt,__ap);
                                    }
                                     int _setjmp (__jmp_buf_tag * __env) noexcept {
                                    return g_libc._setjmp_fn (__env);
                                    }
                                     int __sigsetjmp (__jmp_buf_tag * __env,int __savemask) noexcept {
                                    return g_libc.__sigsetjmp_fn (__env,__savemask);
                                    }
                                     void siglongjmp (__jmp_buf_tag * __env,int __val) noexcept {
                                     g_libc.siglongjmp_fn (__env,__val);
                                    }
                                     char * bindtextdomain (char const * __domainname,char const * __dirname) noexcept {
                                    return g_libc.bindtextdomain_fn (__domainname,__dirname);
                                    }
                                     char * textdomain (char const * __domainname) noexcept {
                                    return g_libc.textdomain_fn (__domainname);
                                    }
                                     char * gettext (char const * __msgid) noexcept {
                                    return g_libc.gettext_fn (__msgid);
                                    }
                                     nl_catd catopen (char const * __cat_name,int __flag)  {
                                    return g_libc.catopen_fn (__cat_name,__flag);
                                    }
                                     char * catgets (nl_catd __catalog,int __set,int __number,char const * __string) noexcept {
                                    return g_libc.catgets_fn (__catalog,__set,__number,__string);
                                    }
                                     passwd * getpwnam (char const * __name)  {
                                    return g_libc.getpwnam_fn (__name);
                                    }
                                     passwd * getpwuid (__uid_t __uid)  {
                                    return dce_getpwuid (__uid);
                                    }
                                     void endpwent ()  {
                                     dce_endpwent ();
                                    }
                                     intmax_t strtoimax (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept {
                                    return g_libc.strtoimax_fn (__nptr,__endptr,__base);
                                    }
                                     uintmax_t strtoumax (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept {
                                    return g_libc.strtoumax_fn (__nptr,__endptr,__base);
                                    }
                                     ether_addr * ether_aton_r (char const * __asc,ether_addr * __addr) noexcept {
                                    return g_libc.ether_aton_r_fn (__asc,__addr);
                                    }
                                     ether_addr * ether_aton (char const * __asc) noexcept {
                                    return g_libc.ether_aton_fn (__asc);
                                    }
                                     void * tsearch (void const * __key,void * * __rootp,__compar_fn_t __compar)  {
                                    return g_libc.tsearch_fn (__key,__rootp,__compar);
                                    }
                                     void * tfind (void const * __key,void * const * __rootp,__compar_fn_t __compar)  {
                                    return g_libc.tfind_fn (__key,__rootp,__compar);
                                    }
                                     void * tdelete (__restrict__ void const * __key,__restrict__ void * * __rootp,__compar_fn_t __compar)  {
                                    return g_libc.tdelete_fn (__key,__rootp,__compar);
                                    }
                                     void twalk (void const * __root,__action_fn_t __action)  {
                                     g_libc.twalk_fn (__root,__action);
                                    }
                                     void tdestroy (void * __root,__free_fn_t __freefct)  {
                                     g_libc.tdestroy_fn (__root,__freefct);
                                    }
                                     int fnmatch (char const * __pattern,char const * __name,int __flags)  {
                                    return g_libc.fnmatch_fn (__pattern,__name,__flags);
                                    }
                                     char * nl_langinfo (nl_item __item) noexcept {
                                    return g_libc.nl_langinfo_fn (__item);
                                    }
                                     int fstatfs (int __fildes, struct statfs * __buf) noexcept {
                                    return dce_fstatfs (__fildes, __buf);
                                    }
                                     int fstatfs64 (int __fildes, struct statfs64 * __buf) noexcept {
                                    return dce_fstatfs64 (__fildes, __buf);
                                    }
                                     int statfs (const char *path, struct statfs *buf) noexcept {
                                    return dce_statfs (path, buf);
                                    }
                                     int statfs64 (const char *path, struct statfs64 *buf) noexcept {
                                    return dce_statfs64 (path, buf);
                                    }
                                     int statvfs (const char *path, struct statvfs *buf) noexcept {
                                    return dce_statvfs (path, buf);
                                    }
                                     int fstatvfs (int __fildes, struct statvfs * __buf) noexcept {
                                    return dce_fstatvfs (__fildes, __buf);
                                    }
                                     int tcgetattr (int __fd,termios * __termios_p) noexcept {
                                    return dce_tcgetattr (__fd,__termios_p);
                                    }
                                     int tcsetattr (int __fd,int __optional_actions,termios const * __termios_p) noexcept {
                                    return dce_tcsetattr (__fd,__optional_actions,__termios_p);
                                    }
                                     short unsigned int const * * __ctype_b_loc () noexcept {
                                    return g_libc.__ctype_b_loc_fn ();
                                    }
                                     wctype_t wctype_l (char const * __property,__locale_t __locale) noexcept {
                                    return g_libc.wctype_l_fn (__property,__locale);
                                    }
                                    decltype (wctype_l) __wctype_l __attribute__ ((weak, alias ("wctype_l")));
 __int32_t const * * __ctype_tolower_loc () noexcept {
                                    return g_libc.__ctype_tolower_loc_fn ();
                                    }
                                     size_t __ctype_get_mb_cur_max () noexcept {
                                    return g_libc.__ctype_get_mb_cur_max_fn ();
                                    }
                                     void __fpurge (FILE *fd)  {
                                     g_libc.__fpurge_fn (fd);
                                    }
                                     size_t __fpending (FILE *fd)  {
                                    return g_libc.__fpending_fn (fd);
                                    }
                                     int __printf_chk (int __flag,__restrict__ char const * __format,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __format);
        auto ret = dce___printf_chk_v ( __flag,__format, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int __vfprintf_chk (__restrict__ ::FILE * __stream,int __flag,__restrict__ char const * __format,va_list __ap)  {
                                    return dce___vfprintf_chk (__stream,__flag,__format,__ap);
                                    }
                                     int __fprintf_chk (__restrict__ ::FILE * __stream,int __flag,__restrict__ char const * __format,... )   {
        va_list __dce_va_list;
        va_start (__dce_va_list, __format);
        auto ret = dce___fprintf_chk_v ( __stream,__flag,__format, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int __snprintf_chk (__restrict__ char * __s,size_t __n,int __flag,size_t __slen,__restrict__ char const * __format,... ) noexcept  {
        va_list __dce_va_list;
        va_start (__dce_va_list, __format);
        auto ret = dce___snprintf_chk_v ( __s,__n,__flag,__slen,__format, __dce_va_list);
        va_end (__dce_va_list);
        return ret;
            };

             int * __errno_location () noexcept {
                                    return dce___errno_location ();
                                    }
                                     int * __h_errno_location () noexcept {
                                    return dce___h_errno_location ();
                                    }
                                     int __vsnprintf_chk (__restrict__ char * __s,size_t __n,int __flag,size_t __slen,__restrict__ char const * __format,va_list __ap) noexcept {
                                    return dce___vsnprintf_chk (__s,__n,__flag,__slen,__format,__ap);
                                    }
                                     int __xstat (int __ver,char const * __filename,stat * __stat_buf) noexcept {
                                    return dce___xstat (__ver,__filename,__stat_buf);
                                    }
                                     int __lxstat (int __ver,char const * __filename,stat * __stat_buf) noexcept {
                                    return dce___lxstat (__ver,__filename,__stat_buf);
                                    }
                                     int __fxstat (int __ver,int __fildes,stat * __stat_buf) noexcept {
                                    return dce___fxstat (__ver,__fildes,__stat_buf);
                                    }
                                     int __xstat64 (int __ver,char const * __filename,stat64 * __stat_buf) noexcept {
                                    return dce___xstat64 (__ver,__filename,__stat_buf);
                                    }
                                     int __lxstat64 (int __ver,char const * __filename,stat64 * __stat_buf) noexcept {
                                    return dce___lxstat64 (__ver,__filename,__stat_buf);
                                    }
                                     int __fxstat64 (int __ver,int __fildes,stat64 * __stat_buf) noexcept {
                                    return dce___fxstat64 (__ver,__fildes,__stat_buf);
                                    }
                                     int __fxstatat (int __ver,int __fildes,char const * __filename,stat * __stat_buf,int __flag) noexcept {
                                    return dce___fxstatat (__ver,__fildes,__filename,__stat_buf,__flag);
                                    }
                                     cmsghdr * __cmsg_nxthdr (msghdr * __mhdr,cmsghdr * __cmsg) noexcept {
                                    return g_libc.__cmsg_nxthdr_fn (__mhdr,__cmsg);
                                    }
                                     int fstat64 (int __fd,struct stat64 *__buf) noexcept {
                                    return dce_fstat64 (__fd,__buf);
                                    }
                                     int creat (char const * __file,mode_t __mode)  {
                                    return dce_creat (__file,__mode);
                                    }
                                     long int lrintl (long double __x) noexcept {
                                    return g_libc.lrintl_fn (__x);
                                    }
                                     long long int llrintl (long double __x) noexcept {
                                    return g_libc.llrintl_fn (__x);
                                    }
                                     void * dlopen (char const * __file,int __mode) noexcept {
                                    return dce_dlopen (__file,__mode);
                                    }
                                    decltype (dlopen) __dlopen __attribute__ ((weak, alias ("dlopen")));
 void * dlsym (__restrict__ void * __handle,__restrict__ char const * __name) noexcept {
                                    return dce_dlsym (__handle,__name);
                                    }
                                     int dl_iterate_phdr (int (*)( ::dl_phdr_info *,::size_t,void * ) ,void * __data)  {
                                    return g_libc.dl_iterate_phdr_fn (__callback,__data);
                                    }
                                    