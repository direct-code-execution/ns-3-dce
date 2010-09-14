#ifndef SYS_SIMU_SOCKET_H
#define SYS_SIMU_SOCKET_H

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

int simu_socket (int domain, int type, int protocol);
int simu_bind (int fd, const struct sockaddr *my_addr, socklen_t addrlen);
int simu_connect (int fd, const struct sockaddr *my_addr, socklen_t addrlen);
int simu_listen (int sockfd, int backlog);
int simu_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int simu_shutdown(int s, int how);
ssize_t simu_recv (int fd, void *buf, size_t count, int flags);
ssize_t simu_recvfrom(int s, void *buf, size_t len, int flags,
		      struct sockaddr *from, socklen_t *fromlen);
ssize_t simu_recvmsg(int s, struct msghdr *msg, int flags);
int simu_setsockopt(int s, int level, int optname,
		    const void *optval, socklen_t optlen);
int simu_getsockopt(int s, int level, int optname,
		    void *optval, socklen_t *optlen);
ssize_t simu_send(int s, const void *buf, size_t len, int flags);
ssize_t simu_sendto(int s, const void *buf, size_t len, int flags,
		    const struct sockaddr *to, socklen_t tolen);
ssize_t simu_sendmsg(int s, const struct msghdr *msg, int flags);
int simu_getsockname(int s, struct sockaddr *name, socklen_t *namelen);
int simu_getpeername(int s, struct sockaddr *name, socklen_t *namelen);

#ifdef __cplusplus
}
#endif

#endif /* SYS_SIMU_SOCKET_H */
