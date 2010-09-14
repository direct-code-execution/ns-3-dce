#ifndef SIMU_STAT_H
#define SIMU_STAT_H

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat;
struct stat64;

int simu_xstat (int ver, const char *path, struct stat *buf);
int simu_fxstat (int ver, int fd, struct stat *buf);
int simu_lxstat (int ver, const char *pathname, struct stat *buf);

int simu_xstat64 (int ver, const char *path, struct stat64 *buf);
int simu_fxstat64 (int ver, int fd, struct stat64 *buf);
int simu_lxstat64 (int ver, const char *pathname, struct stat64 *buf);

int simu_mkdir(const char *pathname, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_STAT_H */
