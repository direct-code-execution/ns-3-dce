#ifndef SIMU_STDLIB_H
#define SIMU_STDLIB_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

long int simu_strtol(const char *nptr, char **endptr, int base);
long long int simu_strtoll(const char *nptr, char **endptr, int base);
long unsigned int simu_strtoul(const char *nptr, char **endptr, int base);
long long unsigned int simu_strtoull(const char *nptr, char **endptr, int base);
double simu_strtod(const char *nptr, char **endptr);
void *simu_calloc(size_t nmemb, size_t size);
void *simu_malloc(size_t size);
void simu_free(void *ptr);
void *simu_realloc(void *ptr, size_t size);
int simu_atexit(void (*function)(void));
char *simu_getenv(const char *name);
int simu_putenv(char *string);
int simu_setenv(const char *name, const char *value, int overwrite);
int simu_unsetenv(const char *name);
int simu_clearenv(void);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDLIB_H */
