
#ifndef DCE_SYSLOG_H
#define DCE_SYSLOG_H

#include <stdarg.h>

#include "dce-guard.h"

DCE(void , closelog);
DCE(void , openlog, const char *ident, int logopt, int facility);
DCE(int , setlogmask, int maskpri);
DCE(void , syslog, int priority, const char *message, ...);
DCE(void , vsyslog, int priority, const char *message, va_list args);

#endif // DCE_SYSLOG_H
