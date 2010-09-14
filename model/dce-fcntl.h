#ifndef SIMU_FCNTL_H
#define SIMU_FCNTL_H

#include <stdarg.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_open (const char *path, int flags, mode_t mode);
int dce_creat (const char *path, mode_t mode);
int dce_fcntl(int fd, int cmd, unsigned long arg);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_FCNTL_H */
