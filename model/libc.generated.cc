
                     void __cxa_finalize (void *d) {
                         g_libc.__cxa_finalize_fn (d);
                    }
                    
                     int __cxa_atexit (void (*func)(void *), void *arg, void *d) {
                        return g_libc.__cxa_atexit_fn (func, arg, d);
                    }
                    
                     int atexit (void (*)(  ) ) {
                        return g_libc.atexit_fn (__func);
                    }
                    
                     long int random () {
                        return g_libc.random_fn ();
                    }
                    
                     void srandom (unsigned int __seed) {
                         g_libc.srandom_fn (__seed);
                    }
                    
                     int rand () {
                        return g_libc.rand_fn ();
                    }
                    
                     void srand (unsigned int __seed) {
                         g_libc.srand_fn (__seed);
                    }
                    
                     double drand48 () {
                        return g_libc.drand48_fn ();
                    }
                    
                     double erand48 (short unsigned int * __xsubi) {
                        return g_libc.erand48_fn (__xsubi);
                    }
                    
                     long int lrand48 () {
                        return g_libc.lrand48_fn ();
                    }
                    
                     long int nrand48 (short unsigned int * __xsubi) {
                        return g_libc.nrand48_fn (__xsubi);
                    }
                    
                     long int mrand48 () {
                        return g_libc.mrand48_fn ();
                    }
                    
                     long int jrand48 (short unsigned int * __xsubi) {
                        return g_libc.jrand48_fn (__xsubi);
                    }
                    
                     void srand48 (long int __seedval) {
                         g_libc.srand48_fn (__seedval);
                    }
                    
                     short unsigned int * seed48 (short unsigned int * __seed16v) {
                        return g_libc.seed48_fn (__seed16v);
                    }
                    
                     void lcong48 (short unsigned int * __param) {
                         g_libc.lcong48_fn (__param);
                    }
                    
                     int drand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ double * __result) {
                        return g_libc.drand48_r_fn (__buffer,__result);
                    }
                    
                     int erand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ double * __result) {
                        return g_libc.erand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int lrand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) {
                        return g_libc.lrand48_r_fn (__buffer,__result);
                    }
                    
                     int nrand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) {
                        return g_libc.nrand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int mrand48_r (__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) {
                        return g_libc.mrand48_r_fn (__buffer,__result);
                    }
                    
                     int jrand48_r (short unsigned int * __xsubi,__restrict__ ::drand48_data * __buffer,__restrict__ long int * __result) {
                        return g_libc.jrand48_r_fn (__xsubi,__buffer,__result);
                    }
                    
                     int srand48_r (long int __seedval,drand48_data * __buffer) {
                        return g_libc.srand48_r_fn (__seedval,__buffer);
                    }
                    
                     int seed48_r (short unsigned int * __seed16v,drand48_data * __buffer) {
                        return g_libc.seed48_r_fn (__seed16v,__buffer);
                    }
                    
                     int lcong48_r (short unsigned int * __param,drand48_data * __buffer) {
                        return g_libc.lcong48_r_fn (__param,__buffer);
                    }
                    
                     void * calloc (size_t __nmemb,size_t __size) {
                        return g_libc.calloc_fn (__nmemb,__size);
                    }
                    
                     void * malloc (size_t __size) {
                        return g_libc.malloc_fn (__size);
                    }
                    
                     void free (void * __ptr) {
                         g_libc.free_fn (__ptr);
                    }
                    
                     void * realloc (void * __ptr,size_t __size) {
                        return g_libc.realloc_fn (__ptr,__size);
                    }
                    
                     int atoi (char const * __nptr) {
                        return g_libc.atoi_fn (__nptr);
                    }
                    
                     long int atol (char const * __nptr) {
                        return g_libc.atol_fn (__nptr);
                    }
                    
                     long long int atoll (char const * __nptr) {
                        return g_libc.atoll_fn (__nptr);
                    }
                    
                     double atof (char const * __nptr) {
                        return g_libc.atof_fn (__nptr);
                    }
                    
                     long int strtol (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) {
                        return g_libc.strtol_fn (__nptr,__endptr,__base);
                    }
                    
                     long long int strtoll (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) {
                        return g_libc.strtoll_fn (__nptr,__endptr,__base);
                    }
                    
                     long unsigned int strtoul (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) {
                        return g_libc.strtoul_fn (__nptr,__endptr,__base);
                    }
                    
                     long long unsigned int strtoull (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) {
                        return g_libc.strtoull_fn (__nptr,__endptr,__base);
                    }
                    
                     double strtod (__restrict__ char const * __nptr,__restrict__ char * * __endptr) {
                        return g_libc.strtod_fn (__nptr,__endptr);
                    }
                    
                     char * getenv (char const * __name) {
                        return g_libc.getenv_fn (__name);
                    }
                    
                     int putenv (char * __string) {
                        return g_libc.putenv_fn (__string);
                    }
                    
                     int setenv (char const * __name,char const * __value,int __replace) {
                        return g_libc.setenv_fn (__name,__value,__replace);
                    }
                    
                     int unsetenv (char const * __name) {
                        return g_libc.unsetenv_fn (__name);
                    }
                    
                     int clearenv () {
                        return g_libc.clearenv_fn ();
                    }
                    
                     void qsort (void * __base,size_t __nmemb,size_t __size,__compar_fn_t __compar) {
                         g_libc.qsort_fn (__base,__nmemb,__size,__compar);
                    }
                    
                     void abort (void) {
                         g_libc.abort_fn ();
                    }
                    
                     int mkstemp (char * __template) {
                        return g_libc.mkstemp_fn (__template);
                    }
                    
                     FILE * tmpfile () {
                        return g_libc.tmpfile_fn ();
                    }
                    
                     int rename (char const * __old,char const * __new) {
                        return g_libc.rename_fn (__old,__new);
                    }
                    
                     void bzero (void * __s,size_t __n) {
                         g_libc.bzero_fn (__s,__n);
                    }
                    
                     char * strerror (int __errnum) {
                        return g_libc.strerror_fn (__errnum);
                    }
                    
                     char * strerror_r (int __errnum,char * __buf,size_t __buflen) {
                        return g_libc.strerror_r_fn (__errnum,__buf,__buflen);
                    }
                    
                     int strcoll (char const * __s1,char const * __s2) {
                        return g_libc.strcoll_fn (__s1,__s2);
                    }
                    
                     void * memset (void * __s,int __c,size_t __n) {
                        return g_libc.memset_fn (__s,__c,__n);
                    }
                    
                     void * memcpy (__restrict__ void * __dest,__restrict__ void const * __src,size_t __n) {
                        return g_libc.memcpy_fn (__dest,__src,__n);
                    }
                    
                     void bcopy (void const * __src,void * __dest,size_t __n) {
                         g_libc.bcopy_fn (__src,__dest,__n);
                    }
                    
                     int memcmp (void const * __s1,void const * __s2,size_t __n) {
                        return g_libc.memcmp_fn (__s1,__s2,__n);
                    }
                    
                     void * memmove (void * __dest,void const * __src,size_t __n) {
                        return g_libc.memmove_fn (__dest,__src,__n);
                    }
                    
                     char * strcpy (__restrict__ char * __dest,__restrict__ char const * __src) {
                        return g_libc.strcpy_fn (__dest,__src);
                    }
                    
                     char * strncpy (__restrict__ char * __dest,__restrict__ char const * __src,size_t __n) {
                        return g_libc.strncpy_fn (__dest,__src,__n);
                    }
                    
                     char * strcat (__restrict__ char * __dest,__restrict__ char const * __src) {
                        return g_libc.strcat_fn (__dest,__src);
                    }
                    
                     char * strncat (__restrict__ char * __dest,__restrict__ char const * __src,size_t __n) {
                        return g_libc.strncat_fn (__dest,__src,__n);
                    }
                    
                     int strcmp (char const * __s1,char const * __s2) {
                        return g_libc.strcmp_fn (__s1,__s2);
                    }
                    
                     int strncmp (char const * __s1,char const * __s2,size_t __n) {
                        return g_libc.strncmp_fn (__s1,__s2,__n);
                    }
                    
                     size_t strlen (char const * __s) {
                        return g_libc.strlen_fn (__s);
                    }
                    
                     size_t strnlen (char const * __string,size_t __maxlen) {
                        return g_libc.strnlen_fn (__string,__maxlen);
                    }
                    
                     size_t strcspn (char const * __s,char const * __reject) {
                        return g_libc.strcspn_fn (__s,__reject);
                    }
                    
                     size_t strspn (char const * __s,char const * __accept) {
                        return g_libc.strspn_fn (__s,__accept);
                    }
                    
                     int strcasecmp (char const * __s1,char const * __s2) {
                        return g_libc.strcasecmp_fn (__s1,__s2);
                    }
                    
                     int strncasecmp (char const * __s1,char const * __s2,size_t __n) {
                        return g_libc.strncasecmp_fn (__s1,__s2,__n);
                    }
                    