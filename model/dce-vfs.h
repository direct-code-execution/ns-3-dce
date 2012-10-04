#ifndef SIMU_VFS_H
#define SIMU_VFS_H

#include <sys/vfs.h>
#include <sys/statvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_statfs (const char *path, struct statfs *buf);
int dce_fstatfs (int fd, struct statfs *buf);
int dce_statfs64 (const char *path, struct statfs *buf);
int dce_fstatfs64 (int fd, struct statfs *buf);

int dce_statvfs (const char *path, struct statvfs *buf);
int dce_fstatvfs (int fd, struct statvfs *buf);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_VFS_H */
