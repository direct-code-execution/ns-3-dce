#ifndef SIMU_SIGNAL_H
#define SIMU_SIGNAL_H

#include "simu-clock.h"
#include "simu-pthread.h"
#include <signal.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

sighandler_t simu_signal (int signum, sighandler_t handler);
int simu_sigaction (int signum, const struct sigaction *act,
		   struct sigaction *oldact);
int simu_kill (pid_t pid, int sig);
int simu_pthread_kill(pthread_t thread, int sig);

#ifdef __cplusplus
}
#endif


#endif /* SIMU_SIGNAL_H */
