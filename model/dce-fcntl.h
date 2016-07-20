#ifndef SIMU_FCNTL_H
#define SIMU_FCNTL_H

#include <stdarg.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "dce-guard.h"

DCE(int, open, const char *path, int flags, ...);
DCE(int, open64, const char *path, int flags, ...);
DCE(int, creat, const char *path, mode_t mode);
DCE(int, fcntl, int fd, int cmd, ...);
DCE(int, unlinkat, int dirfd, const char *pathname, int flags);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_FCNTL_H */
