#ifndef SIMU_INET_H
#define SIMU_INET_H

#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *simu_inet_ntop(int af, const void *src,
			   char *dst, socklen_t cnt);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_INET_H */
