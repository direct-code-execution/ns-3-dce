#ifndef SIMU_SELECT_H
#define SIMU_SELECT_H

#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

int simu_select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_SELECT_H */
