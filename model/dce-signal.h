#ifndef SIMU_SIGNAL_H
#define SIMU_SIGNAL_H

#include "dce-clock.h"
#include "dce-pthread.h"
#include <signal.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

sighandler_t dce_signal (int signum, sighandler_t handler);
int dce_sigaction (int signum, const struct sigaction *act,
                   struct sigaction *oldact);
int dce_kill (pid_t pid, int sig);
int dce_pthread_kill (pthread_t thread, int sig);
void dce_abort ();
void dce___assert_fail (const char *__assertion, const char *__file,
                        unsigned int __line, const char *__function);
void dce___stack_chk_fail (void);
int dce_sigprocmask (int how, const sigset_t *set, sigset_t *oldset);
int dce_sigwait (const sigset_t *set, int *sig);

#ifdef __cplusplus
}
#endif


#endif /* SIMU_SIGNAL_H */
