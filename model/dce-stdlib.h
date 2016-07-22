#ifndef SIMU_STDLIB_H
#define SIMU_STDLIB_H

#include <stdlib.h>
#include "dce-stdio.h"

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(long int, strtol , const char *nptr, char **endptr, int base);
DCE(long long int, strtoll , const char *nptr, char **endptr, int base);
DCE(long unsigned, strtoul , const char *nptr, char **endptr, int base);
DCE(long long unsigned, strtoull , const char *nptr, char **endptr, int base);
DCE(double, strtod , const char *nptr, char **endptr);
DCE(void *, calloc , size_t nmemb, size_t size);
DCE(void *, malloc , size_t size);
DCE(void, free , void *ptr);
DCE(void *, realloc , void *ptr, size_t size);
DCE(int, atexit , void (*function)(void));
DCE(char *, getenv , const char *name);
DCE(int, putenv , char *string);
DCE(int, setenv , const char *name, const char *value, int overwrite);
DCE(int, unsetenv , const char *name);
DCE(int, clearenv );
DCE(int, mkstemp , char *temp);
DCE(FILE *, tmpfile);
DCE(int, rename , const char *oldpath, const char *newpath);

// REGRESSION
//#ifdef HAVE___SECURE_GETENV
//NATIVE (__secure_getenv)
//#endif

NATIVE (atoi, const char *__nptr)
NATIVE (atol, const char *__nptr)
NATIVE (atoll, const char *__nptr)
NATIVE (atof, const char *__nptr)

NATIVE (qsort, void *__base, size_t __nmemb, size_t __size, __compar_fn_t __compar)

NATIVE (drand48_r, struct drand48_data *__restrict __buffer, double *__restrict __result)
NATIVE (erand48_r, unsigned short int __xsubi[3], struct drand48_data *__restrict __buffer, double *__restrict __result)
NATIVE (lrand48_r, struct drand48_data *__restrict __buffer, long int *__restrict __result)
NATIVE (nrand48_r,unsigned short int __xsubi[3], struct drand48_data *__restrict __buffer, long int *__restrict __result)
NATIVE (mrand48_r, struct drand48_data *__restrict __buffer,long int *__restrict __result)
NATIVE (jrand48_r, unsigned short int __xsubi[3], struct drand48_data *__restrict __buffer, long int *__restrict __result)
NATIVE (srand48_r, long int __seedval, struct drand48_data *__buffer)
NATIVE (seed48_r, unsigned short int __seed16v[3], struct drand48_data *__buffer)
NATIVE (lcong48_r, unsigned short int __param[7], struct drand48_data *__buffer)

NATIVE (__ctype_get_mb_cur_max) // void

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDLIB_H */
