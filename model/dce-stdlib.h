#ifndef SIMU_STDLIB_H
#define SIMU_STDLIB_H

#include <stdlib.h>
#include "dce-stdio.h"

#include "dce-guard.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

DCE(long int, strtol , (const char *nptr, char **endptr, int base));
DCE(long long int, strtoll , (const char *nptr, char **endptr, int base));
DCE(long unsigned, strtoul , (const char *nptr, char **endptr, int base));
DCE(long long unsigned, strtoull , (const char *nptr, char **endptr, int base));
DCE(double, strtod , (const char *nptr, char **endptr));
DCE(void *, calloc , (size_t nmemb, size_t size));
DCE(void *, malloc , (size_t size));
DCE(void, free , (void *ptr));
DCE(void *, realloc , (void *ptr, size_t size));
DCE(int, atexit , (void (*function)(void)));
DCE(char *, getenv , (const char *name));
DCE(int, putenv , (char *string));
DCE(int, setenv , (const char *name, const char *value, int overwrite));
DCE(int, unsetenv , (const char *name));
DCE(int, clearenv , (void));
DCE(int, mkstemp , (char *temp));
DCE(FILE *, tmpfile, (void));
DCE(int, rename , (const char *oldpath, const char *newpath));

NATIVE (drand48_r)
NATIVE (erand48_r)
NATIVE (lrand48_r)
NATIVE (nrand48_r)
NATIVE (mrand48_r)
NATIVE (jrand48_r)
NATIVE (srand48_r)
NATIVE (seed48_r)
NATIVE (lcong48_r)

//#ifdef __cplusplus
//}
//#endif

#endif /* SIMU_STDLIB_H */
