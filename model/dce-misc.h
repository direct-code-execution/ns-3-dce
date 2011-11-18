#ifndef SIMU_MISC_H
#define SIMU_MISC_H


#include <sys/utsname.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_uname (struct utsname *buf);
int dce_gethostname (char *name, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_MISC_H */
