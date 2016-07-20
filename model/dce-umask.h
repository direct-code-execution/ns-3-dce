#ifndef DCE_UMASK_H
#define DCE_UMASK_H

#include <sys/types.h>
#include <sys/stat.h>

#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(mode_t, umask, mode_t mask);

#ifdef __cplusplus
}
#endif

#endif /* DCE_UMASK_H */

