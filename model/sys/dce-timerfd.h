#ifndef DCE_TIMERFD_H
#define DCE_TIMERFD_H

#include <sys/timerfd.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int , timerfd_create, int clockid, int flags);
DCE(int , timerfd_settime, int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);
DCE(int , timerfd_gettime, int fd, struct itimerspec *curr_value);


#ifdef __cplusplus
}
#endif

#endif /* DCE_TIMERFD_H */
