
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_SYS_STATFS_H
#define DCE_HEADER_SYS_STATFS_H
// TODO add extern "C" ?
#include <sys/statfs.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 int dce_fstatfs (int __fildes,statfs * __buf);

 int dce_fstatfs64 (int __fildes,statfs64 * __buf);


#ifdef __cplusplus
}
#endif
#endif
