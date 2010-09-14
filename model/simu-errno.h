#ifndef ERRNO_H
#define ERRNO_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define simu_errno (*simu_get_errno ())

int *simu_get_errno (void);

#ifdef __cplusplus
}
#endif


#endif /* ERRNO_H */
