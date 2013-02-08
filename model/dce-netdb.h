#ifndef SIMU_NETDB_H
#define SIMU_NETDB_H

#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hostent * dce_gethostbyname (const char *name);
struct hostent * dce_gethostbyname2 (const char *name, int af);
int dce_getaddrinfo (const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **res);
void dce_freeaddrinfo (struct addrinfo *res);
const char * dce_gai_strerror (int errcode);
int dce_getnameinfo (const struct sockaddr *sa, socklen_t salen, char *host,
                     socklen_t hostlen, char *serv, socklen_t servlen, unsigned int flags);

void dce_herror (const char *string);
int dce_getifaddrs (struct ifaddrs **ifap);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_NETDB_H */
