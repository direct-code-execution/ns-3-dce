#ifndef SIMU_TIME_H
#define SIMU_TIME_H

#include "sys/dce-time.h"
#include <time.h>
#include <sys/sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

time_t dce_time (time_t *t);
struct tm *dce_gmtime(const time_t *timep);
struct tm *dce_localtime(const time_t *timep);
char *dce_ctime(const time_t *timep);
char *dce_asctime(const struct tm *tm);
int dce_clock_gettime(clockid_t which_clock, struct timespec *tp);
int dce_sysinfo (struct sysinfo *info);
	
#ifdef __cplusplus
}
#endif

#endif /* SIMU_TIME_H */
