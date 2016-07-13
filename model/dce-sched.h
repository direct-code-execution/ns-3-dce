#ifndef SIMU_SCHED_H
#define SIMU_SCHED_H

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int, sched_yield, (void));

#ifdef __cplusplus
}
#endif

#endif /* SIMU_SCHED_H */
