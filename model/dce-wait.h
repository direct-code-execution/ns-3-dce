#ifndef SIMU_WAIT_H
#define SIMU_WAIT_H
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

pid_t dce_wait (void *status);
pid_t dce_waitpid (pid_t pid, int *status, int options);


#ifdef __cplusplus
}
#endif


#endif /* SIMU_WAIT_H */
