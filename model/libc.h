#ifndef LIBC_H
#define LIBC_H

struct Libc
{

#define DCE(name) \
  void (*name ## _fn)(...);

#include "libc-ns3.h"

};


#endif /* LIBC_H */
