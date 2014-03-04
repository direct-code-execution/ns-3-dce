
#include "libc.h"

struct Libc g_libc;

// macros stolen from glibc.
#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)))

extern "C" {

// Step 2.  Very dirty trick to force redirection to library functions
// This will work only with GCC. Number 128 was picked to be arbitrarily large to allow
// function calls with a large number of arguments.
// \see http://tigcc.ticalc.org/doc/gnuexts.html#SEC67___builtin_apply_args
// FIXME: 120925: 128 was heuristically picked to pass the test under 32bits environment.
#define NATIVE DCE
#define NATIVET DCET
#define NATIVE_WITH_ALIAS DCE_WITH_ALIAS
#define NATIVE_WITH_ALIAS2 DCE_WITH_ALIAS2

#define GCC_BT_NUM_ARGS 128

#define GCC_BUILTIN_APPLY(export_symbol, func_to_call) \
  void export_symbol (...) { \
    void *args =  __builtin_apply_args (); \
    void *result = __builtin_apply (g_libc.func_to_call ## _fn, args, GCC_BT_NUM_ARGS); \
    __builtin_return (result); \
  }

#define GCC_BUILTIN_APPLYT(rtype, export_symbol, func_to_call) \
  rtype export_symbol (...) { \
    void *args =  __builtin_apply_args (); \
    void *result = __builtin_apply ((void (*) (...)) g_libc.func_to_call ## _fn, args, GCC_BT_NUM_ARGS); \
    __builtin_return (result); \
  }


#define DCE(name)                                                               \
  GCC_BUILTIN_APPLY (name,name)

#define DCET(rtype,name)                                                               \
  GCC_BUILTIN_APPLYT (rtype,name,name)

/* From gcc/testsuite/gcc.dg/cpp/vararg2.c */
/* C99 __VA_ARGS__ versions */
#define c99_count(...)    _c99_count1 (, ## __VA_ARGS__) /* If only ## worked.*/
#define _c99_count1(...)  _c99_count2 (__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0)
#define _c99_count2(_,x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,n,...) n

#define FULL_ARGS_0()
#define FULL_ARGS_1(X0)  X0 a0
#define FULL_ARGS_2(X0,X1)  X0 a0, X1 a1
#define FULL_ARGS_3(X0,X1,X2)  X0 a0, X1 a1, X2 a2
#define FULL_ARGS_4(X0,X1,X2,X3)  X0 a0, X1 a1, X2 a2, X3 a3
#define FULL_ARGS_5(X0,X1,X2,X3,X4)  X0 a0, X1 a1, X2 a2, X3 a3, X4 a4

#define _ARGS_0()
#define _ARGS_1(X0)  a0
#define _ARGS_2(X0,X1)   a0, a1
#define _ARGS_3(X0,X1,X2)  a0, a1, a2
#define _ARGS_4(X0,X1,X2,X3)  a0, a1, a2, a3
#define _ARGS_5(X0,X1,X2,X3,X4) a0, a1, a2, a3, a4

#define CAT(a, ...) PRIMITIVE_CAT (a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define  FULL_ARGS(...) CAT (FULL_ARGS_,c99_count (__VA_ARGS__)) (__VA_ARGS__)
#define  ARGS(...) CAT (_ARGS_,c99_count (__VA_ARGS__)) (__VA_ARGS__)


#define DCE_EXPLICIT(name,rtype,...)                                    \
  rtype name (FULL_ARGS (__VA_ARGS__))    \
  {                                                             \
    return g_libc.name ## _fn (ARGS (__VA_ARGS__));              \
  }

#define DCE_WITH_ALIAS(name)                                    \
  GCC_BUILTIN_APPLY (__ ## name,name)                      \
  weak_alias (__ ## name, name);

#define DCE_WITH_ALIAS2(name, internal)                 \
  GCC_BUILTIN_APPLY (internal,name)                        \
  weak_alias (internal, name);


// Note: it looks like that the stdio.h header does
// not define putc and getc as macros if you include
// them from C++ so that we do need to define the putc
// and getc functions anyway.
#undef putc
#undef getc

#include "libc-ns3.h" // do the work

// weak_alias (strtol, __strtol_internal);
// weak_alias (wctype_l, __wctype_l);
// weak_alias (strdup, __strdup);

// void exit(int status)
// {
//   g_libc.exit_fn (status);
//   int a = 0;
//   while (1)
//     {
//       // loop forever to quiet compiler warning:
//       // warning: ‘noreturn’ function does return
//       a++;
//     }
// }

// void abort(void)
// {
//   g_libc.abort_fn ();
//   int a = 0;
//   while (1)
//     {
//       // loop forever to quiet compiler warning:
//       // warning: ‘noreturn’ function does return
//       a++;
//     }
// }

char * strpbrk (const char *s, const char *a)
{
  return g_libc.strpbrk_fn (s,a);
}

char * strstr (const char *u, const char *d)
{
  return g_libc.strstr_fn (u,d);
}

int snprintf (char *s, size_t si, const char *f, ...)
{
  va_list vl;
  va_start (vl, f);
  int r =  g_libc.vsnprintf_fn (s, si, f, vl);
  va_end (vl);

  return r;
}
int vsnprintf (char *s, size_t si, const char *f, va_list v)
{
  return g_libc.vsnprintf_fn (s, si, f, v);
}

#include "libc-globals.h"

void LIBSETUP (const struct Libc *fn)
{
  /* The following assignment of fn to g_libc is a bit weird: we perform a copy of the data
   * structures by hand rather than executing g_libc = fn. This is obviously done on purpose.
   * The reason is that g_libc = fn would trigger a call to the memcpy function because the
   * Libc structure is very big. The memcpy function is resolved through the dynamic loader's
   * symbol lookup mechanism to the local memcpy function and that local memcpy function happens
   * to be calling g_libc.memcpy_fn which is set to NULL before the data structure is initialized.
   */
  const unsigned char *src = (const unsigned char *)fn;
  unsigned char *dst = (unsigned char *)&g_libc;
  unsigned int i;
  for (i = 0; i < sizeof (struct Libc); ++i)
    {
      *dst = *src;
      src++;
      dst++;
    }

  setup_global_variables ();
}

#ifdef HAVE_GETCPUFEATURES
// Below there is an exception implementation: because the libm of glibc2.14 call  __get_cpu_features during dlopen,
// and during dlopen of libm  DCE do not have yet called lib_setup so there we implement __get_cpu_features
// directly without using the global g_libc variable, we can do it only if our implementation of the method
// do not interract with any ressouces of DCE or NS3 and do no call any other libc methods ...
struct cpu_features;
extern const struct cpu_features * dce___get_cpu_features (void);
const struct cpu_features * __get_cpu_features (void)
{
  return dce___get_cpu_features ();
}
#endif
} // extern "C"
