#ifndef SIMU_VFS_H
#define SIMU_VFS_H

#include <sys/vfs.h>
#include <sys/statvfs.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int, statfs, (const char *path, struct statfs *buf));
DCE(int, fstatfs, (int fd, struct statfs *buf));
DCE(int, statfs64, (const char *path, struct statfs *buf));
DCE(int, fstatfs64, (int fd, struct statfs *buf));

DCE(int, statvfs, (const char *path, struct statvfs *buf));
DCE(int, fstatvfs, (int fd, struct statvfs *buf));

#ifdef __cplusplus
}
#endif

#endif /* SIMU_VFS_H */
