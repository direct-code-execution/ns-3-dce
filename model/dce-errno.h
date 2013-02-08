#ifndef ERRNO_H
#define ERRNO_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define dce_errno (*dce___errno_location ())

int * dce___errno_location (void);

#ifdef __cplusplus
}
#endif


#endif /* ERRNO_H */
