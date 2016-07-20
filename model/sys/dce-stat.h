#ifndef DCE_STAT_H
#define DCE_STAT_H

#include "../dce-guard.h"
// rajoute des int ca passe ?

//#include "dce-stat.i"

DCE(int, __xstat , int ver, const char *, struct stat *buf);
DCE(int, __fxstat , int ver, int fd, struct stat *buf);
DCE(int, __lxstat , int ver, const char *pathname, struct stat *buf);

DCE(int, __xstat64 , int ver, const char *path, struct stat64 *buf);
DCE(int, __fxstat64 , int ver, int fd, struct stat64 *buf);
DCE(int, __lxstat64 , int ver, const char *pathname, struct stat64 *buf);
DCE(int, __fxstatat , int ver, int fd, const char *filename, struct stat *buf, int flag);
DCE(int, fstat , int fd, struct stat *buf);
DCE(int, fstat64 , int fd, struct stat64 *buf);

DCE(int, mkdir , const char *pathname, mode_t mode);

#endif /* DCE_STAT_H */


