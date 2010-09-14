#ifndef SIMU_NETDB_H
#define SIMU_NETDB_H

#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hostent *simu_gethostbyname(const char *name);
struct hostent *simu_gethostbyname2(const char *name, int af);
int simu_getaddrinfo(const char *node, const char *service,
		     const struct addrinfo *hints,
		     struct addrinfo **res);
void simu_freeaddrinfo(struct addrinfo *res);
const char *simu_gai_strerror(int errcode);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_NETDB_H */
