#ifndef SIMU_POLL_H
#define SIMU_POLL_H

#include <poll.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int, poll, (struct pollfd *fds, nfds_t nfds, int timeout) );

#ifdef __cplusplus
}
#endif

#endif /* SIMU_POLL_H */
