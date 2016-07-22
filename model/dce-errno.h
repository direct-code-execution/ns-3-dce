#ifndef ERRNO_H
#define ERRNO_H

#include <errno.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

#define dce_errno (*dce___errno_location ())
#define dce_h_errno (*dce__h_errno_location ())

DCE(int *, __errno_location);
DCE(int *, __h_errno_location);

#ifdef __cplusplus
}
#endif


#endif /* ERRNO_H */
