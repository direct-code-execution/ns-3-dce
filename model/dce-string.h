#ifndef SIMU_STRING_H
#define SIMU_STRING_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

char *dce_strdup (const char *s);
char *dce_strndup (const char *s, size_t n);



#ifdef __cplusplus
}
#endif

#endif /* SIMU_STRING_H */
