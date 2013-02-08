#ifndef SYS_DCE_TIME_H
#define SYS_DCE_TIME_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_gettimeofday (struct timeval *tv, struct timezone *tz);
int dce_getitimer (int which, struct itimerval *value);
int dce_setitimer (int which, const struct itimerval *value,
                   struct itimerval *ovalue);
int dce_nanosleep (const struct timespec *req, struct timespec *rem);


#ifdef __cplusplus
}
#endif

#endif /* SYS_DCE_TIME_H */
