#ifndef SIMU_FCNTL_H
#define SIMU_FCNTL_H

#include <stdarg.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_open (const char *path, int flags, ...);
int dce_open64 (const char *path, int flags, ...);
int dce_creat (const char *path, mode_t mode);
int dce_fcntl (int fd, int cmd, ...);
int dce_unlinkat (int dirfd, const char *pathname, int flags);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_FCNTL_H */
