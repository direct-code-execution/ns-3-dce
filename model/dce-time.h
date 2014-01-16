#ifndef SIMU_TIME_H
#define SIMU_TIME_H

#include "sys/dce-time.h"
#include <time.h>
#include <utime.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

time_t dce_time (time_t *t);
struct tm * dce_gmtime (const time_t *timep);
struct tm * dce_localtime (const time_t *timep);
char * dce_ctime (const time_t *timep);
char * dce_asctime (const struct tm *tm);
int dce_clock_gettime (clockid_t which_clock, struct timespec *tp);
int dce_sysinfo (struct sysinfo *info);
void dce_tzset (void);
int dce_clock_getres (clockid_t c, struct timespec *r);
int dce_clock_gettime (clockid_t c, struct timespec *t);
int dce_utime (const char *filename, const struct utimbuf *times);

int dce_timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
int dce_timer_settime(int timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
int dce_timer_gettime(int timerid, struct itimerspec *cur_value);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_TIME_H */
