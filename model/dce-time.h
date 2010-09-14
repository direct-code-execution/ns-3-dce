#ifndef SIMU_TIME_H
#define SIMU_TIME_H

#include "sys/dce-time.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

time_t dce_time (time_t *t);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_TIME_H */
