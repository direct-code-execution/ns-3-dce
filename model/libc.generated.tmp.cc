#define __restrict__ 
                     void __cxa_finalize (void *d) {
                         g_libc.__cxa_finalize_fn (d);
                    }
                    
                     int __cxa_atexit (void (*func)(void *), void *arg, void *d) {
                        return g_libc.__cxa_atexit_fn (func, arg, d);
                    }
                    
                     int atexit (void (*__func)(  ) ) noexcept __attribute__ ((__noreturn__)) {
                        return g_libc.atexit_fn (__func);
                    }
                    
                     long int random (){
                        return g_libc.random_fn ();
                    }
                    
                     void srandom (unsigned int __seed){
                         g_libc.srandom_fn (__seed);
                    }
                    
                     int rand (){
                        return g_libc.rand_fn ();
                    }
                    
                     void srand (unsigned int __seed){
                         g_libc.srand_fn (__seed);
                    }
                    
                     double drand48 (){
                        return g_libc.drand48_fn ();
                    }
                    
                     double erand48 (short unsigned int * __xsubi){
                        return g_libc.erand48_fn (__xsubi);
                    }
                    
                     long int lrand48 (){
                        return g_libc.lrand48_fn ();
                    }
                    
                     long int nrand48 (short unsigned int * __xsubi){
                        return g_libc.nrand48_fn (__xsubi);
                    }
                    
                     long int mrand48 (){
                        return g_libc.mrand48_fn ();
                    }
                    
                     long int jrand48 (short unsigned int * __xsubi){
                        return g_libc.jrand48_fn (__xsubi);
                    }
                    
                     void srand48 (long int __seedval){
                         g_libc.srand48_fn (__seedval);
                    }
                    
                     short unsigned int * seed48 (short unsigned int * __seed16v){
                        return g_libc.seed48_fn (__seed16v);
                    }
                    
                     void lcong48 (short unsigned int * __param){
                         g_libc.lcong48_fn (__param);
                    }
                    
                     int drand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ double * __result){
                        return g_libc.drand48_r_fn (__buffer,__result);
                    }
                    
                     int erand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ double * __result){
                        return g_libc.erand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int lrand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result){
                        return g_libc.lrand48_r_fn (__buffer,__result);
                    }
                    
                     int nrand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result){
                        return g_libc.nrand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int mrand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result){
                        return g_libc.mrand48_r_fn (__buffer,__result);
                    }
                    
                     int jrand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result){
                        return g_libc.jrand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int srand48_r (long int __seedval,drand48_data * __buffer){
                        return g_libc.srand48_r_fn (__seedval,__buffer);
                    }
                    
                     int seed48_r (short unsigned int * __seed16v,drand48_data * __buffer){
                        return g_libc.seed48_r_fn (__seed16v,__buffer);
                    }
                    
                     int lcong48_r (short unsigned int * __param,drand48_data * __buffer){
                        return g_libc.lcong48_r_fn (__param,__buffer);
                    }
                    
                     void * calloc (size_t __nmemb,size_t __size){
                        return g_libc.calloc_fn (__nmemb,__size);
                    }
                    
                     void * malloc (size_t __size){
                        return g_libc.malloc_fn (__size);
                    }
                    
                     void free (void * __ptr){
                         g_libc.free_fn (__ptr);
                    }
                    
                     void * realloc (void * __ptr,size_t __size){
                        return g_libc.realloc_fn (__ptr,__size);
                    }
                    
                     int atoi (char const * __nptr){
                        return g_libc.atoi_fn (__nptr);
                    }
                    
                     long int atol (char const * __nptr){
                        return g_libc.atol_fn (__nptr);
                    }
                    
                     long long int atoll (char const * __nptr){
                        return g_libc.atoll_fn (__nptr);
                    }
                    
                     double atof (char const * __nptr){
                        return g_libc.atof_fn (__nptr);
                    }
                    
                     long int strtol (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base){
                        return g_libc.strtol_fn (__nptr,__endptr,__base);
                    }
                    
                     long long int strtoll (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base){
                        return g_libc.strtoll_fn (__nptr,__endptr,__base);
                    }
                    
                     long unsigned int strtoul (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base){
                        return g_libc.strtoul_fn (__nptr,__endptr,__base);
                    }
                    
                     long long unsigned int strtoull (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base){
                        return g_libc.strtoull_fn (__nptr,__endptr,__base);
                    }
                    
                     double strtod (__restrict__ char const * __nptr,__restrict__ char * * __endptr){
                        return g_libc.strtod_fn (__nptr,__endptr);
                    }
                    
                     char * getenv (char const * __name){
                        return g_libc.getenv_fn (__name);
                    }
                    
                     int putenv (char * __string){
                        return g_libc.putenv_fn (__string);
                    }
                    
                     int setenv (char const * __name,char const * __value,int __replace){
                        return g_libc.setenv_fn (__name,__value,__replace);
                    }
                    
                     int unsetenv (char const * __name){
                        return g_libc.unsetenv_fn (__name);
                    }
                    
                     int clearenv (){
                        return g_libc.clearenv_fn ();
                    }
                    
                     void qsort (void * __base,size_t __nmemb,size_t __size,__compar_fn_t __compar){
                         g_libc.qsort_fn (__base,__nmemb,__size,__compar);
                    }
                    
                     void abort () noexcept __attribute__ ((__noreturn__)) {
                         g_libc.abort_fn ();
                    }
                    
                     int mkstemp (char * __template){
                        return g_libc.mkstemp_fn (__template);
                    }
                    
                     FILE * tmpfile (){
                        return g_libc.tmpfile_fn ();
                    }
                    
                     int rename (char const * __old,char const * __new){
                        return g_libc.rename_fn (__old,__new);
                    }
                    
                     void bzero (void * __s,size_t __n){
                         g_libc.bzero_fn (__s,__n);
                    }
                    
                     char * strerror (int __errnum){
                        return g_libc.strerror_fn (__errnum);
                    }
                    
                     char * strerror_r (int __errnum,char * __buf,size_t __buflen){
                        return g_libc.strerror_r_fn (__errnum,__buf,__buflen);
                    }
                    
                     int strcoll (char const * __s1,char const * __s2){
                        return g_libc.strcoll_fn (__s1,__s2);
                    }
                    
                     void * memset (void * __s,int __c,size_t __n){
                        return g_libc.memset_fn (__s,__c,__n);
                    }
                    
                     void * memcpy (__restrict__ void * __dest,__restrict__ void const * __src,size_t __n){
                        return g_libc.memcpy_fn (__dest,__src,__n);
                    }
                    
                     void bcopy (void const * __src,void * __dest,size_t __n){
                         g_libc.bcopy_fn (__src,__dest,__n);
                    }
                    
                     int memcmp (void const * __s1,void const * __s2,size_t __n){
                        return g_libc.memcmp_fn (__s1,__s2,__n);
                    }
                    
                     void * memmove (void * __dest,void const * __src,size_t __n){
                        return g_libc.memmove_fn (__dest,__src,__n);
                    }
                    
                     char * strcpy (__restrict__ char * __dest,__restrict__ char const * __src){
                        return g_libc.strcpy_fn (__dest,__src);
                    }
                    
                     char * strncpy (__restrict__ char * __dest,__restrict__ char const * __src,size_t __n){
                        return g_libc.strncpy_fn (__dest,__src,__n);
                    }
                    
                     char * strcat (__restrict__ char * __dest,__restrict__ char const * __src){
                        return g_libc.strcat_fn (__dest,__src);
                    }
                    
                     char * strncat (__restrict__ char * __dest,__restrict__ char const * __src,size_t __n){
                        return g_libc.strncat_fn (__dest,__src,__n);
                    }
                    
                     int strcmp (char const * __s1,char const * __s2){
                        return g_libc.strcmp_fn (__s1,__s2);
                    }
                    
                     int strncmp (char const * __s1,char const * __s2,size_t __n){
                        return g_libc.strncmp_fn (__s1,__s2,__n);
                    }
                    
                     size_t strlen (char const * __s){
                        return g_libc.strlen_fn (__s);
                    }
                    
                     size_t strnlen (char const * __string,size_t __maxlen){
                        return g_libc.strnlen_fn (__string,__maxlen);
                    }
                    
                     size_t strcspn (char const * __s,char const * __reject){
                        return g_libc.strcspn_fn (__s,__reject);
                    }
                    
                     size_t strspn (char const * __s,char const * __accept){
                        return g_libc.strspn_fn (__s,__accept);
                    }
                    
                     int strcasecmp (char const * __s1,char const * __s2){
                        return g_libc.strcasecmp_fn (__s1,__s2);
                    }
                    
                     int strncasecmp (char const * __s1,char const * __s2,size_t __n){
                        return g_libc.strncasecmp_fn (__s1,__s2,__n);
                    }
                    
                     char * strdup (char const * __s){
                        return g_libc.strdup_fn (__s);
                    }
                    
                     char * strndup (char const * __string,size_t __n){
                        return g_libc.strndup_fn (__string,__n);
                    }
                    
                     char * strsep (__restrict__ char * * __stringp,__restrict__ char const * __delim){
                        return g_libc.strsep_fn (__stringp,__delim);
                    }
                    
                     char * setlocale (int __category,char const * __locale){
                        return g_libc.setlocale_fn (__category,__locale);
                    }
                    
                     __locale_t newlocale (int __category_mask,char const * __locale,__locale_t __base){
                        return g_libc.newlocale_fn (__category_mask,__locale,__base);
                    }
                    
                     __locale_t uselocale (__locale_t __dataset){
                        return g_libc.uselocale_fn (__dataset);
                    }
                     int * __errno_location () {
                        return g_libc.__errno_location_fn ();
                    }
                    
                     int * __h_errno_location () {
                        return g_libc.__h_errno_location_fn ();
                    }
                     int wctob (wint_t __c){
                        return g_libc.wctob_fn (__c);
                    }
                    
                     wint_t btowc (int __c){
                        return g_libc.btowc_fn (__c);
                    }
                    
                     size_t mbrlen (__restrict__ char const * __s,size_t __n,__restrict__ ::mbstate_t * __ps){
                        return g_libc.mbrlen_fn (__s,__n,__ps);
                    }
                    
                     uint32_t htonl (uint32_t __hostlong){
                        return g_libc.htonl_fn (__hostlong);
                    }
                    
                     uint16_t htons (uint16_t __hostshort){
                        return g_libc.htons_fn (__hostshort);
                    }
                    
                     uint32_t ntohl (uint32_t __netlong){
                        return g_libc.ntohl_fn (__netlong);
                    }
                    
                     uint16_t ntohs (uint16_t __netshort){
                        return g_libc.ntohs_fn (__netshort);
                    }
                    
                     int lockf (int __fd,int __cmd,off_t __len){
                        return g_libc.lockf_fn (__fd,__cmd,__len);
                    }
                    
                     int inet_aton (char const * __cp,in_addr * __inp){
                        return g_libc.inet_aton_fn (__cp,__inp);
                    }
                    
                     in_addr_t inet_addr (char const * __cp){
                        return g_libc.inet_addr_fn (__cp);
                    }
                    
                     in_addr_t inet_network (char const * __cp){
                        return g_libc.inet_network_fn (__cp);
                    }
                    
                     char * inet_ntoa (in_addr __in){
                        return g_libc.inet_ntoa_fn (__in);
                    }
                    
                     in_addr inet_makeaddr (in_addr_t __net,in_addr_t __host){
                        return g_libc.inet_makeaddr_fn (__net,__host);
                    }
                    
                     in_addr_t inet_lnaof (in_addr __in){
                        return g_libc.inet_lnaof_fn (__in);
                    }
                    
                     in_addr_t inet_netof (in_addr __in){
                        return g_libc.inet_netof_fn (__in);
                    }
                    
                     char const * inet_ntop (int __af,__restrict__ void const * __cp,__restrict__ char * __buf,socklen_t __len) noexcept {
                        return g_libc.inet_ntop_fn (__af,__cp,__buf,__len);
                    }
                    
                     int inet_pton (int __af,__restrict__ char const * __cp,__restrict__ void * __buf){
                        return g_libc.inet_pton_fn (__af,__cp,__buf);
                    }
                    
                     int inet6_opt_find (void * __extbuf,socklen_t __extlen,int __offset,uint8_t __type,socklen_t * __lenp,void * * __databufp){
                        return g_libc.inet6_opt_find_fn (__extbuf,__extlen,__offset,__type,__lenp,__databufp);
                    }
                    
                     int socket (int __domain,int __type,int __protocol){
                        return g_libc.socket_fn (__domain,__type,__protocol);
                    }
                    
                     int socketpair (int __domain,int __type,int __protocol,int * __fds){
                        return g_libc.socketpair_fn (__domain,__type,__protocol,__fds);
                    }
                    
                     int getsockname (int __fd,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __len){
                        return g_libc.getsockname_fn (__fd,__addr,__len);
                    }
                    
                     int getpeername (int __fd,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __len){
                        return g_libc.getpeername_fn (__fd,__addr,__len);
                    }
                    
                     int bind (int __fd,sockaddr const * __addr,socklen_t __len){
                        return g_libc.bind_fn (__fd,__addr,__len);
                    }
                    
                     int connect (int __fd,sockaddr const * __addr,socklen_t __len){
                        return g_libc.connect_fn (__fd,__addr,__len);
                    }
                    
                     int setsockopt (int __fd,int __level,int __optname,void const * __optval,socklen_t __optlen){
                        return g_libc.setsockopt_fn (__fd,__level,__optname,__optval,__optlen);
                    }
                    
                     int getsockopt (int __fd,int __level,int __optname,__restrict__ void * __optval,__restrict__ ::socklen_t * __optlen){
                        return g_libc.getsockopt_fn (__fd,__level,__optname,__optval,__optlen);
                    }
                    
                     int listen (int __fd,int __n){
                        return g_libc.listen_fn (__fd,__n);
                    }
                    
                     int accept (int __fd,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __addr_len){
                        return g_libc.accept_fn (__fd,__addr,__addr_len);
                    }
                    
                     int shutdown (int __fd,int __how){
                        return g_libc.shutdown_fn (__fd,__how);
                    }
                    
                     ssize_t send (int __fd,void const * __buf,size_t __n,int __flags){
                        return g_libc.send_fn (__fd,__buf,__n,__flags);
                    }
                    
                     ssize_t sendto (int __fd,void const * __buf,size_t __n,int __flags,sockaddr const * __addr,socklen_t __addr_len){
                        return g_libc.sendto_fn (__fd,__buf,__n,__flags,__addr,__addr_len);
                    }
                    
                     ssize_t sendmsg (int __fd,msghdr const * __message,int __flags){
                        return g_libc.sendmsg_fn (__fd,__message,__flags);
                    }
                    
                     ssize_t recv (int __fd,void * __buf,size_t __n,int __flags){
                        return g_libc.recv_fn (__fd,__buf,__n,__flags);
                    }
                    
                     ssize_t recvfrom (int __fd,__restrict__ void * __buf,size_t __n,int __flags,__restrict__ ::sockaddr * __addr,__restrict__ ::socklen_t * __addr_len){
                        return g_libc.recvfrom_fn (__fd,__buf,__n,__flags,__addr,__addr_len);
                    }
                    
                     ssize_t recvmsg (int __fd,msghdr * __message,int __flags){
                        return g_libc.recvmsg_fn (__fd,__message,__flags);
                    }
                    
                     int getnameinfo (__restrict__ ::sockaddr const * __sa,socklen_t __salen,__restrict__ char * __host,socklen_t __hostlen,__restrict__ char * __serv,socklen_t __servlen,int __flags){
                        return g_libc.getnameinfo_fn (__sa,__salen,__host,__hostlen,__serv,__servlen,__flags);
                    }
                    
                     ssize_t read (int __fd,void * __buf,size_t __nbytes){
                        return g_libc.read_fn (__fd,__buf,__nbytes);
                    }
                    
                     ssize_t write (int __fd,void const * __buf,size_t __n){
                        return g_libc.write_fn (__fd,__buf,__n);
                    }
                    
                     unsigned int sleep (unsigned int __seconds){
                        return g_libc.sleep_fn (__seconds);
                    }
                    
                     int usleep (__useconds_t __useconds){
                        return g_libc.usleep_fn (__useconds);
                    }
                    
                     int getopt (int ___argc,char * const * ___argv,char const * __shortopts){
                        return g_libc.getopt_fn (___argc,___argv,__shortopts);
                    }
                    
                     int getopt_long (int ___argc,char * const * ___argv,char const * __shortopts,option const * __longopts,int * __longind){
                        return g_libc.getopt_long_fn (___argc,___argv,__shortopts,__longopts,__longind);
                    }
                    
                     __pid_t getpid (){
                        return g_libc.getpid_fn ();
                    }
                    
                     __pid_t getppid (){
                        return g_libc.getppid_fn ();
                    }
                    
                     __uid_t getuid (){
                        return g_libc.getuid_fn ();
                    }
                    
                     __uid_t geteuid (){
                        return g_libc.geteuid_fn ();
                    }
                    
                     int setuid (__uid_t __uid){
                        return g_libc.setuid_fn (__uid);
                    }
                    
                     int setgid (__gid_t __gid){
                        return g_libc.setgid_fn (__gid);
                    }
                    
                     int seteuid (__uid_t __uid){
                        return g_libc.seteuid_fn (__uid);
                    }
                    
                     int setegid (__gid_t __gid){
                        return g_libc.setegid_fn (__gid);
                    }
                    
                     int setreuid (__uid_t __ruid,__uid_t __euid){
                        return g_libc.setreuid_fn (__ruid,__euid);
                    }
                    
                     int setregid (__gid_t __rgid,__gid_t __egid){
                        return g_libc.setregid_fn (__rgid,__egid);
                    }
                    
                     int setresuid (__uid_t __ruid,__uid_t __euid,__uid_t __suid){
                        return g_libc.setresuid_fn (__ruid,__euid,__suid);
                    }
                    
                     int setresgid (__gid_t __rgid,__gid_t __egid,__gid_t __sgid){
                        return g_libc.setresgid_fn (__rgid,__egid,__sgid);
                    }
                    
                     int dup (int __fd){
                        return g_libc.dup_fn (__fd);
                    }
                    
                     int dup2 (int __fd,int __fd2){
                        return g_libc.dup2_fn (__fd,__fd2);
                    }
                    
                     int close (int __fd){
                        return g_libc.close_fn (__fd);
                    }
                    
                     int unlink (char const * __name){
                        return g_libc.unlink_fn (__name);
                    }
                    
                     int rmdir (char const * __path){
                        return g_libc.rmdir_fn (__path);
                    }
                    
                     int select (int __nfds,__restrict__ ::fd_set * __readfds,__restrict__ ::fd_set * __writefds,__restrict__ ::fd_set * __exceptfds,__restrict__ ::timeval * __timeout){
                        return g_libc.select_fn (__nfds,__readfds,__writefds,__exceptfds,__timeout);
                    }
                    
                     int isatty (int __fd){
                        return g_libc.isatty_fn (__fd);
                    }
                    
                     void exit (int __status) noexcept __attribute__ ((__noreturn__)) {
                         g_libc.exit_fn (__status);
                    }
                    
                     char * getcwd (char * __buf,size_t __size){
                        return g_libc.getcwd_fn (__buf,__size);
                    }
                    
                     char * getwd (char * __buf){
                        return g_libc.getwd_fn (__buf);
                    }
                    
                     char * get_current_dir_name (){
                        return g_libc.get_current_dir_name_fn ();
                    }
                    
                     int chdir (char const * __path){
                        return g_libc.chdir_fn (__path);
                    }
                    
                     int fchdir (int __fd){
                        return g_libc.fchdir_fn (__fd);
                    }
                    
                     __pid_t fork (){
                        return g_libc.fork_fn ();
                    }
                    
                     int execv (char const * __path,char * const * __argv){
                        return g_libc.execv_fn (__path,__argv);
                    }
                    
                     int execl (char const * __path,char const * __arg, ... ){
                        va_list vl;
                        va_start (vl, __arg);
                        return g_libc.execl_fn (__path,__arg, vl );
                    }
                    
                     int execve (char const * __path,char * const * __argv,char * const * __envp){
                        return g_libc.execve_fn (__path,__argv,__envp);
                    }
                    
                     int execvp (char const * __file,char * const * __argv){
                        return g_libc.execvp_fn (__file,__argv);
                    }
                    
                     int execlp (char const * __file,char const * __arg,... ){
                       va_list vl;
  va_start (vl, __arg);
                        return g_libc.execlp_fn (__file,__arg, vl);
                    }
                    
                     int execle (char const * __path,char const * __arg,... ){
                       va_list vl;
  va_start (vl, __arg);
                        return g_libc.execle_fn (__path,__arg, vl);
                    }
                    
                     int truncate (char const * __file,__off_t __length){
                        return g_libc.truncate_fn (__file,__length);
                    }
                    
                     int ftruncate (int __fd,__off_t __length){
                        return g_libc.ftruncate_fn (__fd,__length);
                    }
                    
                     int ftruncate64 (int __fd,__off64_t __length){
                        return g_libc.ftruncate64_fn (__fd,__length);
                    }
                    
                     long int sysconf (int __name){
                        return g_libc.sysconf_fn (__name);
                    }
                    
                     char * ttyname (int __fd){
                        return g_libc.ttyname_fn (__fd);
                    }
                    
                     void * sbrk (intptr_t __delta){
                        return g_libc.sbrk_fn (__delta);
                    }
                    
                     int getpagesize (){
                        return g_libc.getpagesize_fn ();
                    }
                    
                     __gid_t getgid (){
                        return g_libc.getgid_fn ();
                    }
                    
                     __gid_t getegid (){
                        return g_libc.getegid_fn ();
                    }
                    
                     int gethostname (char * __name,size_t __len){
                        return g_libc.gethostname_fn (__name,__len);
                    }
                    
                     __pid_t getpgrp (){
                        return g_libc.getpgrp_fn ();
                    }
                    
                     __off_t lseek (int __fd,__off_t __offset,int __whence){
                        return g_libc.lseek_fn (__fd,__offset,__whence);
                    }
                    
                     __off64_t lseek64 (int __fd,__off64_t __offset,int __whence){
                        return g_libc.lseek64_fn (__fd,__offset,__whence);
                    }
                    
                     int euidaccess (char const * __name,int __type){
                        return g_libc.euidaccess_fn (__name,__type);
                    }
                    
                     int eaccess (char const * __name,int __type){
                        return g_libc.eaccess_fn (__name,__type);
                    }
                    
                     int access (char const * __name,int __type){
                        return g_libc.access_fn (__name,__type);
                    }
                    
                     int pipe (int * __pipedes){
                        return g_libc.pipe_fn (__pipedes);
                    }
                    
                     long int pathconf (char const * __path,int __name){
                        return g_libc.pathconf_fn (__path,__name);
                    }
                    
                     int getdtablesize (){
                        return g_libc.getdtablesize_fn ();
                    }
                    
                     ssize_t pread (int __fd,void * __buf,size_t __nbytes,__off_t __offset){
                        return g_libc.pread_fn (__fd,__buf,__nbytes,__offset);
                    }
                    
                     ssize_t pwrite (int __fd,void const * __buf,size_t __n,__off_t __offset){
                        return g_libc.pwrite_fn (__fd,__buf,__n,__offset);
                    }
                    
                     int daemon (int __nochdir,int __noclose){
                        return g_libc.daemon_fn (__nochdir,__noclose);
                    }
                    
                     unsigned int alarm (unsigned int __seconds){
                        return g_libc.alarm_fn (__seconds);
                    }
                    
                     ssize_t readlink (__restrict__ char const * __path,__restrict__ char * __buf,size_t __len){
                        return g_libc.readlink_fn (__path,__buf,__len);
                    }
                    
                     int chown (char const * __file,__uid_t __owner,__gid_t __group){
                        return g_libc.chown_fn (__file,__owner,__group);
                    }
                    
                     int initgroups (char const * __user,__gid_t __group){
                        return g_libc.initgroups_fn (__user,__group);
                    }
                    
                     int fsync (int __fd){
                        return g_libc.fsync_fn (__fd);
                    }
                    
                     ssize_t readv (int __fd,iovec const * __iovec,int __count){
                        return g_libc.readv_fn (__fd,__iovec,__count);
                    }
                    
                     ssize_t writev (int __fd,iovec const * __iovec,int __count){
                        return g_libc.writev_fn (__fd,__iovec,__count);
                    }
                    
                     void clearerr (FILE * __stream){
                         g_libc.clearerr_fn (__stream);
                    }
                    
                     int setvbuf (__restrict__ ::FILE * __stream,__restrict__ char * __buf,int __modes,size_t __n){
                        return g_libc.setvbuf_fn (__stream,__buf,__modes,__n);
                    }
                    
                     void setbuf (__restrict__ ::FILE * __stream,__restrict__ char * __buf){
                         g_libc.setbuf_fn (__stream,__buf);
                    }
                    
                     void setbuffer (__restrict__ ::FILE * __stream,__restrict__ char * __buf,size_t __size){
                         g_libc.setbuffer_fn (__stream,__buf,__size);
                    }
                    
                     void setlinebuf (FILE * __stream){
                         g_libc.setlinebuf_fn (__stream);
                    }
                    
                     int fseek (FILE * __stream,long int __off,int __whence){
                        return g_libc.fseek_fn (__stream,__off,__whence);
                    }
                    
                     long int ftell (FILE * __stream){
                        return g_libc.ftell_fn (__stream);
                    }
                    
                     int fseeko (FILE * __stream,__off_t __off,int __whence){
                        return g_libc.fseeko_fn (__stream,__off,__whence);
                    }
                    
                     __off_t ftello (FILE * __stream){
                        return g_libc.ftello_fn (__stream);
                    }
                    
                     void rewind (FILE * __stream){
                         g_libc.rewind_fn (__stream);
                    }
                    
                     int fgetpos (__restrict__ ::FILE * __stream,__restrict__ ::fpos_t * __pos){
                        return g_libc.fgetpos_fn (__stream,__pos);
                    }
                    
                     int fsetpos (FILE * __stream,fpos_t const * __pos){
                        return g_libc.fsetpos_fn (__stream,__pos);
                    }
                    
                     int printf (__restrict__ char const * __format,... ){
                       va_list vl;
                        va_start (vl, __format);                     

                        return g_libc.printf_fn (__format, vl);
                    }
                    
                     int fprintf (__restrict__ ::FILE * __stream,__restrict__ char const * __format,... ){
                                       va_list vl;
                        va_start (vl, __format);                     

                        return g_libc.fprintf_fn (__stream,__format, vl);
                    }
                    
                     int sprintf (__restrict__ char * __s,__restrict__ char const * __format,... ){
                                            va_list vl;
                        va_start (vl, __format);                     

                        return g_libc.sprintf_fn (__s,__format, vl);
                    }
                    
                     int asprintf (__restrict__ char * * __ptr,__restrict__ char const * __fmt,... ){
                        va_list vl;
                        va_start (vl, __fmt);

                        return g_libc.asprintf_fn (__ptr,__fmt, vl);
                    }
                    
                     int vasprintf (__restrict__ char * * __ptr,__restrict__ char const * __f,va_list __arg){
                        return g_libc.vasprintf_fn (__ptr,__f,__arg);
                    }
                    
                     int dprintf (int __fd,__restrict__ char const * __fmt,... ){
                                            va_list vl;
                        va_start (vl, __fmt);                     

                        return g_libc.dprintf_fn (__fd,__fmt, vl);
                    }
                    
                     int vdprintf (int __fd,__restrict__ char const * __fmt,va_list __arg){
                        return g_libc.vdprintf_fn (__fd,__fmt,__arg);
                    }
                    
                     int fgetc (FILE * __stream){
                        return g_libc.fgetc_fn (__stream);
                    }
                    
                     int getc (FILE * __stream){
                        return g_libc.getc_fn (__stream);
                    }
                    
                     int getc_unlocked (FILE * __stream){
                        return g_libc.getc_unlocked_fn (__stream);
                    }
                    
                     int getchar (){
                        return g_libc.getchar_fn ();
                    }
                    
                     int _IO_getc (_IO_FILE * __fp){
                        return g_libc._IO_getc_fn (__fp);
                    }
                    
                     int fputc (int __c,FILE * __stream){
                        return g_libc.fputc_fn (__c,__stream);
                    }
                    
                     int putc (int __c,FILE * __stream){
                        return g_libc.putc_fn (__c,__stream);
                    }
                    
                     int putc_unlocked (int __c,FILE * __stream){
                        return g_libc.putc_unlocked_fn (__c,__stream);
                    }
                    
                     int putchar (int __c){
                        return g_libc.putchar_fn (__c);
                    }
                    
                     int _IO_putc (int __c,_IO_FILE * __fp){
                        return g_libc._IO_putc_fn (__c,__fp);
                    }
                    
                     char * fgets (__restrict__ char * __s,int __n,__restrict__ ::FILE * __stream){
                        return g_libc.fgets_fn (__s,__n,__stream);
                    }
                    
                     int fputs (__restrict__ char const * __s,__restrict__ ::FILE * __stream){
                        return g_libc.fputs_fn (__s,__stream);
                    }
                    
                     int puts (char const * __s){
                        return g_libc.puts_fn (__s);
                    }
                    
                     int ungetc (int __c,FILE * __stream){
                        return g_libc.ungetc_fn (__c,__stream);
                    }
                    
                     int fclose (FILE * __stream){
                        return g_libc.fclose_fn (__stream);
                    }
                    
                     int fcloseall (){
                        return g_libc.fcloseall_fn ();
                    }
                    
                     FILE * fopen (__restrict__ char const * __filename,__restrict__ char const * __modes){
                        return g_libc.fopen_fn (__filename,__modes);
                    }
                    
                     FILE * fopen64 (__restrict__ char const * __filename,__restrict__ char const * __modes){
                        return g_libc.fopen64_fn (__filename,__modes);
                    }
                    
                     FILE * freopen (__restrict__ char const * __filename,__restrict__ char const * __modes,__restrict__ ::FILE * __stream){
                        return g_libc.freopen_fn (__filename,__modes,__stream);
                    }
                    
                     FILE * fdopen (int __fd,char const * __modes){
                        return g_libc.fdopen_fn (__fd,__modes);
                    }
                    
                     size_t fread (__restrict__ void * __ptr,size_t __size,size_t __n,__restrict__ ::FILE * __stream){
                        return g_libc.fread_fn (__ptr,__size,__n,__stream);
                    }
                    
                     size_t fwrite (__restrict__ void const * __ptr,size_t __size,size_t __n,__restrict__ ::FILE * __s){
                        return g_libc.fwrite_fn (__ptr,__size,__n,__s);
                    }
                    
                     int fflush (FILE * __stream){
                        return g_libc.fflush_fn (__stream);
                    }
                    
                     int ferror (FILE * __stream){
                        return g_libc.ferror_fn (__stream);
                    }
                    
                     int feof (FILE * __stream){
                        return g_libc.feof_fn (__stream);
                    }
                    
                     int fileno (FILE * __stream){
                        return g_libc.fileno_fn (__stream);
                    }
                    
                     void perror (char const * __s){
                         g_libc.perror_fn (__s);
                    }
                    
                     int remove (char const * __filename){
                        return g_libc.remove_fn (__filename);
                    }
                    
                     int sscanf (__restrict__ char const * __s,__restrict__ char const * __format,... ){
                        va_list vl;
                        va_start (vl, __format);
                        return g_libc.sscanf_fn (__s,__format,vl);
                    }
                    
                     void flockfile (FILE * __stream){
                         g_libc.flockfile_fn (__stream);
                    }
                    
                     void funlockfile (FILE * __stream){
                         g_libc.funlockfile_fn (__stream);
                    }
                    
                     int vprintf (__restrict__ char const * __format,va_list __arg){
                        return g_libc.vprintf_fn (__format,__arg);
                    }
                    
                     int vfprintf (__restrict__ ::FILE * __s,__restrict__ char const * __format,va_list __arg){
                        return g_libc.vfprintf_fn (__s,__format,__arg);
                    }
                    
                     int vsprintf (__restrict__ char * __s,__restrict__ char const * __format,va_list __arg){
                        return g_libc.vsprintf_fn (__s,__format,__arg);
                    }
                    
                     int fcntl (int __fd,int __cmd,... ){
                                             va_list vl;
                        va_start (vl, __cmd);
                        return g_libc.fcntl_fn (__fd,__cmd, vl);
                    }
                    
                     int open (char const * __file,int __oflag,... ){
                        va_list vl;
                        va_start (vl, __oflag);
                        return g_libc.open_fn (__file,__oflag, vl);
                    }
                    
                     int open64 (char const * __file,int __oflag,... ){
                        va_list vl;
                        va_start (vl, __oflag);
                        return g_libc.open64_fn (__file,__oflag, vl);
                    }
                    
                     int unlinkat (int __fd,char const * __name,int __flag){
                        return g_libc.unlinkat_fn (__fd,__name,__flag);
                    }
                    
                     int nanosleep (timespec const * __requested_time,timespec * __remaining){
                        return g_libc.nanosleep_fn (__requested_time,__remaining);
                    }
                    
                     char * asctime (tm const * __tp){
                        return g_libc.asctime_fn (__tp);
                    }
                    
                     char * asctime_r (__restrict__ ::tm const * __tp,__restrict__ char * __buf){
                        return g_libc.asctime_r_fn (__tp,__buf);
                    }
                    
                     char * ctime (time_t const * __timer){
                        return g_libc.ctime_fn (__timer);
                    }
                    
                     char * ctime_r (__restrict__ ::time_t const * __timer,__restrict__ char * __buf){
                        return g_libc.ctime_r_fn (__timer,__buf);
                    }
                    
                     tm * gmtime (time_t const * __timer){
                        return g_libc.gmtime_fn (__timer);
                    }
                    
                     tm * gmtime_r (__restrict__ ::time_t const * __timer,__restrict__ ::tm * __tp){
                        return g_libc.gmtime_r_fn (__timer,__tp);
                    }
                    
                     time_t mktime (tm * __tp){
                        return g_libc.mktime_fn (__tp);
                    }
                    
                     size_t strftime (__restrict__ char * __s,size_t __maxsize,__restrict__ char const * __format,__restrict__ ::tm const * __tp){
                        return g_libc.strftime_fn (__s,__maxsize,__format,__tp);
                    }
                    
                     char * strptime (__restrict__ char const * __s,__restrict__ char const * __fmt,tm * __tp){
                        return g_libc.strptime_fn (__s,__fmt,__tp);
                    }
                    
                     time_t timegm (tm * __tp){
                        return g_libc.timegm_fn (__tp);
                    }
                    
                     time_t timelocal (tm * __tp){
                        return g_libc.timelocal_fn (__tp);
                    }
                    
                     int timer_create (clockid_t __clock_id,__restrict__ ::sigevent * __evp,__restrict__ ::timer_t * __timerid){
                        return g_libc.timer_create_fn (__clock_id,__evp,__timerid);
                    }
                    
                     int timer_settime (timer_t __timerid,int __flags,__restrict__ ::itimerspec const * __value,__restrict__ ::itimerspec * __ovalue){
                        return g_libc.timer_settime_fn (__timerid,__flags,__value,__ovalue);
                    }
                    
                     int timer_gettime (timer_t __timerid,itimerspec * __value){
                        return g_libc.timer_gettime_fn (__timerid,__value);
                    }
                    
                     int utime (char const * __file,utimbuf const * __file_times){
                        return g_libc.utime_fn (__file,__file_times);
                    }
                    
                     void tzset (){
                         g_libc.tzset_fn ();
                    }
                    
                     int gettimeofday (__restrict__ ::timeval * __tv,__timezone_ptr_t __tz){
                        return g_libc.gettimeofday_fn (__tv,__tz);
                    }
                    
                     time_t time (time_t * __timer){
                        return g_libc.time_fn (__timer);
                    }
                    
                     int setitimer (__itimer_which_t __which,__restrict__ ::itimerval const * __new,__restrict__ ::itimerval * __old){
                        return g_libc.setitimer_fn (__which,__new,__old);
                    }
                    
                     int getitimer (__itimer_which_t __which,itimerval * __value){
                        return g_libc.getitimer_fn (__which,__value);
                    }
                    
                     int sysinfo (struct sysinfo *info) noexcept {
                        return g_libc.sysinfo_fn (info);
                    }
                    
                     void * mmap (void * __addr,size_t __len,int __prot,int __flags,int __fd,__off_t __offset){
                        return g_libc.mmap_fn (__addr,__len,__prot,__flags,__fd,__offset);
                    }
                    
                     void * mmap64 (void * __addr,size_t __len,int __prot,int __flags,int __fd,__off64_t __offset){
                        return g_libc.mmap64_fn (__addr,__len,__prot,__flags,__fd,__offset);
                    }
                    
                     int munmap (void * __addr,size_t __len){
                        return g_libc.munmap_fn (__addr,__len);
                    }
                    
                     int mkdir (char const * __path,__mode_t __mode){
                        return g_libc.mkdir_fn (__path,__mode);
                    }
                    
                     __mode_t umask (__mode_t __mask){
                        return g_libc.umask_fn (__mask);
                    }
                    
                     int ioctl (int __fd,long unsigned int __request,... ){
                        va_list vl;
                        va_start (vl, __request);
                        return g_libc.ioctl_fn (__fd,__request, vl);
                    }
                    
                     int sched_yield (){
                        return g_libc.sched_yield_fn ();
                    }
                    
                     int poll (pollfd * __fds,nfds_t __nfds,int __timeout){
                        return g_libc.poll_fn (__fds,__nfds,__timeout);
                    }
                    
                     __sighandler_t signal (int __sig,__sighandler_t __handler){
                        return g_libc.signal_fn (__sig,__handler);
                    }
                    
                     int sigaction (int signum, const struct sigaction *act, struct sigaction *oldact) noexcept {
                        return g_libc.sigaction_fn (signum, act, oldact);
                    }
                    
                     int sigemptyset (sigset_t * __set){
                        return g_libc.sigemptyset_fn (__set);
                    }
                    
                     int sigfillset (sigset_t * __set){
                        return g_libc.sigfillset_fn (__set);
                    }
                    
                     int sigaddset (sigset_t * __set,int __signo){
                        return g_libc.sigaddset_fn (__set,__signo);
                    }
                    
                     int sigdelset (sigset_t * __set,int __signo){
                        return g_libc.sigdelset_fn (__set,__signo);
                    }
                    
                     int sigismember (sigset_t const * __set,int __signo){
                        return g_libc.sigismember_fn (__set,__signo);
                    }
                    
                     int sigprocmask (int __how,__restrict__ ::sigset_t const * __set,__restrict__ ::sigset_t * __oset){
                        return g_libc.sigprocmask_fn (__how,__set,__oset);
                    }
                    
                     int sigwait (__restrict__ ::sigset_t const * __set,__restrict__ int * __sig){
                        return g_libc.sigwait_fn (__set,__sig);
                    }
                    
                     int kill (__pid_t __pid,int __sig){
                        return g_libc.kill_fn (__pid,__sig);
                    }
                    
                     int pthread_create (__restrict__ ::pthread_t * __newthread,__restrict__ ::pthread_attr_t const * __attr,void * (*__start_routine)( void * ) ,__restrict__ void * __arg){
                        return g_libc.pthread_create_fn (__newthread,__attr,__start_routine,__arg);
                    }
                    
                     void pthread_exit (void * __retval) __attribute__ ((__noreturn__)) {
                         g_libc.pthread_exit_fn (__retval);
                    }
                    
                     pthread_t pthread_self (){
                        return g_libc.pthread_self_fn ();
                    }
                    
                     int pthread_once (pthread_once_t * __once_control,void (*__init_routine)(  ) ){
                        return g_libc.pthread_once_fn (__once_control,__init_routine);
                    }
                    
                     void * pthread_getspecific (pthread_key_t __key){
                        return g_libc.pthread_getspecific_fn (__key);
                    }
                    
                     int pthread_setspecific (pthread_key_t __key,void const * __pointer){
                        return g_libc.pthread_setspecific_fn (__key,__pointer);
                    }
                    
                     int pthread_key_create (pthread_key_t * __key,void (*__destr_function)( void * ) ){
                        return g_libc.pthread_key_create_fn (__key,__destr_function);
                    }
                    
                     int pthread_key_delete (pthread_key_t __key){
                        return g_libc.pthread_key_delete_fn (__key);
                    }
                    
                     int pthread_mutex_destroy (pthread_mutex_t * __mutex){
                        return g_libc.pthread_mutex_destroy_fn (__mutex);
                    }
                    
                     int pthread_mutex_init (pthread_mutex_t * __mutex,pthread_mutexattr_t const * __mutexattr){
                        return g_libc.pthread_mutex_init_fn (__mutex,__mutexattr);
                    }
                    
                     int pthread_mutex_lock (pthread_mutex_t * __mutex){
                        return g_libc.pthread_mutex_lock_fn (__mutex);
                    }
                    
                     int pthread_mutex_unlock (pthread_mutex_t * __mutex){
                        return g_libc.pthread_mutex_unlock_fn (__mutex);
                    }
                    
                     int pthread_mutex_trylock (pthread_mutex_t * __mutex){
                        return g_libc.pthread_mutex_trylock_fn (__mutex);
                    }
                    
                     int pthread_mutexattr_init (pthread_mutexattr_t * __attr){
                        return g_libc.pthread_mutexattr_init_fn (__attr);
                    }
                    
                     int pthread_mutexattr_destroy (pthread_mutexattr_t * __attr){
                        return g_libc.pthread_mutexattr_destroy_fn (__attr);
                    }
                    
                     int pthread_mutexattr_settype (pthread_mutexattr_t * __attr,int __kind){
                        return g_libc.pthread_mutexattr_settype_fn (__attr,__kind);
                    }
                    
                     int pthread_cancel (pthread_t __th){
                        return g_libc.pthread_cancel_fn (__th);
                    }
                    
                     int pthread_kill (pthread_t thread, int sig) noexcept {
                        return g_libc.pthread_kill_fn (thread, sig);
                    }
                    
                     int pthread_join (pthread_t __th,void * * __thread_return){
                        return g_libc.pthread_join_fn (__th,__thread_return);
                    }
                    
                     int pthread_detach (pthread_t __th){
                        return g_libc.pthread_detach_fn (__th);
                    }
                    
                     int pthread_cond_destroy (pthread_cond_t * __cond){
                        return g_libc.pthread_cond_destroy_fn (__cond);
                    }
                    
                     int pthread_cond_init (__restrict__ ::pthread_cond_t * __cond,__restrict__ ::pthread_condattr_t const * __cond_attr){
                        return g_libc.pthread_cond_init_fn (__cond,__cond_attr);
                    }
                    
                     int pthread_cond_broadcast (pthread_cond_t * __cond){
                        return g_libc.pthread_cond_broadcast_fn (__cond);
                    }
                    
                     int pthread_cond_signal (pthread_cond_t * __cond){
                        return g_libc.pthread_cond_signal_fn (__cond);
                    }
                    
                     int pthread_cond_timedwait (__restrict__ ::pthread_cond_t * __cond,__restrict__ ::pthread_mutex_t * __mutex,__restrict__ ::timespec const * __abstime){
                        return g_libc.pthread_cond_timedwait_fn (__cond,__mutex,__abstime);
                    }
                    
                     int pthread_cond_wait (__restrict__ ::pthread_cond_t * __cond,__restrict__ ::pthread_mutex_t * __mutex){
                        return g_libc.pthread_cond_wait_fn (__cond,__mutex);
                    }
                    
                     int pthread_condattr_destroy (pthread_condattr_t * __attr){
                        return g_libc.pthread_condattr_destroy_fn (__attr);
                    }
                    
                     int pthread_condattr_init (pthread_condattr_t * __attr){
                        return g_libc.pthread_condattr_init_fn (__attr);
                    }
                    
                     int pthread_rwlock_init (__restrict__ ::pthread_rwlock_t * __rwlock,__restrict__ ::pthread_rwlockattr_t const * __attr){
                        return g_libc.pthread_rwlock_init_fn (__rwlock,__attr);
                    }
                    
                     int pthread_rwlock_unlock (pthread_rwlock_t * __rwlock){
                        return g_libc.pthread_rwlock_unlock_fn (__rwlock);
                    }
                    
                     int pthread_rwlock_wrlock (pthread_rwlock_t * __rwlock){
                        return g_libc.pthread_rwlock_wrlock_fn (__rwlock);
                    }
                    
                     int pthread_rwlock_rdlock (pthread_rwlock_t * __rwlock){
                        return g_libc.pthread_rwlock_rdlock_fn (__rwlock);
                    }
                    
                     int pthread_rwlock_destroy (pthread_rwlock_t * __rwlock){
                        return g_libc.pthread_rwlock_destroy_fn (__rwlock);
                    }
                    
                     int pthread_setcancelstate (int __state,int * __oldstate){
                        return g_libc.pthread_setcancelstate_fn (__state,__oldstate);
                    }
                    
                     int pthread_sigmask (int __how,__restrict__ ::__sigset_t const * __newmask,__restrict__ ::__sigset_t * __oldmask){
                        return g_libc.pthread_sigmask_fn (__how,__newmask,__oldmask);
                    }
                    
                     int pthread_equal (pthread_t __thread1,pthread_t __thread2){
                        return g_libc.pthread_equal_fn (__thread1,__thread2);
                    }
                    
                     int pthread_spin_init (pthread_spinlock_t * __lock,int __pshared){
                        return g_libc.pthread_spin_init_fn (__lock,__pshared);
                    }
                    
                     int pthread_spin_lock (pthread_spinlock_t * __lock){
                        return g_libc.pthread_spin_lock_fn (__lock);
                    }
                    
                     int pthread_spin_unlock (pthread_spinlock_t * __lock){
                        return g_libc.pthread_spin_unlock_fn (__lock);
                    }
                    
                     int pthread_spin_destroy (pthread_spinlock_t * __lock){
                        return g_libc.pthread_spin_destroy_fn (__lock);
                    }
                    
                     int sem_init (sem_t * __sem,int __pshared,unsigned int __value){
                        return g_libc.sem_init_fn (__sem,__pshared,__value);
                    }
                    
                     int sem_destroy (sem_t * __sem){
                        return g_libc.sem_destroy_fn (__sem);
                    }
                    
                     int sem_post (sem_t * __sem){
                        return g_libc.sem_post_fn (__sem);
                    }
                    
                     int sem_wait (sem_t * __sem){
                        return g_libc.sem_wait_fn (__sem);
                    }
                    
                     int sem_timedwait (__restrict__ ::sem_t * __sem,__restrict__ ::timespec const * __abstime){
                        return g_libc.sem_timedwait_fn (__sem,__abstime);
                    }
                    
                     int sem_trywait (sem_t * __sem){
                        return g_libc.sem_trywait_fn (__sem);
                    }
                    
                     int sem_getvalue (__restrict__ ::sem_t * __sem,__restrict__ int * __sval){
                        return g_libc.sem_getvalue_fn (__sem,__sval);
                    }
                    
                     hostent * gethostbyname (char const * __name){
                        return g_libc.gethostbyname_fn (__name);
                    }
                    
                     hostent * gethostbyname2 (char const * __name,int __af){
                        return g_libc.gethostbyname2_fn (__name,__af);
                    }
                    
                     int getaddrinfo (__restrict__ char const * __name,__restrict__ char const * __service,__restrict__ ::addrinfo const * __req,__restrict__ ::addrinfo * * __pai){
                        return g_libc.getaddrinfo_fn (__name,__service,__req,__pai);
                    }
                    
                     void freeaddrinfo (addrinfo * __ai){
                         g_libc.freeaddrinfo_fn (__ai);
                    }
                    
                     char const * gai_strerror (int __ecode){
                        return g_libc.gai_strerror_fn (__ecode);
                    }
                    
                     int getifaddrs (ifaddrs * * __ifap){
                        return g_libc.getifaddrs_fn (__ifap);
                    }
                    
                     void freeifaddrs (ifaddrs * __ifa){
                         g_libc.freeifaddrs_fn (__ifa);
                    }
                    
                     hostent * gethostent (){
                        return g_libc.gethostent_fn ();
                    }
                    
                     void sethostent (int __stay_open){
                         g_libc.sethostent_fn (__stay_open);
                    }
                    
                     void endhostent (){
                         g_libc.endhostent_fn ();
                    }
                    
                     void herror (char const * __str){
                         g_libc.herror_fn (__str);
                    }
                    
                     char const * hstrerror (int __err_num){
                        return g_libc.hstrerror_fn (__err_num);
                    }
                    
                     protoent * getprotoent (){
                        return g_libc.getprotoent_fn ();
                    }
                    
                     protoent * getprotobyname (char const * __name){
                        return g_libc.getprotobyname_fn (__name);
                    }
                    
                     protoent * getprotobynumber (int __proto){
                        return g_libc.getprotobynumber_fn (__proto);
                    }
                    
                     void setprotoent (int __stay_open){
                         g_libc.setprotoent_fn (__stay_open);
                    }
                    
                     void endprotoent (){
                         g_libc.endprotoent_fn ();
                    }
                    
                     servent * getservent (){
                        return g_libc.getservent_fn ();
                    }
                    
                     servent * getservbyname (char const * __name,char const * __proto){
                        return g_libc.getservbyname_fn (__name,__proto);
                    }
                    
                     servent * getservbyport (int __port,char const * __proto){
                        return g_libc.getservbyport_fn (__port,__proto);
                    }
                    
                     void setservent (int __stay_open){
                         g_libc.setservent_fn (__stay_open);
                    }
                    
                     void endservent (){
                         g_libc.endservent_fn ();
                    }
                    
                     int toupper (int __c){
                        return g_libc.toupper_fn (__c);
                    }
                    
                     int tolower (int __c){
                        return g_libc.tolower_fn (__c);
                    }
                    
                     int isdigit (int arg0){
                        return g_libc.isdigit_fn (arg0);
                    }
                    
                     int isxdigit (int arg0){
                        return g_libc.isxdigit_fn (arg0);
                    }
                    
                     int isalnum (int arg0){
                        return g_libc.isalnum_fn (arg0);
                    }
                    
                     int timerfd_create (clockid_t __clock_id,int __flags){
                        return g_libc.timerfd_create_fn (__clock_id,__flags);
                    }
                    
                     int timerfd_settime (int __ufd,int __flags,itimerspec const * __utmr,itimerspec * __otmr){
                        return g_libc.timerfd_settime_fn (__ufd,__flags,__utmr,__otmr);
                    }
                    
                     int timerfd_gettime (int __ufd,itimerspec * __otmr){
                        return g_libc.timerfd_gettime_fn (__ufd,__otmr);
                    }
                    
                     unsigned int if_nametoindex (char const * __ifname){
                        return g_libc.if_nametoindex_fn (__ifname);
                    }
                    
                     char * if_indextoname (unsigned int __ifindex,char * __ifname){
                        return g_libc.if_indextoname_fn (__ifindex,__ifname);
                    }
                    
                     DIR * opendir (char const * __name){
                        return g_libc.opendir_fn (__name);
                    }
                    
                     DIR * fdopendir (int __fd){
                        return g_libc.fdopendir_fn (__fd);
                    }
                    
                     dirent * readdir (DIR * __dirp){
                        return g_libc.readdir_fn (__dirp);
                    }
                    
                     int readdir_r (__restrict__ ::DIR * __dirp,__restrict__ ::dirent * __entry,__restrict__ ::dirent * * __result){
                        return g_libc.readdir_r_fn (__dirp,__entry,__result);
                    }
                    
                     int closedir (DIR * __dirp){
                        return g_libc.closedir_fn (__dirp);
                    }
                    
                     int dirfd (DIR * __dirp){
                        return g_libc.dirfd_fn (__dirp);
                    }
                    
                     void rewinddir (DIR * __dirp){
                         g_libc.rewinddir_fn (__dirp);
                    }
                    
                     int scandir (__restrict__ char const * __dir,__restrict__ ::dirent * * * __namelist,int (*__selector)( ::dirent const * ) ,int (*__cmp)( ::dirent const * *,::dirent const * * ) ){
                        return g_libc.scandir_fn (__dir,__namelist,__selector,__cmp);
                    }
                    
                     int alphasort (dirent const * * __e1,dirent const * * __e2){
                        return g_libc.alphasort_fn (__e1,__e2);
                    }
                    
                     int alphasort64 (dirent64 const * * __e1,dirent64 const * * __e2){
                        return g_libc.alphasort64_fn (__e1,__e2);
                    }
                    
                     int versionsort (dirent const * * __e1,dirent const * * __e2){
                        return g_libc.versionsort_fn (__e1,__e2);
                    }
                    
                     int uname (struct utsname *__name){
                        return g_libc.uname_fn (__name);
                    }
                    
                     pid_t wait (void *stat_loc){
                        return g_libc.wait_fn (stat_loc);
                    }
                    
                     __pid_t waitpid (__pid_t __pid,int * __stat_loc,int __options){
                        return g_libc.waitpid_fn (__pid,__stat_loc,__options);
                    }
                    
                     char * dirname (char * __path){
                        return g_libc.dirname_fn (__path);
                    }
                    
                     char * __xpg_basename (char * __path){
                        return g_libc.__xpg_basename_fn (__path);
                    }
                    
                     group * getgrnam (char const * __name){
                        return g_libc.getgrnam_fn (__name);
                    }
                    
                     int getrusage (__rusage_who_t __who,rusage * __usage){
                        return g_libc.getrusage_fn (__who,__usage);
                    }
                    
                     int getrlimit (__rlimit_resource_t __resource,rlimit * __rlimits){
                        return g_libc.getrlimit_fn (__resource,__rlimits);
                    }
                    
                     int setrlimit (__rlimit_resource_t __resource,rlimit const * __rlimits){
                        return g_libc.setrlimit_fn (__resource,__rlimits);
                    }
                    
                     void openlog (char const * __ident,int __option,int __facility){
                         g_libc.openlog_fn (__ident,__option,__facility);
                    }
                    
                     void closelog (){
                         g_libc.closelog_fn ();
                    }
                    
                     int setlogmask (int __mask) noexcept {
                        return g_libc.setlogmask_fn (__mask);
                    }
                    
                     void syslog (int __pri,char const * __fmt,... ){
                        va_list vl;
                        va_start (vl, __fmt);
                         g_libc.syslog_fn (__pri,__fmt,vl);
                    }
                    
                     void vsyslog (int __pri,char const * __fmt,va_list __ap){
                         g_libc.vsyslog_fn (__pri,__fmt,__ap);
                    }
                    
                     int _setjmp (__jmp_buf_tag * __env){
                        return g_libc._setjmp_fn (__env);
                    }
                    
                     int __sigsetjmp (__jmp_buf_tag * __env,int __savemask){
                        return g_libc.__sigsetjmp_fn (__env,__savemask);
                    }
                    
                     void siglongjmp (__jmp_buf_tag * __env,int __val){
                         g_libc.siglongjmp_fn (__env,__val);
                    }
                    
                     char * bindtextdomain (char const * __domainname,char const * __dirname){
                        return g_libc.bindtextdomain_fn (__domainname,__dirname);
                    }
                    
                     char * textdomain (char const * __domainname){
                        return g_libc.textdomain_fn (__domainname);
                    }
                    
                     char * gettext (char const * __msgid){
                        return g_libc.gettext_fn (__msgid);
                    }
                    
                     nl_catd catopen (char const * __cat_name,int __flag){
                        return g_libc.catopen_fn (__cat_name,__flag);
                    }
                    
                     char * catgets (nl_catd __catalog,int __set,int __number,char const * __string){
                        return g_libc.catgets_fn (__catalog,__set,__number,__string);
                    }
                    
                     passwd * getpwnam (char const * __name){
                        return g_libc.getpwnam_fn (__name);
                    }
                    
                     passwd * getpwuid (__uid_t __uid){
                        return g_libc.getpwuid_fn (__uid);
                    }
                    
                     void endpwent (){
                         g_libc.endpwent_fn ();
                    }
                    
                     intmax_t strtoimax (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base){
                        return g_libc.strtoimax_fn (__nptr,__endptr,__base);
                    }
                    
                     uintmax_t strtoumax (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base){
                        return g_libc.strtoumax_fn (__nptr,__endptr,__base);
                    }
                    
                     ether_addr * ether_aton_r (char const * __asc,ether_addr * __addr){
                        return g_libc.ether_aton_r_fn (__asc,__addr);
                    }
                    
                     ether_addr * ether_aton (char const * __asc){
                        return g_libc.ether_aton_fn (__asc);
                    }
                    
                     void * tsearch (void const * __key,void * * __rootp,__compar_fn_t __compar){
                        return g_libc.tsearch_fn (__key,__rootp,__compar);
                    }
                    
                     void * tfind (void const * __key,void * const * __rootp,__compar_fn_t __compar){
                        return g_libc.tfind_fn (__key,__rootp,__compar);
                    }
                    
                     void * tdelete (__restrict__ void const * __key,__restrict__ void * * __rootp,__compar_fn_t __compar){
                        return g_libc.tdelete_fn (__key,__rootp,__compar);
                    }
                    
                     void twalk (void const * __root,__action_fn_t __action){
                         g_libc.twalk_fn (__root,__action);
                    }
                    
                     void tdestroy (void * __root,__free_fn_t __freefct){
                         g_libc.tdestroy_fn (__root,__freefct);
                    }
                    
                     int fnmatch (char const * __pattern,char const * __name,int __flags){
                        return g_libc.fnmatch_fn (__pattern,__name,__flags);
                    }
                    
                     char * nl_langinfo (nl_item __item){
                        return g_libc.nl_langinfo_fn (__item);
                    }
                    
                     int fstatfs (int __fildes, struct statfs * __buf) noexcept {
                        return g_libc.fstatfs_fn (__fildes,__buf);
                    }
                    
                     int fstatfs64 (int __fildes, struct statfs64 * __buf) {
                        return g_libc.fstatfs64_fn (__fildes,__buf);
                    }
                    
                     int statfs (const char *path, struct statfs *buf){
                         return g_libc.statfs_fn (path, buf);
                    }
                    
                     int statfs64 (const char *path, struct statfs64 *buf){
                         g_libc.statfs64_fn (path, buf);
                    }
                    
                     int statvfs (const char *path, struct statvfs *buf){
                         return g_libc.statvfs_fn (path, buf);
                    }
                    
                     int fstatvfs (int __fildes, struct statvfs * __buf){
                        return g_libc.fstatvfs_fn (__fildes,__buf);
                    }
                    
                     int tcgetattr (int __fd, struct termios * __termios_p) noexcept {
                        return g_libc.tcgetattr_fn (__fd,__termios_p);
                    }
                    
                     int tcsetattr (int __fd,int __optional_actions, struct termios const * __termios_p) noexcept {
                        return g_libc.tcsetattr_fn (__fd,__optional_actions,__termios_p);
                    }
                    
                     short unsigned int const * * __ctype_b_loc (){
                        return g_libc.__ctype_b_loc_fn ();
                    }
                    
                     wctype_t wctype_l (char const * __property,__locale_t __locale){
                        return g_libc.wctype_l_fn (__property,__locale);
                    }
                    
                     __int32_t const * * __ctype_tolower_loc (){
                        return g_libc.__ctype_tolower_loc_fn ();
                    }
                    
                     size_t __ctype_get_mb_cur_max (){
                        return g_libc.__ctype_get_mb_cur_max_fn ();
                    }
                    
                     void __fpurge (FILE *fd){
                         g_libc.__fpurge_fn (fd);
                    }
                    
                     size_t __fpending (FILE *fd){
                        return g_libc.__fpending_fn (fd);
                    }
                    
                     int fstat64 (int __fd, struct stat64 *__buf){
                        return g_libc.fstat64_fn (__fd, __buf);
                    }
                    
                     int creat (char const * __file,mode_t __mode){
                        return g_libc.creat_fn (__file,__mode);
                    }
                    
                     long int lrintl (long double __x){
                        return g_libc.lrintl_fn (__x);
                    }
                    
                     long long int llrintl (long double __x){
                        return g_libc.llrintl_fn (__x);
                    }
                    
                     double ceil (double __x){
                        return g_libc.ceil_fn (__x);
                    }
                    
                     double floor (double __x){
                        return g_libc.floor_fn (__x);
                    }
                    
                     void * dlopen (char const * __file,int __mode){
                        return g_libc.dlopen_fn (__file,__mode);
                    }
                    
                     void * dlsym (__restrict__ void * __handle,__restrict__ char const * __name){
                        return g_libc.dlsym_fn (__handle,__name);
                    }
                    
                     int dl_iterate_phdr (int (*__callback)( ::dl_phdr_info *,::size_t,void * ) ,void * __data){
                        return g_libc.dl_iterate_phdr_fn (__callback,__data);
                    }

//decltype (newlocale) newlocale __attribute__ ((weak, alias ("__newlocale")));
                    decltype (malloc) valloc __attribute__ ((weak, alias ("malloc")));
                    decltype (strtol) __strtol_internal __attribute__ ((weak, alias ("strtol")));
                    decltype (newlocale) __newlocale __attribute__ ((weak, alias ("newlocale")));
                    decltype (uselocale) __uselocale __attribute__ ((weak, alias ("uselocale")));
                    decltype (clearerr) clearerr_unlocked __attribute__ ((weak, alias ("clearerr")));
decltype (clearerr)  __clearerr_unlocked __attribute__ ((weak, alias ("clearerr")));
                    decltype (fgetc) fgetc_unlocked __attribute__ ((weak, alias ("fgetc")));
                    decltype (getchar) getchar_unlocked __attribute__ ((weak, alias ("getchar")));
                    decltype (fputc) fputc_unlocked __attribute__ ((weak, alias ("fputc")));
                    decltype (putchar) putchar_unlocked __attribute__ ((weak, alias ("putchar")));
                    decltype (fgets) fgets_unlocked __attribute__ ((weak, alias ("fgets")));
                    decltype (fputs) fputs_unlocked __attribute__ ((weak, alias ("fputs")));
                    decltype (fread) fread_unlocked __attribute__ ((weak, alias ("fread")));
                    decltype (fwrite) fwrite_unlocked __attribute__ ((weak, alias ("fwrite")));
                    decltype (fflush) fflush_unlocked __attribute__ ((weak, alias ("fflush")));
                    decltype (ferror) ferror_unlocked __attribute__ ((weak, alias ("ferror")));
                    decltype (feof) feof_unlocked __attribute__ ((weak, alias ("feof")));
                    decltype (fileno) fileno_unlocked __attribute__ ((weak, alias ("fileno")));
                    decltype (sscanf) __isoc99_sscanf __attribute__ ((weak, alias ("sscanf")));
                    decltype (gmtime) localtime __attribute__ ((weak, alias ("gmtime")));
                    decltype (gmtime_r) localtime_r __attribute__ ((weak, alias ("gmtime_r")));
                    decltype (pthread_once) __pthread_once __attribute__ ((weak, alias ("pthread_once")));
                    decltype (pthread_key_create) __pthread_key_create __attribute__ ((weak, alias ("pthread_key_create")));
                    decltype (wctype_l) __wctype_l __attribute__ ((weak, alias ("wctype_l")));
                    decltype (dlopen) __dlopen __attribute__ ((weak, alias ("dlopen")));

