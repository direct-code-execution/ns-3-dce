#ifndef SIMU_SEMAPHORE_H
#define SIMU_SEMAPHORE_H

#include <semaphore.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int , sem_init, sem_t *sem, int pshared, unsigned int value);
DCE(int , sem_destroy, sem_t *sem);
DCE(int , sem_post, sem_t *sem);
DCE(int , sem_wait, sem_t *sem);
DCE(int , sem_trywait, sem_t *sem);
DCE(int , sem_timedwait, sem_t *sem, const struct timespec *abs_timeout);
DCE(int , sem_getvalue, sem_t *sem, int *sval);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_SEMAPHORE */
