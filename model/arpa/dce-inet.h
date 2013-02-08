#ifndef DCE_INET_H
#define DCE_INET_H

#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

const char * dce_inet_ntop (int af, const void *src,
                            char *dst, socklen_t cnt);

#ifdef __cplusplus
}
#endif

#endif /* DCE_INET_H */
