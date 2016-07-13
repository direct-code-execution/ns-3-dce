#ifndef SIMU_NETDB_H
#define SIMU_NETDB_H

#include <netdb.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(struct hostent * , gethostbyname, (const char *name));
DCE(struct hostent * , gethostbyname2, (const char *name, int af));
DCE(int , getaddrinfo, (const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res));
DCE(void , freeaddrinfo, (struct addrinfo *res));
DCE(const char * , gai_strerror, (int errcode));
DCE(int , getnameinfo, (const struct sockaddr *sa, socklen_t salen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, unsigned int flags));

DCE(void , herror, (const char *string));
DCE(int , getifaddrs, (struct ifaddrs **ifap));

NATIVE (freeifaddrs)
NATIVE (gethostent)
NATIVE (sethostent)
NATIVE (endhostent)
NATIVE (hstrerror) // this could be replaced by DCE call
NATIVE (getprotoent)
NATIVE (getprotobyname)
NATIVE (getprotobynumber)
NATIVE (setprotoent)
NATIVE (endprotoent)
NATIVE (getservent)
NATIVE (getservbyname)
NATIVE (getservbyport)
NATIVE (setservent)
NATIVE (endservent)
#ifdef __cplusplus
}
#endif

#endif /* SIMU_NETDB_H */
