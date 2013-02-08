#ifndef SIMU_PTHREAD_H
#define SIMU_PTHREAD_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_pthread_create (pthread_t * thread,
                        const pthread_attr_t * attr,
                        void *(*start_routine)(void*),
                        void *arg);
void dce_pthread_exit (void *arg);
int dce_pthread_join (pthread_t thread, void **value_ptr);
int dce_pthread_detach (pthread_t thread);
int dce_pthread_cancel (pthread_t thread);
pthread_t dce_pthread_self (void);

int dce_pthread_mutex_init (pthread_mutex_t *mutex,
                            const pthread_mutexattr_t *attr);
int dce_pthread_mutex_destroy (pthread_mutex_t *mutex);
int dce_pthread_mutex_lock (pthread_mutex_t *mutex);
int dce_pthread_mutex_trylock (pthread_mutex_t *mutex);
int dce_pthread_mutex_unlock (pthread_mutex_t *mutex);
int dce_pthread_mutexattr_init (pthread_mutexattr_t *attr);
int dce_pthread_mutexattr_destroy (pthread_mutexattr_t *attr);
int dce_pthread_mutexattr_settype (pthread_mutexattr_t *attr, int kind);

int dce_pthread_once (pthread_once_t * once_control, void (*init_routine)(void));
void * dce_pthread_getspecific (pthread_key_t key);
int dce_pthread_setspecific (pthread_key_t key, const void *value);
int dce_pthread_key_create (pthread_key_t * key, void (*destructor)(void*));
int dce_pthread_key_delete (pthread_key_t key);

int dce_pthread_cond_destroy (pthread_cond_t *cond);
int dce_pthread_cond_init (pthread_cond_t *cond,
                           const pthread_condattr_t *attr);
int dce_pthread_cond_broadcast (pthread_cond_t *cond);
int dce_pthread_cond_signal (pthread_cond_t *cond);
int dce_pthread_cond_timedwait (pthread_cond_t * cond,
                                pthread_mutex_t * mutex,
                                const struct timespec * abstime);
int dce_pthread_cond_wait (pthread_cond_t * cond,
                           pthread_mutex_t * mutex);
int dce_pthread_condattr_destroy (pthread_condattr_t *attr);
int dce_pthread_condattr_init (pthread_condattr_t *attr);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_PTHREAD_H */
