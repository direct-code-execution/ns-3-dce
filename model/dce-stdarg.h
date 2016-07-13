#ifndef SIMU_STDARG_H
#define SIMU_STDARG_H

#include <stdarg.h>

//#ifdef __cplusplus
//extern "C" {
//#endif

#include "dce-guard.h"

DCE(int, vprintf, (const char* format, va_list ap));
NATIVE (vfprintf)
NATIVE (vsprintf)
//#ifdef __cplusplus
//}
//#endif

#endif /* SIMU_STDARG_H */
