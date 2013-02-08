#ifndef SIMU_PWD_H
#define SIMU_PWD_H

#include <sys/types.h>
#include <pwd.h>

#ifdef __cplusplus
extern "C" {
#endif

struct passwd * dce_getpwnam (const char *name);

struct passwd * dce_getpwuid (uid_t uid);

void dce_endpwent (void);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_PWD_H */
