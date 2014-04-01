#ifndef SIMU_STDLIB_H
#define SIMU_STDLIB_H

#include <stdlib.h>
#include "dce-stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

long int dce_strtol (const char *nptr, char **endptr, int base);
long long int dce_strtoll (const char *nptr, char **endptr, int base);
long unsigned int dce_strtoul (const char *nptr, char **endptr, int base);
long long unsigned int dce_strtoull (const char *nptr, char **endptr, int base);
double dce_strtod (const char *nptr, char **endptr);
void * dce_calloc (size_t nmemb, size_t size);
void * dce_malloc (size_t size);
void dce_free (void *ptr);
void * dce_realloc (void *ptr, size_t size);
int dce_atexit (void (*function)(void));
char * dce_getenv (const char *name);
int dce_putenv (char *string);
int dce_setenv (const char *name, const char *value, int overwrite);
int dce_unsetenv (const char *name);
int dce_clearenv (void);
int dce_mkstemp (char *temp);
FILE * dce_tmpfile(void);
int dce_rename (const char *oldpath, const char *newpath);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDLIB_H */
