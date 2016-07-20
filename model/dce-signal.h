#ifndef SIMU_SIGNAL_H
#define SIMU_SIGNAL_H

#include "dce-clock.h"
#include "dce-pthread.h"
#include <signal.h>
#include <stdint.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(sighandler_t , signal, int signum, sighandler_t handler);
DCE(int , sigaction, int signum, const struct sigaction *act, struct sigaction *oldact);
DCE(int , kill, pid_t pid, int sig);
DCE(int , pthread_kill, pthread_t thread, int sig);
DCE(void , abort, void);
DCE(void , __assert_fail, const char *__assertion, const char *__file, unsigned int __line, const char *__function);
DCE(void , __stack_chk_fail, void);
DCE(int , sigprocmask, int how, const sigset_t *set, sigset_t *oldset);
DCE(int , sigwait, const sigset_t *set, int *sig);


NATIVE (sigemptyset)
NATIVE (sigfillset)
NATIVE (sigaddset)
NATIVE (sigdelset)
NATIVE (sigismember)
//exists only for compatibility, we should use strsignal instead
//NATIVE (sys_siglist) // REGRESSION

#ifdef __cplusplus
}
#endif

#endif /* SIMU_SIGNAL_H */
