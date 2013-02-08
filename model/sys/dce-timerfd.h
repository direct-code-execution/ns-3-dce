#ifndef DCE_TIMERFD_H
#define DCE_TIMERFD_H

#include <sys/timerfd.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_timerfd_create (int clockid, int flags);
int dce_timerfd_settime (int fd, int flags,
                         const struct itimerspec *new_value,
                         struct itimerspec *old_value);
int dce_timerfd_gettime (int fd, struct itimerspec *curr_value);


#ifdef __cplusplus
}
#endif

#endif /* DCE_TIMERFD_H */
