#ifndef DCE_INET_H
#define DCE_INET_H

#include <arpa/inet.h>
#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(const char *, inet_ntop, int af, const void *src, char *dst, socklen_t cnt);
NATIVE (htonl, uint32_t)
NATIVE (htons, uint16_t)
NATIVE (ntohl, uint32_t)
NATIVE (ntohs, uint16_t)
NATIVE (lockf, int __fd, int __cmd, __off_t )
NATIVE (inet_aton)
NATIVE (inet_addr)
NATIVE (inet_network)
NATIVE (inet_ntoa)
NATIVE (inet_makeaddr)
NATIVE (inet_lnaof)
NATIVE (inet_netof, struct in_addr)
//DCE (inet_ntop)
NATIVE (inet_pton)
NATIVE (inet6_opt_find)

#ifdef __cplusplus
}
#endif

#endif /* DCE_INET_H */
