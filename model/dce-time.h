#ifndef SIMU_TIME_H
#define SIMU_TIME_H

#include "sys/dce-time.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

time_t dce_time (time_t *t);
struct tm *dce_gmtime(const time_t *timep);
struct tm *dce_localtime(const time_t *timep);
char *dce_ctime(const time_t *timep);
char *dce_asctime(const struct tm *tm);
	
#ifdef __cplusplus
}
#endif

#endif /* SIMU_TIME_H */
