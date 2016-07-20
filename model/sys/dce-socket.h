#ifndef SYS_DCE_SOCKET_H
#define SYS_DCE_SOCKET_H

#include <stdint.h>
#include <sys/socket.h>
#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif


DCE (int, socket , int domain, int type, int protocol);
DCE(int, bind , int fd, const struct sockaddr *my_addr, socklen_t addrlen);
DCE(int, connect , int fd, const struct sockaddr *my_addr, socklen_t addrlen);
DCE(int, listen , int sockfd, int backlog);
DCE(int, accept , int sockfd, struct sockaddr *addr, socklen_t *addrlen);
DCE(int, shutdown , int s, int how);
DCE(ssize_t, recv , int fd, void *buf, size_t count, int flags);
DCE(ssize_t, recvfrom , int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
DCE(ssize_t, recvmsg , int s, struct msghdr *msg, int flags);
DCE(int, setsockopt , int s, int level, int optname, const void *optval, socklen_t optlen);
DCE(int, getsockopt , int s, int level, int optname, void *optval, socklen_t *optlen);
DCE(ssize_t, send , int s, const void *buf, size_t len, int flags);
DCE(ssize_t, sendto , int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
DCE(ssize_t, sendmsg , int s, const struct msghdr *msg, int flags);
DCE(int, getsockname , int s, struct sockaddr *name, socklen_t *namelen);
DCE(int, getpeername , int s, struct sockaddr *name, socklen_t *namelen);
DCE(int, socketpair , int domain, int type, int protocol, int sv[2]);

#ifdef __cplusplus
}
#endif

#endif /* SYS_DCE_SOCKET_H */
