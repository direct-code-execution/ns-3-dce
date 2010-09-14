#ifndef SIMU_TIMERFD_H
#define SIMU_TIMERFD_H

#include <sys/timerfd.h>

#ifdef __cplusplus
extern "C" {
#endif

int simu_timerfd_create(int clockid, int flags);
int simu_timerfd_settime(int fd, int flags,
			 const struct itimerspec *new_value,
			 struct itimerspec *old_value);
int simu_timerfd_gettime(int fd, struct itimerspec *curr_value);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_TIMERFD_H */
