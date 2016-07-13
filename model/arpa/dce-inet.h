#ifndef DCE_INET_H
#define DCE_INET_H

#include <arpa/inet.h>
#include "dce-guard.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

DCE(const char *, inet_ntop, (int af, const void *src, char *dst, socklen_t cnt));
NATIVE (htonl)
NATIVE (htons)
NATIVE (ntohl)
NATIVE (ntohs)
NATIVE (lockf)
NATIVE (inet_aton)
NATIVE (inet_addr)
NATIVE (inet_network)
NATIVE (inet_ntoa)
NATIVE (inet_makeaddr)
NATIVE (inet_lnaof)
NATIVE (inet_netof)
//DCE (inet_ntop)
NATIVE (inet_pton)
NATIVE (inet6_opt_find)

//#ifdef __cplusplus
//}
//#endif

#endif /* DCE_INET_H */
