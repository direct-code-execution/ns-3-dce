#ifndef SIMU_NETDB_H
#define SIMU_NETDB_H

#include <netdb.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(struct hostent * , gethostbyname, const char *name);
DCE(struct hostent * , gethostbyname2, const char *name, int af);
DCE(int , getaddrinfo, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
DCE(void , freeaddrinfo, struct addrinfo *res);
DCE(const char * , gai_strerror, int errcode);
DCE(int , getnameinfo, const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);

DCE(void , herror, const char *string);
DCE(int , getifaddrs, struct ifaddrs **ifap);

NATIVE (freeifaddrs, struct ifaddrs *__ifa)
NATIVE (gethostent) // void
NATIVE (sethostent, int __stay_open)
NATIVE (endhostent) //void
NATIVE (hstrerror, int __err_num) // this could be replaced by DCE call
NATIVE (getprotoent) //void
NATIVE (getprotobyname, const char *__name)
NATIVE (getprotobynumber, int __proto)
NATIVE (setprotoent, int __stay_open)
NATIVE (endprotoent)
NATIVE (getservent) // void
NATIVE (getservbyname, const char *__name, const char *__proto)
NATIVE (getservbyport, int __port, const char *__proto)
NATIVE (setservent, int __stay_open)
NATIVE (endservent) // void
#ifdef __cplusplus
}
#endif

#endif /* SIMU_NETDB_H */
