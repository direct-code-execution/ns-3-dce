#ifndef SIMU_SEMAPHORE_H
#define SIMU_SEMAPHORE_H

#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_sem_init (sem_t *sem, int pshared, unsigned int value);
int dce_sem_destroy (sem_t *sem);
int dce_sem_post (sem_t *sem);
int dce_sem_wait (sem_t *sem);
int dce_sem_trywait (sem_t *sem);
int dce_sem_timedwait (sem_t *sem, const struct timespec *abs_timeout);
int dce_sem_getvalue (sem_t *sem, int *sval);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_SEMAPHORE */
