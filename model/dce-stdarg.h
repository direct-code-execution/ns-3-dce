#ifndef SIMU_STDARG_H
#define SIMU_STDARG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_vfprintf (FILE *stream, const char *format, va_list ap);
int dce_vsprintf (char *str, const char *format, va_list ap);
int dce_vsnprintf (char *str, size_t size, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDARG_H */
