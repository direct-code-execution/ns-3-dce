#ifndef DCE_IOCTL_H
#define DCE_IOCTL_H

#include "../dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int, ioctl, (int d, long unsigned int request, ...));

#ifdef __cplusplus
}
#endif


#endif /* DCE_IOCTL_H */
