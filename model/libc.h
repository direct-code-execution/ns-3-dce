#ifndef LIBC_H
#define LIBC_H

#include <stdarg.h>
#define _SYS_SELECT_H
#include <sys/types.h>
#undef _SYS_SELECT_H

struct Libc
{

#define DCE(name) void (*name ## _fn) (...);

#define DCET(rtype, name) DCE (name)
#define NATIVET(rtype, name) NATIVE (name)

#define DCE_EXPLICIT(name,rtype,...) rtype (*name ## _fn) (__VA_ARGS__);
#include "libc-ns3.h"

  char* (*strpbrk_fn)(const char *s, const char *accept);
  char* (*strstr_fn)(const char *a, const char *b);
  int (*vsnprintf_fn)(char *str, size_t size, const char *format, va_list v);
};


#endif /* LIBC_H */
