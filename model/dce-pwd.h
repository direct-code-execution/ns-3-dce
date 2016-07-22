#ifndef SIMU_PWD_H
#define SIMU_PWD_H

#include <sys/types.h>
#include <pwd.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(struct passwd * , getpwnam, const char *name);

DCE(struct passwd * , getpwuid, uid_t uid);

DCE(void , endpwent);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_PWD_H */
