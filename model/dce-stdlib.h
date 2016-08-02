
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_STDLIB_H
#define DCE_HEADER_STDLIB_H
// TODO add extern "C" ?
#include <stdlib.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 int dce_atexit (void (*)(  ) ) noexcept;

 long int dce_random () noexcept;

 void dce_srandom (unsigned int __seed) noexcept;

 int dce_rand () noexcept;

 void dce_srand (unsigned int __seed) noexcept;

 double dce_drand48 () noexcept;

 double dce_erand48 (short unsigned int * __xsubi) noexcept;

 long int dce_lrand48 () noexcept;

 long int dce_nrand48 (short unsigned int * __xsubi) noexcept;

 long int dce_mrand48 () noexcept;

 long int dce_jrand48 (short unsigned int * __xsubi) noexcept;

 void dce_srand48 (long int __seedval) noexcept;

 short unsigned int * dce_seed48 (short unsigned int * __seed16v) noexcept;

 void dce_lcong48 (short unsigned int * __param) noexcept;










 void * dce_calloc (size_t __nmemb,size_t __size) noexcept;

 void * dce_malloc (size_t __size) noexcept;

 void dce_free (void * __ptr) noexcept;

 void * dce_realloc (void * __ptr,size_t __size) noexcept;





 long int dce_strtol (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept;

 long long int dce_strtoll (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept;

 long unsigned int dce_strtoul (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept;

 long long unsigned int dce_strtoull (__restrict__ char const * __nptr,__restrict__ char * * __endptr,int __base) noexcept;

 double dce_strtod (__restrict__ char const * __nptr,__restrict__ char * * __endptr) noexcept;

 char * dce_getenv (char const * __name) noexcept;

 int dce_putenv (char * __string) noexcept;

 int dce_setenv (char const * __name,char const * __value,int __replace) noexcept;

 int dce_unsetenv (char const * __name) noexcept;

 int dce_clearenv () noexcept;


 void dce_abort () noexcept __attribute__ ((__noreturn__));

 int dce_mkstemp (char * __template) ;

 void dce_exit (int __status) noexcept __attribute__ ((__noreturn__));



#ifdef __cplusplus
}
#endif
#endif
