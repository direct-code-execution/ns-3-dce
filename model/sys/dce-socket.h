#ifndef SYS_DCE_SOCKET_H
#define SYS_DCE_SOCKET_H

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_socket (int domain, int type, int protocol);
int dce_bind (int fd, const struct sockaddr *my_addr, socklen_t addrlen);
int dce_connect (int fd, const struct sockaddr *my_addr, socklen_t addrlen);
int dce_listen (int sockfd, int backlog);
int dce_accept (int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int dce_shutdown (int s, int how);
ssize_t dce_recv (int fd, void *buf, size_t count, int flags);
ssize_t dce_recvfrom (int s, void *buf, size_t len, int flags,
                      struct sockaddr *from, socklen_t *fromlen);
ssize_t dce_recvmsg (int s, struct msghdr *msg, int flags);
int dce_setsockopt (int s, int level, int optname,
                    const void *optval, socklen_t optlen);
int dce_getsockopt (int s, int level, int optname,
                    void *optval, socklen_t *optlen);
ssize_t dce_send (int s, const void *buf, size_t len, int flags);
ssize_t dce_sendto (int s, const void *buf, size_t len, int flags,
                    const struct sockaddr *to, socklen_t tolen);
ssize_t dce_sendmsg (int s, const struct msghdr *msg, int flags);
int dce_getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int dce_getpeername (int s, struct sockaddr *name, socklen_t *namelen);
int dce_socketpair (int domain, int type, int protocol, int sv[2]);

#ifdef __cplusplus
}
#endif

#endif /* SYS_DCE_SOCKET_H */
