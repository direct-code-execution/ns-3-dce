#ifndef DCE_UMASK_H
#define DCE_UMASK_H

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

mode_t dce_umask (mode_t mask);

#ifdef __cplusplus
}
#endif

#endif /* DCE_UMASK_H */

