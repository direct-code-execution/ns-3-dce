#ifndef LIBC_H
#define LIBC_H

#define _SYS_SELECT_H
#include <sys/types.h>
#undef _SYS_SELECT_H

struct Libc
{

#define DCE(name) void (*name ## _fn)(...);

#define DCET(rtype, name) DCE(name)

#define DCE_EXPLICIT(name,rtype,...) rtype (*name ## _fn)(__VA_ARGS__);
#include "libc-ns3.h"

};


#endif /* LIBC_H */
