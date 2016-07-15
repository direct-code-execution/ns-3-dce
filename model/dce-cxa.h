#ifndef SIMU_CXA_H
#define SIMU_CXA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dce-guard.h"

DCE(int, __cxa_atexit , (void (*func)(void *), void *arg, void *d));
DCE(void, __cxa_finalize , (void *d));

#ifdef __cplusplus
}
#endif

#endif /* SIMU_CXA_H */
