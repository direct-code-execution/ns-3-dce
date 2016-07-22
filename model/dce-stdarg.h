#ifndef SIMU_STDARG_H
#define SIMU_STDARG_H

#include <stdarg.h>


#include "dce-guard.h"


#ifdef __cplusplus
extern "C" {
#endif


DCE(int, vprintf, const char* format, va_list ap);
NATIVE (vfprintf, FILE *stream, const char *format, va_list ap)
NATIVE (vsprintf, char *str, const char *format, va_list ap)

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STDARG_H */
