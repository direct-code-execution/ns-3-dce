
#ifndef DCE_SYSLOG_H
#define DCE_SYSLOG_H

#include <stdarg.h>

void
dce_closelog (void);

void
dce_openlog (const char *ident, int logopt, int facility);

int
dce_setlogmask (int maskpri);

void
dce_syslog (int priority, const char *message, ...);

void
dce_vsyslog (int priority, const char *message, va_list args);

#endif // DCE_SYSLOG_H
