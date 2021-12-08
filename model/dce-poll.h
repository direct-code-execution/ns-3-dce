#ifndef SIMU_POLL_H
#define SIMU_POLL_H

#include <poll.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_poll (struct pollfd *fds, nfds_t nfds, int timeout);
int dce___poll_chk (struct pollfd *fds, nfds_t nfds, int timeout, size_t fdslen);
int dce_ppoll (struct pollfd *fds, nfds_t nfds, 
  const struct timespec *timeout_ts, const sigset_t *sigmask);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_POLL_H */
