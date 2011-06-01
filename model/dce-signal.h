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
int dce_pthread_kill(pthread_t thread, int sig);
int dce_abort ();

#ifdef __cplusplus
}
#endif


#endif /* SIMU_SIGNAL_H */
