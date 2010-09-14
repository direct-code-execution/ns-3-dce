#ifndef SYS_SIMU_TIME_H
#define SYS_SIMU_TIME_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

int simu_gettimeofday (struct timeval *tv, struct timezone *tz);
int simu_getitimer(int which, struct itimerval *value);
int simu_setitimer(int which, const struct itimerval *value,
		   struct itimerval *ovalue);
int simu_nanosleep(const struct timespec *req, struct timespec *rem);


#ifdef __cplusplus
}
#endif

#endif /* SYS_SIMU_TIME_H */
