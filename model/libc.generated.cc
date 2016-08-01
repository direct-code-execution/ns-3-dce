
                     int atexit (void (*__func)(  ) ){
                        return g_libc.atexit_fn ();
                    }
                    
                     long int random (){
                        return g_libc.random_fn ();
                    }
                    
                     void srandom (unsigned int){
                         g_libc.srandom_fn (__seed);
                    }
                    
                     int rand (){
                        return g_libc.rand_fn ();
                    }
                    
                     void srand (unsigned int){
                         g_libc.srand_fn (__seed);
                    }
                    
                     double drand48 (){
                        return g_libc.drand48_fn ();
                    }
                    
                     double erand48 (short unsigned int *){
                        return g_libc.erand48_fn (__xsubi);
                    }
                    
                     long int lrand48 (){
                        return g_libc.lrand48_fn ();
                    }
                    
                     long int nrand48 (short unsigned int *){
                        return g_libc.nrand48_fn (__xsubi);
                    }
                    
                     long int mrand48 (){
                        return g_libc.mrand48_fn ();
                    }
                    
                     long int jrand48 (short unsigned int *){
                        return g_libc.jrand48_fn (__xsubi);
                    }
                    
                     void srand48 (long int){
                         g_libc.srand48_fn (__seedval);
                    }
                    
                     short unsigned int * seed48 (short unsigned int *){
                        return g_libc.seed48_fn (__seed16v);
                    }
                    
                     void lcong48 (short unsigned int *){
                         g_libc.lcong48_fn (__param);
                    }
                    
                     int drand48_r (__restrict__ ::drand48_data *,__restrict__ double *){
                        return g_libc.drand48_r_fn (__buffer,__result);
                    }
                    
                     int erand48_r (short unsigned int *,__restrict__ ::drand48_data *,__restrict__ double *){
                        return g_libc.erand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int lrand48_r (__restrict__ ::drand48_data *,__restrict__ long int *){
                        return g_libc.lrand48_r_fn (__buffer,__result);
                    }
                    
                     int nrand48_r (short unsigned int *,__restrict__ ::drand48_data *,__restrict__ long int *){
                        return g_libc.nrand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int mrand48_r (__restrict__ ::drand48_data *,__restrict__ long int *){
                        return g_libc.mrand48_r_fn (__buffer,__result);
                    }
                    
                     int jrand48_r (short unsigned int *,__restrict__ ::drand48_data *,__restrict__ long int *){
                        return g_libc.jrand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int srand48_r (long int,drand48_data *){
                        return g_libc.srand48_r_fn (__seedval,__buffer);
                    }
                    
                     int seed48_r (short unsigned int *,drand48_data *){
                        return g_libc.seed48_r_fn (__seed16v,__buffer);
                    }
                    
                     int lcong48_r (short unsigned int *,drand48_data *){
                        return g_libc.lcong48_r_fn (__param,__buffer);
                    }
                    
                     void * calloc (size_t,size_t){
                        return g_libc.calloc_fn (__nmemb,__size);
                    }
                    
                     void * malloc (size_t){
                        return g_libc.malloc_fn (__size);
                    }
                    
                     void free (void *){
                         g_libc.free_fn (__ptr);
                    }
                    
                     void * realloc (void *,size_t){
                        return g_libc.realloc_fn (__ptr,__size);
                    }
                    
                     int atoi (char const *){
                        return g_libc.atoi_fn (__nptr);
                    }
                    
                     long int atol (char const *){
                        return g_libc.atol_fn (__nptr);
                    }
                    
                     long long int atoll (char const *){
                        return g_libc.atoll_fn (__nptr);
                    }
                    
                     double atof (char const *){
                        return g_libc.atof_fn (__nptr);
                    }
                    
                     long int strtol (__restrict__ char const *,__restrict__ char * *,int){
                        return g_libc.strtol_fn (__nptr,__endptr,__base);
                    }
                    
                     long long int strtoll (__restrict__ char const *,__restrict__ char * *,int){
                        return g_libc.strtoll_fn (__nptr,__endptr,__base);
                    }
                    
                     long unsigned int strtoul (__restrict__ char const *,__restrict__ char * *,int){
                        return g_libc.strtoul_fn (__nptr,__endptr,__base);
                    }
                    
                     long long unsigned int strtoull (__restrict__ char const *,__restrict__ char * *,int){
                        return g_libc.strtoull_fn (__nptr,__endptr,__base);
                    }
                    
                     double strtod (__restrict__ char const *,__restrict__ char * *){
                        return g_libc.strtod_fn (__nptr,__endptr);
                    }
                    
                     char * getenv (char const *){
                        return g_libc.getenv_fn (__name);
                    }
                    
                     int putenv (char *){
                        return g_libc.putenv_fn (__string);
                    }
                    
                     int setenv (char const *,char const *,int){
                        return g_libc.setenv_fn (__name,__value,__replace);
                    }
                    
                     int unsetenv (char const *){
                        return g_libc.unsetenv_fn (__name);
                    }
                    
                     int clearenv (){
                        return g_libc.clearenv_fn ();
                    }
                    
                     void qsort (void *,size_t,size_t,__compar_fn_t){
                         g_libc.qsort_fn (__base,__nmemb,__size,__compar);
                    }
                    
                     void abort (){
                         g_libc.abort_fn ();
                    }
                    
                     int mkstemp (char *){
                        return g_libc.mkstemp_fn (__template);
                    }
                    
                     FILE * tmpfile (){
                        return g_libc.tmpfile_fn ();
                    }
                    
                     int rename (char const *,char const *){
                        return g_libc.rename_fn (__old,__new);
                    }
                    
                     void bzero (void *,size_t){
                         g_libc.bzero_fn (__s,__n);
                    }
                    
                     char * strerror (int){
                        return g_libc.strerror_fn (__errnum);
                    }
                    
                     char * strerror_r (int,char *,size_t){
                        return g_libc.strerror_r_fn (__errnum,__buf,__buflen);
                    }
                    
                     int strcoll (char const *,char const *){
                        return g_libc.strcoll_fn (__s1,__s2);
                    }
                    
                     void * memset (void *,int,size_t){
                        return g_libc.memset_fn (__s,__c,__n);
                    }
                    
                     void * memcpy (__restrict__ void *,__restrict__ void const *,size_t){
                        return g_libc.memcpy_fn (__dest,__src,__n);
                    }
                    
                     void bcopy (void const *,void *,size_t){
                         g_libc.bcopy_fn (__src,__dest,__n);
                    }
                    
                     int memcmp (void const *,void const *,size_t){
                        return g_libc.memcmp_fn (__s1,__s2,__n);
                    }
                    
                     void * memmove (void *,void const *,size_t){
                        return g_libc.memmove_fn (__dest,__src,__n);
                    }
                    
                     char * strcpy (__restrict__ char *,__restrict__ char const *){
                        return g_libc.strcpy_fn (__dest,__src);
                    }
                    
                     char * strncpy (__restrict__ char *,__restrict__ char const *,size_t){
                        return g_libc.strncpy_fn (__dest,__src,__n);
                    }
                    
                     char * strcat (__restrict__ char *,__restrict__ char const *){
                        return g_libc.strcat_fn (__dest,__src);
                    }
                    
                     char * strncat (__restrict__ char *,__restrict__ char const *,size_t){
                        return g_libc.strncat_fn (__dest,__src,__n);
                    }
                    
                     int strcmp (char const *,char const *){
                        return g_libc.strcmp_fn (__s1,__s2);
                    }
                    
                     int strncmp (char const *,char const *,size_t){
                        return g_libc.strncmp_fn (__s1,__s2,__n);
                    }
                    
                     size_t strlen (char const *){
                        return g_libc.strlen_fn (__s);
                    }
                    
                     size_t strnlen (char const *,size_t){
                        return g_libc.strnlen_fn (__string,__maxlen);
                    }
                    
                     size_t strcspn (char const *,char const *){
                        return g_libc.strcspn_fn (__s,__reject);
                    }
                    
                     size_t strspn (char const *,char const *){
                        return g_libc.strspn_fn (__s,__accept);
                    }
                    
                     int strcasecmp (char const *,char const *){
                        return g_libc.strcasecmp_fn (__s1,__s2);
                    }
                    
                     int strncasecmp (char const *,char const *,size_t){
                        return g_libc.strncasecmp_fn (__s1,__s2,__n);
                    }
                    
                     char * strdup (char const *){
                        return g_libc.strdup_fn (__s);
                    }
                    
                     char * strndup (char const *,size_t){
                        return g_libc.strndup_fn (__string,__n);
                    }
                    
                     char * strsep (__restrict__ char * *,__restrict__ char const *){
                        return g_libc.strsep_fn (__stringp,__delim);
                    }
                    
                     char * setlocale (int,char const *){
                        return g_libc.setlocale_fn (__category,__locale);
                    }
                    
                     __locale_t newlocale (int,char const *,__locale_t){
                        return g_libc.newlocale_fn (__category_mask,__locale,__base);
                    }
                    
                     __locale_t uselocale (__locale_t){
                        return g_libc.uselocale_fn (__dataset);
                    }
                    
                     int wctob (wint_t){
                        return g_libc.wctob_fn (__c);
                    }
                    
                     wint_t btowc (int){
                        return g_libc.btowc_fn (__c);
                    }
                    
                     size_t mbrlen (__restrict__ char const *,size_t,__restrict__ ::mbstate_t *){
                        return g_libc.mbrlen_fn (__s,__n,__ps);
                    }
                    
                     uint32_t htonl (uint32_t){
                        return g_libc.htonl_fn (__hostlong);
                    }
                    
                     uint16_t htons (uint16_t){
                        return g_libc.htons_fn (__hostshort);
                    }
                    
                     uint32_t ntohl (uint32_t){
                        return g_libc.ntohl_fn (__netlong);
                    }
                    
                     uint16_t ntohs (uint16_t){
                        return g_libc.ntohs_fn (__netshort);
                    }
                    
                     int lockf (int,int,off_t){
                        return g_libc.lockf_fn (__fd,__cmd,__len);
                    }
                    
                     int inet_aton (char const *,in_addr *){
                        return g_libc.inet_aton_fn (__cp,__inp);
                    }
                    
                     in_addr_t inet_addr (char const *){
                        return g_libc.inet_addr_fn (__cp);
                    }
                    
                     in_addr_t inet_network (char const *){
                        return g_libc.inet_network_fn (__cp);
                    }
                    
                     char * inet_ntoa (in_addr){
                        return g_libc.inet_ntoa_fn (__in);
                    }
                    
                     in_addr inet_makeaddr (in_addr_t,in_addr_t){
                        return g_libc.inet_makeaddr_fn (__net,__host);
                    }
                    
                     in_addr_t inet_lnaof (in_addr){
                        return g_libc.inet_lnaof_fn (__in);
                    }
                    
                     in_addr_t inet_netof (in_addr){
                        return g_libc.inet_netof_fn (__in);
                    }
                    
                     char const * inet_ntop (int,__restrict__ void const *,__restrict__ char *,socklen_t){
                        return g_libc.inet_ntop_fn (__af,__cp,__buf,__len);
                    }
                    
                     int inet_pton (int,__restrict__ char const *,__restrict__ void *){
                        return g_libc.inet_pton_fn (__af,__cp,__buf);
                    }
                    
                     int inet6_opt_find (void *,socklen_t,int,uint8_t,socklen_t *,void * *){
                        return g_libc.inet6_opt_find_fn (__extbuf,__extlen,__offset,__type,__lenp,__databufp);
                    }
                    
                     int socket (int,int,int){
                        return g_libc.socket_fn (__domain,__type,__protocol);
                    }
                    
                     int socketpair (int,int,int,int *){
                        return g_libc.socketpair_fn (__domain,__type,__protocol,__fds);
                    }
                    
                     int getsockname (int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *){
                        return g_libc.getsockname_fn (__fd,__addr,__len);
                    }
                    
                     int getpeername (int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *){
                        return g_libc.getpeername_fn (__fd,__addr,__len);
                    }
                    
                     int bind (int,sockaddr const *,socklen_t){
                        return g_libc.bind_fn (__fd,__addr,__len);
                    }
                    
                     int connect (int,sockaddr const *,socklen_t){
                        return g_libc.connect_fn (__fd,__addr,__len);
                    }
                    
                     int setsockopt (int,int,int,void const *,socklen_t){
                        return g_libc.setsockopt_fn (__fd,__level,__optname,__optval,__optlen);
                    }
                    
                     int getsockopt (int,int,int,__restrict__ void *,__restrict__ ::socklen_t *){
                        return g_libc.getsockopt_fn (__fd,__level,__optname,__optval,__optlen);
                    }
                    
                     int listen (int,int){
                        return g_libc.listen_fn (__fd,__n);
                    }
                    
                     int accept (int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *){
                        return g_libc.accept_fn (__fd,__addr,__addr_len);
                    }
                    
                     int shutdown (int,int){
                        return g_libc.shutdown_fn (__fd,__how);
                    }
                    
                     ssize_t send (int,void const *,size_t,int){
                        return g_libc.send_fn (__fd,__buf,__n,__flags);
                    }
                    
                     ssize_t sendto (int,void const *,size_t,int,sockaddr const *,socklen_t){
                        return g_libc.sendto_fn (__fd,__buf,__n,__flags,__addr,__addr_len);
                    }
                    
                     ssize_t sendmsg (int,msghdr const *,int){
                        return g_libc.sendmsg_fn (__fd,__message,__flags);
                    }
                    
                     ssize_t recv (int,void *,size_t,int){
                        return g_libc.recv_fn (__fd,__buf,__n,__flags);
                    }
                    
                     ssize_t recvfrom (int,__restrict__ void *,size_t,int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *){
                        return g_libc.recvfrom_fn (__fd,__buf,__n,__flags,__addr,__addr_len);
                    }
                    
                     ssize_t recvmsg (int,msghdr *,int){
                        return g_libc.recvmsg_fn (__fd,__message,__flags);
                    }
                    
                     int getnameinfo (__restrict__ ::sockaddr const *,socklen_t,__restrict__ char *,socklen_t,__restrict__ char *,socklen_t,int){
                        return g_libc.getnameinfo_fn (__sa,__salen,__host,__hostlen,__serv,__servlen,__flags);
                    }
                    
                     ssize_t read (int,void *,size_t){
                        return g_libc.read_fn (__fd,__buf,__nbytes);
                    }
                    
                     ssize_t write (int,void const *,size_t){
                        return g_libc.write_fn (__fd,__buf,__n);
                    }
                    
                     unsigned int sleep (unsigned int){
                        return g_libc.sleep_fn (__seconds);
                    }
                    
                     int usleep (__useconds_t){
                        return g_libc.usleep_fn (__useconds);
                    }
                    
                     int getopt (int,char * const *,char const *){
                        return g_libc.getopt_fn (___argc,___argv,__shortopts);
                    }
                    
                     int getopt_long (int,char * const *,char const *,option const *,int *){
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
                    
                     int setuid (__uid_t){
                        return g_libc.setuid_fn (__uid);
                    }
                    
                     int setgid (__gid_t){
                        return g_libc.setgid_fn (__gid);
                    }
                    
                     int seteuid (__uid_t){
                        return g_libc.seteuid_fn (__uid);
                    }
                    
                     int setegid (__gid_t){
                        return g_libc.setegid_fn (__gid);
                    }
                    
                     int setreuid (__uid_t,__uid_t){
                        return g_libc.setreuid_fn (__ruid,__euid);
                    }
                    
                     int setregid (__gid_t,__gid_t){
                        return g_libc.setregid_fn (__rgid,__egid);
                    }
                    
                     int setresuid (__uid_t,__uid_t,__uid_t){
                        return g_libc.setresuid_fn (__ruid,__euid,__suid);
                    }
                    
                     int setresgid (__gid_t,__gid_t,__gid_t){
                        return g_libc.setresgid_fn (__rgid,__egid,__sgid);
                    }
                    
                     int dup (int){
                        return g_libc.dup_fn (__fd);
                    }
                    
                     int dup2 (int,int){
                        return g_libc.dup2_fn (__fd,__fd2);
                    }
                    
                     int close (int){
                        return g_libc.close_fn (__fd);
                    }
                    
                     int unlink (char const *){
                        return g_libc.unlink_fn (__name);
                    }
                    
                     int rmdir (char const *){
                        return g_libc.rmdir_fn (__path);
                    }
                    
                     int select (int,__restrict__ ::fd_set *,__restrict__ ::fd_set *,__restrict__ ::fd_set *,__restrict__ ::timeval *){
                        return g_libc.select_fn (__nfds,__readfds,__writefds,__exceptfds,__timeout);
                    }
                    
                     int isatty (int){
                        return g_libc.isatty_fn (__fd);
                    }
                    
                     void exit (int){
                         g_libc.exit_fn (__status);
                    }
                    
                     char * getcwd (char *,size_t){
                        return g_libc.getcwd_fn (__buf,__size);
                    }
                    
                     char * getwd (char *){
                        return g_libc.getwd_fn (__buf);
                    }
                    
                     char * get_current_dir_name (){
                        return g_libc.get_current_dir_name_fn ();
                    }
                    
                     int chdir (char const *){
                        return g_libc.chdir_fn (__path);
                    }
                    
                     int fchdir (int){
                        return g_libc.fchdir_fn (__fd);
                    }
                    
                     __pid_t fork (){
                        return g_libc.fork_fn ();
                    }
                    
                     int execv (char const *,char * const *){
                        return g_libc.execv_fn (__path,__argv);
                    }
                    
                     int execl (char const *,char const *,...){
                        return g_libc.execl_fn (__path,__arg,);
                    }
                    
                     int execve (char const *,char * const *,char * const *){
                        return g_libc.execve_fn (__path,__argv,__envp);
                    }
                    
                     int execvp (char const *,char * const *){
                        return g_libc.execvp_fn (__file,__argv);
                    }
                    
                     int execlp (char const *,char const *,...){
                        return g_libc.execlp_fn (__file,__arg,);
                    }
                    
                     int execle (char const *,char const *,...){
                        return g_libc.execle_fn (__path,__arg,);
                    }
                    
                     int truncate (char const *,__off_t){
                        return g_libc.truncate_fn (__file,__length);
                    }
                    
                     int ftruncate (int,__off_t){
                        return g_libc.ftruncate_fn (__fd,__length);
                    }
                    
                     int ftruncate64 (int,__off64_t){
                        return g_libc.ftruncate64_fn (__fd,__length);
                    }
                    
                     long int sysconf (int){
                        return g_libc.sysconf_fn (__name);
                    }
                    
                     char * ttyname (int){
                        return g_libc.ttyname_fn (__fd);
                    }
                    
                     void * sbrk (intptr_t){
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
                    
                     int gethostname (char *,size_t){
                        return g_libc.gethostname_fn (__name,__len);
                    }
                    
                     __pid_t getpgrp (){
                        return g_libc.getpgrp_fn ();
                    }
                    
                     __off_t lseek (int,__off_t,int){
                        return g_libc.lseek_fn (__fd,__offset,__whence);
                    }
                    
                     __off64_t lseek64 (int,__off64_t,int){
                        return g_libc.lseek64_fn (__fd,__offset,__whence);
                    }
                    
                     int euidaccess (char const *,int){
                        return g_libc.euidaccess_fn (__name,__type);
                    }
                    
                     int eaccess (char const *,int){
                        return g_libc.eaccess_fn (__name,__type);
                    }
                    
                     int access (char const *,int){
                        return g_libc.access_fn (__name,__type);
                    }
                    
                     int pipe (int *){
                        return g_libc.pipe_fn (__pipedes);
                    }
                    
                     long int pathconf (char const *,int){
                        return g_libc.pathconf_fn (__path,__name);
                    }
                    
                     int getdtablesize (){
                        return g_libc.getdtablesize_fn ();
                    }
                    
                     ssize_t pread (int,void *,size_t,__off_t){
                        return g_libc.pread_fn (__fd,__buf,__nbytes,__offset);
                    }
                    
                     ssize_t pwrite (int,void const *,size_t,__off_t){
                        return g_libc.pwrite_fn (__fd,__buf,__n,__offset);
                    }
                    
                     int daemon (int,int){
                        return g_libc.daemon_fn (__nochdir,__noclose);
                    }
                    
                     unsigned int alarm (unsigned int){
                        return g_libc.alarm_fn (__seconds);
                    }
                    
                     ssize_t readlink (__restrict__ char const *,__restrict__ char *,size_t){
                        return g_libc.readlink_fn (__path,__buf,__len);
                    }
                    
                     int chown (char const *,__uid_t,__gid_t){
                        return g_libc.chown_fn (__file,__owner,__group);
                    }
                    
                     int initgroups (char const *,__gid_t){
                        return g_libc.initgroups_fn (__user,__group);
                    }
                    
                     int fsync (int){
                        return g_libc.fsync_fn (__fd);
                    }
                    
                     ssize_t readv (int,iovec const *,int){
                        return g_libc.readv_fn (__fd,__iovec,__count);
                    }
                    
                     ssize_t writev (int,iovec const *,int){
                        return g_libc.writev_fn (__fd,__iovec,__count);
                    }
                    
                     void clearerr (FILE *){
                         g_libc.clearerr_fn (__stream);
                    }
                    
                     int setvbuf (__restrict__ ::FILE *,__restrict__ char *,int,size_t){
                        return g_libc.setvbuf_fn (__stream,__buf,__modes,__n);
                    }
                    
                     void setbuf (__restrict__ ::FILE *,__restrict__ char *){
                         g_libc.setbuf_fn (__stream,__buf);
                    }
                    
                     void setbuffer (__restrict__ ::FILE *,__restrict__ char *,size_t){
                         g_libc.setbuffer_fn (__stream,__buf,__size);
                    }
                    
                     void setlinebuf (FILE *){
                         g_libc.setlinebuf_fn (__stream);
                    }
                    
                     int fseek (FILE *,long int,int){
                        return g_libc.fseek_fn (__stream,__off,__whence);
                    }
                    
                     long int ftell (FILE *){
                        return g_libc.ftell_fn (__stream);
                    }
                    
                     int fseeko (FILE *,__off_t,int){
                        return g_libc.fseeko_fn (__stream,__off,__whence);
                    }
                    
                     __off_t ftello (FILE *){
                        return g_libc.ftello_fn (__stream);
                    }
                    
                     void rewind (FILE *){
                         g_libc.rewind_fn (__stream);
                    }
                    
                     int fgetpos (__restrict__ ::FILE *,__restrict__ ::fpos_t *){
                        return g_libc.fgetpos_fn (__stream,__pos);
                    }
                    
                     int fsetpos (FILE *,fpos_t const *){
                        return g_libc.fsetpos_fn (__stream,__pos);
                    }
                    
                     int printf (__restrict__ char const *,...){
                        return g_libc.printf_fn (__format,);
                    }
                    
                     int fprintf (__restrict__ ::FILE *,__restrict__ char const *,...){
                        return g_libc.fprintf_fn (__stream,__format,);
                    }
                    
                     int sprintf (__restrict__ char *,__restrict__ char const *,...){
                        return g_libc.sprintf_fn (__s,__format,);
                    }
                    
                     int asprintf (__restrict__ char * *,__restrict__ char const *,...){
                        return g_libc.asprintf_fn (__ptr,__fmt,);
                    }
                    
                     int vasprintf (__restrict__ char * *,__restrict__ char const *,va_list){
                        return g_libc.vasprintf_fn (__ptr,__f,__arg);
                    }
                    
                     int dprintf (int,__restrict__ char const *,...){
                        return g_libc.dprintf_fn (__fd,__fmt,);
                    }
                    
                     int vdprintf (int,__restrict__ char const *,va_list){
                        return g_libc.vdprintf_fn (__fd,__fmt,__arg);
                    }
                    
                     int fgetc (FILE *){
                        return g_libc.fgetc_fn (__stream);
                    }
                    
                     int getc (FILE *){
                        return g_libc.getc_fn (__stream);
                    }
                    
                     int getc_unlocked (FILE *){
                        return g_libc.getc_unlocked_fn (__stream);
                    }
                    
                     int getchar (){
                        return g_libc.getchar_fn ();
                    }
                    
                     int _IO_getc (_IO_FILE *){
                        return g_libc._IO_getc_fn (__fp);
                    }
                    
                     int fputc (int,FILE *){
                        return g_libc.fputc_fn (__c,__stream);
                    }
                    
                     int putc (int,FILE *){
                        return g_libc.putc_fn (__c,__stream);
                    }
                    
                     int putc_unlocked (int,FILE *){
                        return g_libc.putc_unlocked_fn (__c,__stream);
                    }
                    
                     int putchar (int){
                        return g_libc.putchar_fn (__c);
                    }
                    
                     int _IO_putc (int,_IO_FILE *){
                        return g_libc._IO_putc_fn (__c,__fp);
                    }
                    
                     char * fgets (__restrict__ char *,int,__restrict__ ::FILE *){
                        return g_libc.fgets_fn (__s,__n,__stream);
                    }
                    
                     int fputs (__restrict__ char const *,__restrict__ ::FILE *){
                        return g_libc.fputs_fn (__s,__stream);
                    }
                    
                     int puts (char const *){
                        return g_libc.puts_fn (__s);
                    }
                    
                     int ungetc (int,FILE *){
                        return g_libc.ungetc_fn (__c,__stream);
                    }
                    
                     int fclose (FILE *){
                        return g_libc.fclose_fn (__stream);
                    }
                    
                     int fcloseall (){
                        return g_libc.fcloseall_fn ();
                    }
                    
                     FILE * fopen (__restrict__ char const *,__restrict__ char const *){
                        return g_libc.fopen_fn (__filename,__modes);
                    }
                    
                     FILE * fopen64 (__restrict__ char const *,__restrict__ char const *){
                        return g_libc.fopen64_fn (__filename,__modes);
                    }
                    
                     FILE * freopen (__restrict__ char const *,__restrict__ char const *,__restrict__ ::FILE *){
                        return g_libc.freopen_fn (__filename,__modes,__stream);
                    }
                    
                     FILE * fdopen (int,char const *){
                        return g_libc.fdopen_fn (__fd,__modes);
                    }
                    
                     size_t fread (__restrict__ void *,size_t,size_t,__restrict__ ::FILE *){
                        return g_libc.fread_fn (__ptr,__size,__n,__stream);
                    }
                    
                     size_t fwrite (__restrict__ void const *,size_t,size_t,__restrict__ ::FILE *){
                        return g_libc.fwrite_fn (__ptr,__size,__n,__s);
                    }
                    
                     int fflush (FILE *){
                        return g_libc.fflush_fn (__stream);
                    }
                    
                     int ferror (FILE *){
                        return g_libc.ferror_fn (__stream);
                    }
                    
                     int feof (FILE *){
                        return g_libc.feof_fn (__stream);
                    }
                    
                     int fileno (FILE *){
                        return g_libc.fileno_fn (__stream);
                    }
                    
                     void perror (char const *){
                         g_libc.perror_fn (__s);
                    }
                    
                     int remove (char const *){
                        return g_libc.remove_fn (__filename);
                    }
                    
                     int sscanf (__restrict__ char const *,__restrict__ char const *,...){
                        return g_libc.sscanf_fn (__s,__format,);
                    }
                    
                     void flockfile (FILE *){
                         g_libc.flockfile_fn (__stream);
                    }
                    
                     void funlockfile (FILE *){
                         g_libc.funlockfile_fn (__stream);
                    }
                    
                     int vprintf (__restrict__ char const *,va_list){
                        return g_libc.vprintf_fn (__format,__arg);
                    }
                    
                     int vfprintf (__restrict__ ::FILE *,__restrict__ char const *,va_list){
                        return g_libc.vfprintf_fn (__s,__format,__arg);
                    }
                    
                     int vsprintf (__restrict__ char *,__restrict__ char const *,va_list){
                        return g_libc.vsprintf_fn (__s,__format,__arg);
                    }
                    
                     int fcntl (int,int,...){
                        return g_libc.fcntl_fn (__fd,__cmd,);
                    }
                    
                     int open (char const *,int,...){
                        return g_libc.open_fn (__file,__oflag,);
                    }
                    
                     int open64 (char const *,int,...){
                        return g_libc.open64_fn (__file,__oflag,);
                    }
                    
                     int unlinkat (int,char const *,int){
                        return g_libc.unlinkat_fn (__fd,__name,__flag);
                    }
                    
                     int nanosleep (timespec const *,timespec *){
                        return g_libc.nanosleep_fn (__requested_time,__remaining);
                    }
                    
                     char * asctime (tm const *){
                        return g_libc.asctime_fn (__tp);
                    }
                    
                     char * asctime_r (__restrict__ ::tm const *,__restrict__ char *){
                        return g_libc.asctime_r_fn (__tp,__buf);
                    }
                    
                     char * ctime (time_t const *){
                        return g_libc.ctime_fn (__timer);
                    }
                    
                     char * ctime_r (__restrict__ ::time_t const *,__restrict__ char *){
                        return g_libc.ctime_r_fn (__timer,__buf);
                    }
                    
                     tm * gmtime (time_t const *){
                        return g_libc.gmtime_fn (__timer);
                    }
                    
                     tm * gmtime_r (__restrict__ ::time_t const *,__restrict__ ::tm *){
                        return g_libc.gmtime_r_fn (__timer,__tp);
                    }
                    
                     time_t mktime (tm *){
                        return g_libc.mktime_fn (__tp);
                    }
                    
                     size_t strftime (__restrict__ char *,size_t,__restrict__ char const *,__restrict__ ::tm const *){
                        return g_libc.strftime_fn (__s,__maxsize,__format,__tp);
                    }
                    
                     char * strptime (__restrict__ char const *,__restrict__ char const *,tm *){
                        return g_libc.strptime_fn (__s,__fmt,__tp);
                    }
                    
                     time_t timegm (tm *){
                        return g_libc.timegm_fn (__tp);
                    }
                    
                     time_t timelocal (tm *){
                        return g_libc.timelocal_fn (__tp);
                    }
                    
                     int timer_create (clockid_t,__restrict__ ::sigevent *,__restrict__ ::timer_t *){
                        return g_libc.timer_create_fn (__clock_id,__evp,__timerid);
                    }
                    
                     int timer_settime (timer_t,int,__restrict__ ::itimerspec const *,__restrict__ ::itimerspec *){
                        return g_libc.timer_settime_fn (__timerid,__flags,__value,__ovalue);
                    }
                    
                     int timer_gettime (timer_t,itimerspec *){
                        return g_libc.timer_gettime_fn (__timerid,__value);
                    }
                    
                     int utime (char const *,utimbuf const *){
                        return g_libc.utime_fn (__file,__file_times);
                    }
                    
                     void tzset (){
                         g_libc.tzset_fn ();
                    }
                    
                     int gettimeofday (__restrict__ ::timeval *,__timezone_ptr_t){
                        return g_libc.gettimeofday_fn (__tv,__tz);
                    }
                    
                     time_t time (time_t *){
                        return g_libc.time_fn (__timer);
                    }
                    
                     int setitimer (__itimer_which_t,__restrict__ ::itimerval const *,__restrict__ ::itimerval *){
                        return g_libc.setitimer_fn (__which,__new,__old);
                    }
                    
                     int getitimer (__itimer_which_t,itimerval *){
                        return g_libc.getitimer_fn (__which,__value);
                    }
                    
                     int sysinfo (struct sysinfo *info){
                        return g_libc.sysinfo_fn (info);
                    }
                    
                     void * mmap (void *,size_t,int,int,int,__off_t){
                        return g_libc.mmap_fn (__addr,__len,__prot,__flags,__fd,__offset);
                    }
                    
                     void * mmap64 (void *,size_t,int,int,int,__off64_t){
                        return g_libc.mmap64_fn (__addr,__len,__prot,__flags,__fd,__offset);
                    }
                    
                     int munmap (void *,size_t){
                        return g_libc.munmap_fn (__addr,__len);
                    }
                    
                     int mkdir (char const *,__mode_t){
                        return g_libc.mkdir_fn (__path,__mode);
                    }
                    
                     __mode_t umask (__mode_t){
                        return g_libc.umask_fn (__mask);
                    }
                    
                     int ioctl (int,long unsigned int,...){
                        return g_libc.ioctl_fn (__fd,__request,);
                    }
                    
                     int sched_yield (){
                        return g_libc.sched_yield_fn ();
                    }
                    
                     int poll (pollfd *,nfds_t,int){
                        return g_libc.poll_fn (__fds,__nfds,__timeout);
                    }
                    
                     __sighandler_t signal (int,__sighandler_t){
                        return g_libc.signal_fn (__sig,__handler);
                    }
                    
                     int sigaction (int signum, const struct sigaction *act, struct sigaction *oldact){
                        return g_libc.sigaction_fn (signum, act, oldact);
                    }
                    
                     int sigemptyset (sigset_t *){
                        return g_libc.sigemptyset_fn (__set);
                    }
                    
                     int sigfillset (sigset_t *){
                        return g_libc.sigfillset_fn (__set);
                    }
                    
                     int sigaddset (sigset_t *,int){
                        return g_libc.sigaddset_fn (__set,__signo);
                    }
                    
                     int sigdelset (sigset_t *,int){
                        return g_libc.sigdelset_fn (__set,__signo);
                    }
                    
                     int sigismember (sigset_t const *,int){
                        return g_libc.sigismember_fn (__set,__signo);
                    }
                    
                     int sigprocmask (int,__restrict__ ::sigset_t const *,__restrict__ ::sigset_t *){
                        return g_libc.sigprocmask_fn (__how,__set,__oset);
                    }
                    
                     int sigwait (__restrict__ ::sigset_t const *,__restrict__ int *){
                        return g_libc.sigwait_fn (__set,__sig);
                    }
                    
                     int kill (__pid_t,int){
                        return g_libc.kill_fn (__pid,__sig);
                    }
                    
                     int pthread_create (__restrict__ ::pthread_t *,__restrict__ ::pthread_attr_t const *,void * (*)( void * ) ,__restrict__ void *){
                        return g_libc.pthread_create_fn (__newthread,__attr,__start_routine,__arg);
                    }
                    
                     void pthread_exit (void *){
                         g_libc.pthread_exit_fn (__retval);
                    }
                    
                     pthread_t pthread_self (){
                        return g_libc.pthread_self_fn ();
                    }
                    
                     int pthread_once (pthread_once_t *,void (*)(  ) ){
                        return g_libc.pthread_once_fn (__once_control,__init_routine);
                    }
                    
                     void * pthread_getspecific (pthread_key_t){
                        return g_libc.pthread_getspecific_fn (__key);
                    }
                    
                     int pthread_setspecific (pthread_key_t,void const *){
                        return g_libc.pthread_setspecific_fn (__key,__pointer);
                    }
                    
                     int pthread_key_create (pthread_key_t *,void (*)( void * ) ){
                        return g_libc.pthread_key_create_fn (__key,__destr_function);
                    }
                    
                     int pthread_key_delete (pthread_key_t){
                        return g_libc.pthread_key_delete_fn (__key);
                    }
                    
                     int pthread_mutex_destroy (pthread_mutex_t *){
                        return g_libc.pthread_mutex_destroy_fn (__mutex);
                    }
                    
                     int pthread_mutex_init (pthread_mutex_t *,pthread_mutexattr_t const *){
                        return g_libc.pthread_mutex_init_fn (__mutex,__mutexattr);
                    }
                    
                     int pthread_mutex_lock (pthread_mutex_t *){
                        return g_libc.pthread_mutex_lock_fn (__mutex);
                    }
                    
                     int pthread_mutex_unlock (pthread_mutex_t *){
                        return g_libc.pthread_mutex_unlock_fn (__mutex);
                    }
                    
                     int pthread_mutex_trylock (pthread_mutex_t *){
                        return g_libc.pthread_mutex_trylock_fn (__mutex);
                    }
                    
                     int pthread_mutexattr_init (pthread_mutexattr_t *){
                        return g_libc.pthread_mutexattr_init_fn (__attr);
                    }
                    
                     int pthread_mutexattr_destroy (pthread_mutexattr_t *){
                        return g_libc.pthread_mutexattr_destroy_fn (__attr);
                    }
                    
                     int pthread_mutexattr_settype (pthread_mutexattr_t *,int){
                        return g_libc.pthread_mutexattr_settype_fn (__attr,__kind);
                    }
                    
                     int pthread_cancel (pthread_t){
                        return g_libc.pthread_cancel_fn (__th);
                    }
                    
                     int pthread_kill (pthread_t thread, int sig){
                        return g_libc.pthread_kill_fn (thread, sig);
                    }
                    
                     int pthread_join (pthread_t,void * *){
                        return g_libc.pthread_join_fn (__th,__thread_return);
                    }
                    
                     int pthread_detach (pthread_t){
                        return g_libc.pthread_detach_fn (__th);
                    }
                    
                     int pthread_cond_destroy (pthread_cond_t *){
                        return g_libc.pthread_cond_destroy_fn (__cond);
                    }
                    
                     int pthread_cond_init (__restrict__ ::pthread_cond_t *,__restrict__ ::pthread_condattr_t const *){
                        return g_libc.pthread_cond_init_fn (__cond,__cond_attr);
                    }
                    
                     int pthread_cond_broadcast (pthread_cond_t *){
                        return g_libc.pthread_cond_broadcast_fn (__cond);
                    }
                    
                     int pthread_cond_signal (pthread_cond_t *){
                        return g_libc.pthread_cond_signal_fn (__cond);
                    }
                    
                     int pthread_cond_timedwait (__restrict__ ::pthread_cond_t *,__restrict__ ::pthread_mutex_t *,__restrict__ ::timespec const *){
                        return g_libc.pthread_cond_timedwait_fn (__cond,__mutex,__abstime);
                    }
                    
                     int pthread_cond_wait (__restrict__ ::pthread_cond_t *,__restrict__ ::pthread_mutex_t *){
                        return g_libc.pthread_cond_wait_fn (__cond,__mutex);
                    }
                    
                     int pthread_condattr_destroy (pthread_condattr_t *){
                        return g_libc.pthread_condattr_destroy_fn (__attr);
                    }
                    
                     int pthread_condattr_init (pthread_condattr_t *){
                        return g_libc.pthread_condattr_init_fn (__attr);
                    }
                    
                     int pthread_rwlock_init (__restrict__ ::pthread_rwlock_t *,__restrict__ ::pthread_rwlockattr_t const *){
                        return g_libc.pthread_rwlock_init_fn (__rwlock,__attr);
                    }
                    
                     int pthread_rwlock_unlock (pthread_rwlock_t *){
                        return g_libc.pthread_rwlock_unlock_fn (__rwlock);
                    }
                    
                     int pthread_rwlock_wrlock (pthread_rwlock_t *){
                        return g_libc.pthread_rwlock_wrlock_fn (__rwlock);
                    }
                    
                     int pthread_rwlock_rdlock (pthread_rwlock_t *){
                        return g_libc.pthread_rwlock_rdlock_fn (__rwlock);
                    }
                    
                     int pthread_rwlock_destroy (pthread_rwlock_t *){
                        return g_libc.pthread_rwlock_destroy_fn (__rwlock);
                    }
                    
                     int pthread_setcancelstate (int,int *){
                        return g_libc.pthread_setcancelstate_fn (__state,__oldstate);
                    }
                    
                     int pthread_sigmask (int,__restrict__ ::__sigset_t const *,__restrict__ ::__sigset_t *){
                        return g_libc.pthread_sigmask_fn (__how,__newmask,__oldmask);
                    }
                    
                     int pthread_equal (pthread_t,pthread_t){
                        return g_libc.pthread_equal_fn (__thread1,__thread2);
                    }
                    
                     int pthread_spin_init (pthread_spinlock_t *,int){
                        return g_libc.pthread_spin_init_fn (__lock,__pshared);
                    }
                    
                     int pthread_spin_lock (pthread_spinlock_t *){
                        return g_libc.pthread_spin_lock_fn (__lock);
                    }
                    
                     int pthread_spin_unlock (pthread_spinlock_t *){
                        return g_libc.pthread_spin_unlock_fn (__lock);
                    }
                    
                     int pthread_spin_destroy (pthread_spinlock_t *){
                        return g_libc.pthread_spin_destroy_fn (__lock);
                    }
                    
                     int sem_init (sem_t *,int,unsigned int){
                        return g_libc.sem_init_fn (__sem,__pshared,__value);
                    }
                    
                     int sem_destroy (sem_t *){
                        return g_libc.sem_destroy_fn (__sem);
                    }
                    
                     int sem_post (sem_t *){
                        return g_libc.sem_post_fn (__sem);
                    }
                    
                     int sem_wait (sem_t *){
                        return g_libc.sem_wait_fn (__sem);
                    }
                    
                     int sem_timedwait (__restrict__ ::sem_t *,__restrict__ ::timespec const *){
                        return g_libc.sem_timedwait_fn (__sem,__abstime);
                    }
                    
                     int sem_trywait (sem_t *){
                        return g_libc.sem_trywait_fn (__sem);
                    }
                    
                     int sem_getvalue (__restrict__ ::sem_t *,__restrict__ int *){
                        return g_libc.sem_getvalue_fn (__sem,__sval);
                    }
                    
                     hostent * gethostbyname (char const *){
                        return g_libc.gethostbyname_fn (__name);
                    }
                    
                     hostent * gethostbyname2 (char const *,int){
                        return g_libc.gethostbyname2_fn (__name,__af);
                    }
                    
                     int getaddrinfo (__restrict__ char const *,__restrict__ char const *,__restrict__ ::addrinfo const *,__restrict__ ::addrinfo * *){
                        return g_libc.getaddrinfo_fn (__name,__service,__req,__pai);
                    }
                    
                     void freeaddrinfo (addrinfo *){
                         g_libc.freeaddrinfo_fn (__ai);
                    }
                    
                     char const * gai_strerror (int){
                        return g_libc.gai_strerror_fn (__ecode);
                    }
                    
                     int getifaddrs (ifaddrs * *){
                        return g_libc.getifaddrs_fn (__ifap);
                    }
                    
                     void freeifaddrs (ifaddrs *){
                         g_libc.freeifaddrs_fn (__ifa);
                    }
                    
                     hostent * gethostent (){
                        return g_libc.gethostent_fn ();
                    }
                    
                     void sethostent (int){
                         g_libc.sethostent_fn (__stay_open);
                    }
                    
                     void endhostent (){
                         g_libc.endhostent_fn ();
                    }
                    
                     void herror (char const *){
                         g_libc.herror_fn (__str);
                    }
                    
                     char const * hstrerror (int){
                        return g_libc.hstrerror_fn (__err_num);
                    }
                    
                     protoent * getprotoent (){
                        return g_libc.getprotoent_fn ();
                    }
                    
                     protoent * getprotobyname (char const *){
                        return g_libc.getprotobyname_fn (__name);
                    }
                    
                     protoent * getprotobynumber (int){
                        return g_libc.getprotobynumber_fn (__proto);
                    }
                    
                     void setprotoent (int){
                         g_libc.setprotoent_fn (__stay_open);
                    }
                    
                     void endprotoent (){
                         g_libc.endprotoent_fn ();
                    }
                    
                     servent * getservent (){
                        return g_libc.getservent_fn ();
                    }
                    
                     servent * getservbyname (char const *,char const *){
                        return g_libc.getservbyname_fn (__name,__proto);
                    }
                    
                     servent * getservbyport (int,char const *){
                        return g_libc.getservbyport_fn (__port,__proto);
                    }
                    
                     void setservent (int){
                         g_libc.setservent_fn (__stay_open);
                    }
                    
                     void endservent (){
                         g_libc.endservent_fn ();
                    }
                    
                     int toupper (int){
                        return g_libc.toupper_fn (__c);
                    }
                    
                     int tolower (int){
                        return g_libc.tolower_fn (__c);
                    }
                    
                     int isdigit (int){
                        return g_libc.isdigit_fn (arg0);
                    }
                    
                     int isxdigit (int){
                        return g_libc.isxdigit_fn (arg0);
                    }
                    
                     int isalnum (int){
                        return g_libc.isalnum_fn (arg0);
                    }
                    
                     int timerfd_create (clockid_t,int){
                        return g_libc.timerfd_create_fn (__clock_id,__flags);
                    }
                    
                     int timerfd_settime (int,int,itimerspec const *,itimerspec *){
                        return g_libc.timerfd_settime_fn (__ufd,__flags,__utmr,__otmr);
                    }
                    
                     int timerfd_gettime (int,itimerspec *){
                        return g_libc.timerfd_gettime_fn (__ufd,__otmr);
                    }
                    
                     unsigned int if_nametoindex (char const *){
                        return g_libc.if_nametoindex_fn (__ifname);
                    }
                    
                     char * if_indextoname (unsigned int,char *){
                        return g_libc.if_indextoname_fn (__ifindex,__ifname);
                    }
                    
                     DIR * opendir (char const *){
                        return g_libc.opendir_fn (__name);
                    }
                    
                     DIR * fdopendir (int){
                        return g_libc.fdopendir_fn (__fd);
                    }
                    
                     dirent * readdir (DIR *){
                        return g_libc.readdir_fn (__dirp);
                    }
                    
                     int readdir_r (__restrict__ ::DIR *,__restrict__ ::dirent *,__restrict__ ::dirent * *){
                        return g_libc.readdir_r_fn (__dirp,__entry,__result);
                    }
                    
                     int closedir (DIR *){
                        return g_libc.closedir_fn (__dirp);
                    }
                    
                     int dirfd (DIR *){
                        return g_libc.dirfd_fn (__dirp);
                    }
                    
                     void rewinddir (DIR *){
                         g_libc.rewinddir_fn (__dirp);
                    }
                    
                     int scandir (__restrict__ char const *,__restrict__ ::dirent * * *,int (*)( ::dirent const * ) ,int (*)( ::dirent const * *,::dirent const * * ) ){
                        return g_libc.scandir_fn (__dir,__namelist,__selector,__cmp);
                    }
                    
                     int alphasort (dirent const * *,dirent const * *){
                        return g_libc.alphasort_fn (__e1,__e2);
                    }
                    
                     int alphasort64 (dirent64 const * *,dirent64 const * *){
                        return g_libc.alphasort64_fn (__e1,__e2);
                    }
                    
                     int versionsort (dirent const * *,dirent const * *){
                        return g_libc.versionsort_fn (__e1,__e2);
                    }
                    
                     int uname (struct utsname *__name){
                        return g_libc.uname_fn (__name);
                    }
                    
                     pid_t wait (void *stat_loc){
                        return g_libc.wait_fn (stat_loc);
                    }
                    
                     __pid_t waitpid (__pid_t,int *,int){
                        return g_libc.waitpid_fn (__pid,__stat_loc,__options);
                    }
                    
                     char * dirname (char *){
                        return g_libc.dirname_fn (__path);
                    }
                    
                     char * __xpg_basename (char *){
                        return g_libc.__xpg_basename_fn (__path);
                    }
                    
                     group * getgrnam (char const *){
                        return g_libc.getgrnam_fn (__name);
                    }
                    
                     int getrusage (__rusage_who_t,rusage *){
                        return g_libc.getrusage_fn (__who,__usage);
                    }
                    
                     int getrlimit (__rlimit_resource_t,rlimit *){
                        return g_libc.getrlimit_fn (__resource,__rlimits);
                    }
                    
                     int setrlimit (__rlimit_resource_t,rlimit const *){
                        return g_libc.setrlimit_fn (__resource,__rlimits);
                    }
                    
                     void openlog (char const *,int,int){
                         g_libc.openlog_fn (__ident,__option,__facility);
                    }
                    
                     void closelog (){
                         g_libc.closelog_fn ();
                    }
                    
                     int setlogmask (int){
                        return g_libc.setlogmask_fn (__mask);
                    }
                    
                     void syslog (int,char const *,...){
                         g_libc.syslog_fn (__pri,__fmt,);
                    }
                    
                     void vsyslog (int,char const *,va_list){
                         g_libc.vsyslog_fn (__pri,__fmt,__ap);
                    }
                    
                     int _setjmp (__jmp_buf_tag *){
                        return g_libc._setjmp_fn (__env);
                    }
                    
                     int __sigsetjmp (__jmp_buf_tag *,int){
                        return g_libc.__sigsetjmp_fn (__env,__savemask);
                    }
                    
                     void siglongjmp (__jmp_buf_tag *,int){
                         g_libc.siglongjmp_fn (__env,__val);
                    }
                    
                     char * bindtextdomain (char const *,char const *){
                        return g_libc.bindtextdomain_fn (__domainname,__dirname);
                    }
                    
                     char * textdomain (char const *){
                        return g_libc.textdomain_fn (__domainname);
                    }
                    
                     char * gettext (char const *){
                        return g_libc.gettext_fn (__msgid);
                    }
                    
                     nl_catd catopen (char const *,int){
                        return g_libc.catopen_fn (__cat_name,__flag);
                    }
                    
                     char * catgets (nl_catd,int,int,char const *){
                        return g_libc.catgets_fn (__catalog,__set,__number,__string);
                    }
                    
                     passwd * getpwnam (char const *){
                        return g_libc.getpwnam_fn (__name);
                    }
                    
                     passwd * getpwuid (__uid_t){
                        return g_libc.getpwuid_fn (__uid);
                    }
                    
                     void endpwent (){
                         g_libc.endpwent_fn ();
                    }
                    
                     intmax_t strtoimax (__restrict__ char const *,__restrict__ char * *,int){
                        return g_libc.strtoimax_fn (__nptr,__endptr,__base);
                    }
                    
                     uintmax_t strtoumax (__restrict__ char const *,__restrict__ char * *,int){
                        return g_libc.strtoumax_fn (__nptr,__endptr,__base);
                    }
                    
                     ether_addr * ether_aton_r (char const *,ether_addr *){
                        return g_libc.ether_aton_r_fn (__asc,__addr);
                    }
                    
                     ether_addr * ether_aton (char const *){
                        return g_libc.ether_aton_fn (__asc);
                    }
                    
                     void * tsearch (void const *,void * *,__compar_fn_t){
                        return g_libc.tsearch_fn (__key,__rootp,__compar);
                    }
                    
                     void * tfind (void const *,void * const *,__compar_fn_t){
                        return g_libc.tfind_fn (__key,__rootp,__compar);
                    }
                    
                     void * tdelete (__restrict__ void const *,__restrict__ void * *,__compar_fn_t){
                        return g_libc.tdelete_fn (__key,__rootp,__compar);
                    }
                    
                     void twalk (void const *,__action_fn_t){
                         g_libc.twalk_fn (__root,__action);
                    }
                    
                     void tdestroy (void *,__free_fn_t){
                         g_libc.tdestroy_fn (__root,__freefct);
                    }
                    
                     int fnmatch (char const *,char const *,int){
                        return g_libc.fnmatch_fn (__pattern,__name,__flags);
                    }
                    
                     char * nl_langinfo (nl_item){
                        return g_libc.nl_langinfo_fn (__item);
                    }
                    
                     int fstatfs (int,statfs *){
                        return g_libc.fstatfs_fn (__fildes,__buf);
                    }
                    
                     int fstatfs64 (int,statfs64 *){
                        return g_libc.fstatfs64_fn (__fildes,__buf);
                    }
                    
                     void statfs::statfs (){
                         g_libc.statfs::statfs_fn ();
                    }
                    
                     void statfs64::statfs64 (){
                         g_libc.statfs64::statfs64_fn ();
                    }
                    
                     void statvfs::statvfs (){
                         g_libc.statvfs::statvfs_fn ();
                    }
                    
                     int fstatvfs (int,statvfs *){
                        return g_libc.fstatvfs_fn (__fildes,__buf);
                    }
                    
                     int tcgetattr (int,termios *){
                        return g_libc.tcgetattr_fn (__fd,__termios_p);
                    }
                    
                     int tcsetattr (int,int,termios const *){
                        return g_libc.tcsetattr_fn (__fd,__optional_actions,__termios_p);
                    }
                    
                     short unsigned int const * * __ctype_b_loc (){
                        return g_libc.__ctype_b_loc_fn ();
                    }
                    
                     wctype_t wctype_l (char const *,__locale_t){
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
                    
                     int creat (char const *,mode_t){
                        return g_libc.creat_fn (__file,__mode);
                    }
                    
                     long int lrintl (long double){
                        return g_libc.lrintl_fn (__x);
                    }
                    
                     long long int llrintl (long double){
                        return g_libc.llrintl_fn (__x);
                    }
                    
                     double ceil (double){
                        return g_libc.ceil_fn (__x);
                    }
                    
                     double floor (double){
                        return g_libc.floor_fn (__x);
                    }
                    
                     void * dlopen (char const *,int){
                        return g_libc.dlopen_fn (__file,__mode);
                    }
                    
                     void * dlsym (__restrict__ void *,__restrict__ char const *){
                        return g_libc.dlsym_fn (__handle,__name);
                    }
                    
                     int dl_iterate_phdr (int (*)( ::dl_phdr_info *,::size_t,void * ) ,void *){
                        return g_libc.dl_iterate_phdr_fn (__callback,__data);
                    }
                    