#ifndef DCE_IF_H
#define DCE_IF_H


#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(unsigned , if_nametoindex, (const char *ifname));
DCE(char * , if_indextoname, (unsigned ifindex, char *ifname));


#ifdef __cplusplus
}
#endif

#endif /* DCE_IF_H */
