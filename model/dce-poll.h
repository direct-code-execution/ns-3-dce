#ifndef SIMU_POLL_H
#define SIMU_POLL_H

#include <poll.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_poll (struct pollfd *fds, nfds_t nfds, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_POLL_H */
