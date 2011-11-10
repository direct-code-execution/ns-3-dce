#ifndef SIMU_STRING_H
#define SIMU_STRING_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

char *dce_strdup (const char *s);
char *dce_strndup (const char *s, size_t n);
char *dce___strcpy_chk (char *__restrict __dest,
						const char *__restrict __src,
						size_t __destlen);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STRING_H */
