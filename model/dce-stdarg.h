#ifndef SIMU_STDARG_H
#define SIMU_STDARG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_vprintf (const char* format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDARG_H */
