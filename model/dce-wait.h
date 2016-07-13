#ifndef SIMU_WAIT_H
#define SIMU_WAIT_H
#include <sys/types.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(pid_t , wait, (void *status));
DCE(pid_t , waitpid, (pid_t pid, int *status, int options));


#ifdef __cplusplus
}
#endif


#endif /* SIMU_WAIT_H */
