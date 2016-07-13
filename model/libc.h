#ifndef LIBC_H
#define LIBC_H

#include <stdarg.h>
#define _SYS_SELECT_H
#include <sys/types.h>
#undef _SYS_SELECT_H
#include <wchar.h>
#include <link.h>

// Generate struct
//#define GENERATE_LIBC

//on peut utiliser des templates pour recuperer les arguments 
#define DCE(rtype, name, args...) rtype (*name ## _fn) args ;
/* native => decltype(name) */
#define NATIVE(name) decltype(name) name ## _fn ;

// NATIVE_EXPLICIT
struct Libc
{


/* #define DCET(rtype, name) DCE (name) */
/* #define NATIVET(rtype, name) NATIVE (name) */

/* #define DCE_EXPLICIT(name,rtype,...) rtype (*name ## _fn) (__VA_ARGS__); */
 #include "libc-ns3.generated.h"

  void (*dce_global_variables_setup_fn)(struct DceGlobalVariables *variables);
  char* (*strpbrk_fn)(const char *s, const char *accept);
  char* (*strstr_fn)(const char *a, const char *b);
  int (*vsnprintf_fn)(char *str, size_t size, const char *format, va_list v);
};

#undef DCE
#undef NATIVE

//#undef GENERATE_LIBC

#endif /* LIBC_H */
