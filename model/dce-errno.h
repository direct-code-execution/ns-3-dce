#ifndef ERRNO_H
#define ERRNO_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define dce_errno (*dce_get_errno ())

int *dce_get_errno (void);

#ifdef __cplusplus
}
#endif


#endif /* ERRNO_H */
