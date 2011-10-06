#ifndef SIMU_MISC_H
#define SIMU_MISC_H


#include <sys/utsname.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_uname (struct utsname *buf);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_MISC_H */
