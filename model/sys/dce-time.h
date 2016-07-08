#ifndef SYS_DCE_TIME_H
#define SYS_DCE_TIME_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

DCE(int, gettimeofday , (struct timeval *tv, struct timezone *tz));
DCE(int, getitimer , (int which, struct itimerval *value));
DCE(int, setitimer , (int which, const struct itimerval *value,
                   struct itimerval *ovalue));
DCE(int, nanosleep , (const struct timespec *req, struct timespec *rem));


#ifdef __cplusplus
}
#endif

#endif /* SYS_DCE_TIME_H */
