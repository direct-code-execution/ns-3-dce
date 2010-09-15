#ifndef DCE_STAT_H
#define DCE_STAT_H

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat;
struct stat64;

int dce_xstat (int ver, const char *path, struct stat *buf);
int dce_fxstat (int ver, int fd, struct stat *buf);
int dce_lxstat (int ver, const char *pathname, struct stat *buf);

int dce_xstat64 (int ver, const char *path, struct stat64 *buf);
int dce_fxstat64 (int ver, int fd, struct stat64 *buf);
int dce_lxstat64 (int ver, const char *pathname, struct stat64 *buf);

int dce_fstat (int fd, struct stat *buf);
int dce_fstat64 (int fd, struct stat64 *buf);

int dce_mkdir(const char *pathname, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* DCE_STAT_H */
