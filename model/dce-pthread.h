#ifndef SIMU_PTHREAD_H
#define SIMU_PTHREAD_H

#include <pthread.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int , pthread_create, (pthread_t * thread, const pthread_attr_t * attr, void *(*start_routine)(void*), void *arg));
DCE(void , pthread_exit, (void *arg));
DCE(int , pthread_join, (pthread_t thread, void **value_ptr));
DCE(int , pthread_detach, (pthread_t thread));
DCE(int , pthread_cancel, (pthread_t thread));
DCE(pthread_t , pthread_self, (void));

DCE(int , pthread_mutex_init, (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr));
DCE(int , pthread_mutex_destroy, (pthread_mutex_t *mutex));
DCE(int , pthread_mutex_lock, (pthread_mutex_t *mutex));
DCE(int , pthread_mutex_trylock, (pthread_mutex_t *mutex));
DCE(int , pthread_mutex_unlock, (pthread_mutex_t *mutex));
DCE(int , pthread_mutexattr_init, (pthread_mutexattr_t *attr));
DCE(int , pthread_mutexattr_destroy, (pthread_mutexattr_t *attr));
DCE(int , pthread_mutexattr_settype, (pthread_mutexattr_t *attr, int kind));

DCE(int , pthread_once, (pthread_once_t * once_control, void (*init_routine)(void)));
DCE(void * , pthread_getspecific, (pthread_key_t key));
DCE(int , pthread_setspecific, (pthread_key_t key, const void *value));
DCE(int , pthread_key_create, (pthread_key_t * key, void (*destructor)(void*)));
DCE(int , pthread_key_delete, (pthread_key_t key));

DCE(int , pthread_cond_destroy, (pthread_cond_t *cond));
DCE(int , pthread_cond_init, (pthread_cond_t *cond, const pthread_condattr_t *attr));
DCE(int , pthread_cond_broadcast, (pthread_cond_t *cond));
DCE(int , pthread_cond_signal, (pthread_cond_t *cond));
DCE(int , pthread_cond_timedwait, (pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime));
DCE(int , pthread_cond_wait, (pthread_cond_t * cond, pthread_mutex_t * mutex));
DCE(int , pthread_condattr_destroy, (pthread_condattr_t *attr));
DCE(int , pthread_condattr_init, (pthread_condattr_t *attr));

NATIVE (pthread_rwlock_init)
NATIVE (pthread_rwlock_unlock)
NATIVE (pthread_rwlock_wrlock)
NATIVE (pthread_rwlock_rdlock)
NATIVE (pthread_rwlock_destroy)
NATIVE (pthread_setcancelstate)
NATIVE (pthread_sigmask)
NATIVE (pthread_equal)
NATIVE (pthread_spin_init)
NATIVE (pthread_spin_lock)
NATIVE (pthread_spin_unlock)
NATIVE (pthread_spin_destroy)

#ifdef __cplusplus
}
#endif

#endif /* SIMU_PTHREAD_H */
