#ifndef SIMU_STDARG_H
#define SIMU_STDARG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int simu_vfprintf(FILE *stream, const char *format, va_list ap);
int simu_vsprintf(char *str, const char *format, va_list ap);
int simu_vsnprintf(char *str, size_t size, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDARG_H */
