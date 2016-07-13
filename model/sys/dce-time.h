#ifndef SYS_DCE_TIME_H
#define SYS_DCE_TIME_H

#include <sys/time.h>

#include "../dce-guard.h"
//#include "dce-time.i"

DCE(int, gettimeofday , (struct timeval *tv, struct timezone *tz));
DCE(int, getitimer , (int which, struct itimerval *value));
DCE(int, setitimer , (int which, const struct itimerval *value,
                   struct itimerval *ovalue));
DCE(int, nanosleep , (const struct timespec *req, struct timespec *rem));


#endif /* SYS_DCE_TIME_H */
