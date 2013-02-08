#ifndef DCE_STAT_H
#define DCE_STAT_H

#ifdef __cplusplus
extern "C" {
#endif

int dce___xstat (int ver, const char *path, struct stat *buf);
int dce___fxstat (int ver, int fd, struct stat *buf);
int dce___lxstat (int ver, const char *pathname, struct stat *buf);

int dce___xstat64 (int ver, const char *path, struct stat64 *buf);
int dce___fxstat64 (int ver, int fd, struct stat64 *buf);
int dce___lxstat64 (int ver, const char *pathname, struct stat64 *buf);
int dce___fxstatat (int ver, int fd, const char *filename,
                    struct stat *buf, int flag);
int dce_fstat (int fd, struct stat *buf);
int dce_fstat64 (int fd, struct stat64 *buf);

int dce_mkdir (const char *pathname, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* DCE_STAT_H */
