
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_SYS_VFS_H
#define DCE_HEADER_SYS_VFS_H
// TODO add extern "C" ?
#include <sys/vfs.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 int dce_fstatfs (int __fildes, struct statfs * __buf) noexcept;

 int dce_fstatfs64 (int __fildes, struct statfs64 * __buf) noexcept;

 int dce_statfs (const char *path, struct statfs *buf) noexcept;

 int dce_statfs64 (const char *path, struct statfs64 *buf) noexcept;


#ifdef __cplusplus
}
#endif
#endif
