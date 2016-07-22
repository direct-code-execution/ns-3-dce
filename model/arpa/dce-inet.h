#ifndef DCE_INET_H
#define DCE_INET_H

#include <arpa/inet.h>
#include "dce-guard.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

DCE(const char *, inet_ntop, int af, const void *src, char *dst, socklen_t cnt);
NATIVE (htonl, uint32_t)
NATIVE (htons, uint16_t)
NATIVE (ntohl, uint32_t)
NATIVE (ntohs, uint16_t)
NATIVE (lockf, int __fd, int __cmd, __off_t )
NATIVE (inet_aton, const char *__cp, struct in_addr *__inp)
NATIVE (inet_addr, const char *__cp)
NATIVE (inet_network, const char *)
NATIVE (inet_ntoa, struct in_addr __in)
NATIVE (inet_makeaddr, in_addr_t __net, in_addr_t __host)
NATIVE (inet_lnaof, struct in_addr __in)
NATIVE (inet_netof, struct in_addr)
//DCE (inet_ntop)
NATIVE (inet_pton, int __af, const char *__restrict __cp, void *__restrict __buf)
//NATIVE (inet6_opt_find) // REGRESSION

//#ifdef __cplusplus
//}
//#endif

#endif /* DCE_INET_H */
