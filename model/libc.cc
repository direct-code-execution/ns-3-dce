
#include "libc.h"
#include "dce-unistd.h"
#include "dce-errno.h"
#include "dce-libc-private.h"
#include "dce-signal.h"
#include "dce-netdb.h"
#include "dce-unistd.h"
#include "dce-time.h"
#include "sys/dce-socket.h"
#include "dce-pthread.h"
#include "dce-stdio.h"
#include "dce-stdarg.h"
#include "dce-stdlib.h"
#include "dce-string.h"
#include "dce-locale.h"
#include "dce-getopt.h"
#include "sys/dce-select.h"
#include "arpa/dce-inet.h"
#include "sys/dce-ioctl.h"
#include "sys/dce-time.h"
#include "sys/dce-sysinfo.h"
#include "sys/dce-uio.h"
#include "dce-stdio2.h"
#include "dce-sched.h"
#include "dce-fcntl.h"
#include "dce-utime.h"
#include "dce-grp.h"
#include "dce-pwd.h"
#include "dce-libio.h"
#include "dce-termios.h"
#include "dce-dlfcn.h"
#include "sys/dce-stat.h"
#include "sys/dce-vfs.h"
#include "sys/dce-statvfs.h"
#include "sys/dce-utsname.h"
#include "sys/dce-wait.h"
#include "dce-poll.h"
#include "dce-dirent.h"
#include "dce-semaphore.h"
#include "dce-ifaddrs.h"
#include "sys/dce-mman.h"
#include "sys/dce-timerfd.h"
#include "sys/dce-syslog.h"
#include "net/dce-if.h"
#include <utility>
#include <stdlib.h>
//extern "C"
//{
// Don't need that anymore right ?
//#include <p99_args.h>
//}
struct Libc g_libc;

// macros stolen from glibc.
//The weak attribute causes the declaration to be emitted as a weak symbol rather 
//than a global. This is primarily useful in defining library functions which can 
//be overridden in user code, though it can also be used with non-function declarations
// TODO use decltype(&name)

//https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes
//The alias attribute causes the declaration to be emitted as an alias for another symbol, which must be specified. For instance,
//
//          void __f () { /* Do something. */; }
//          void f () __attribute__ ((weak, alias ("__f")));
//
//defines ‘f’ to be a weak alias for ‘__f’. In C++, the mangled name for the target must be used. It is an error if ‘__f’ is not defined in the same translation unit. 
#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)));

//#define weak(name) __attribute__((weak))


//#define GCC_BT_NUM_ARGS 128
//
//#define GCC_BUILTIN_APPLY(export_symbol, func_to_call) \
//  void export_symbol (...) { \
//    void *args =  __builtin_apply_args (); \
//    void *result = __builtin_apply (g_libc.func_to_call ## _fn, args, GCC_BT_NUM_ARGS); \
//    __builtin_return (result); \
//  }




//#define c99_count(...)    sizeof...(__VA_ARGS__)) /* If only ## worked.*/
template<class... Types>
struct test {
    static const std::size_t value = sizeof...(Types);
};


#define c99_count(...)    P99_NARG(__VA_ARGS__)
//#define c99_count(...)    _c99_count1 (__VA_ARGS__) /* If only ## worked.*/
//#define _c99_count1(...)  _c99_count2 (__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0)
//#define _c99_count1(...)  _c99_count2 (__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0)
//#define _c99_count2(_,x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,n,...) n

namespace detail_paramType {
    template <class>
    struct unpackType;

    template <class T>
    struct unpackType<void(T)> { using type = T; };

    struct VariadicC {};

    template <> 
    struct unpackType<void(...)> { using type= VariadicC; };

    template <> 
    struct unpackType<void(void)> { using type= void; };
}

#define PARAM_TYPE(...) \
    typename detail_paramType::unpackType<void(__VA_ARGS__)>::type


#define FULL_ARGS_0()
#define FULL_ARGS_1(X0)  PARAM_TYPE(X0) a0
#define FULL_ARGS_2(X0,X1)  PARAM_TYPE(X0) a0, PARAM_TYPE(X1) a1
#define FULL_ARGS_3(X0,X1,X2)  PARAM_TYPE(X0) a0, PARAM_TYPE(X1) a1, PARAM_TYPE(X2) a2
#define FULL_ARGS_4(X0,X1,X2,X3)  PARAM_TYPE(X0) a0, PARAM_TYPE(X1) a1, PARAM_TYPE(X2) a2, PARAM_TYPE(X3) a3
#define FULL_ARGS_5(X0,X1,X2,X3,X4)  PARAM_TYPE(X0) a0, PARAM_TYPE(X1) a1, PARAM_TYPE(X2) a2, PARAM_TYPE(X3) a3, PARAM_TYPE(X4) a4
#define FULL_ARGS_6(X0,X1,X2,X3,X4,X5)  PARAM_TYPE(X0) a0, PARAM_TYPE(X1) a1, PARAM_TYPE(X2) a2, PARAM_TYPE(X3) a3, PARAM_TYPE(X4) a4, PARAM_TYPE(X5) a5
#define FULL_ARGS_7(X0,X1,X2,X3,X4,X5, X6)  PARAM_TYPE(X0) a0, PARAM_TYPE(X1) a1, PARAM_TYPE(X2) a2, PARAM_TYPE(X3) a3, PARAM_TYPE(X4) a4, PARAM_TYPE(X5) a5, PARAM_TYPE(X6) a6

// TODO use successive cats ?
#define _ARGS_0()
#define _ARGS_1(X0)  a0
#define _ARGS_2(X0,X1)   a0, a1
#define _ARGS_3(X0,X1,X2)  a0, a1, a2
#define _ARGS_4(X0,X1,X2,X3)  a0, a1, a2, a3
#define _ARGS_5(X0,X1,X2,X3,X4) a0, a1, a2, a3, a4
#define _ARGS_6(X0,X1,X2,X3,X4,X5) a0, a1, a2, a3, a4, a5
#define _ARGS_7(X0,X1,X2,X3,X4,X5,X6) a0, a1, a2, a3, a4, a5, a6

#define CAT(a, ...) PRIMITIVE_CAT (a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

/*
 gives a name X0 a0
 */
#define  FULL_ARGS(...) CAT ( FULL_ARGS_, c99_count (__VA_ARGS__) ) (__VA_ARGS__)
#define  ARGS(...) CAT (_ARGS_,c99_count (__VA_ARGS__)) (__VA_ARGS__)




//#define DCE_WITH_ALIAS(name)  weak_alias (__ ## name, name);


// Note: it looks like that the stdio.h header does
// not define putc and getc as macros if you include
// them from C++ so that we do need to define the putc
// and getc functions anyway.
#undef putc
#undef getc


// generate the implementations of stub
//weak_alias (name, dce_ ## name);
//extern "C"
// Implemente comme des stubs, qui ne retourne rien

//#define NATIVE(name,...)       name MATT = test<__VA_ARGS__>::value;
//int name (FULL_ARGS(__VA_ARGS__))   \
//  auto name (FULL_ARGS(__VA_ARGS__)) -> decltype ( name( ARGS(__VA_ARGS__))) 


/* old one with macros */
//#define NATIVE(name,args...)                                    \
//name =  c99_count(args),args|FULL_ARGS(args)

#define NATIVE(name, args...)                                    \
  auto name (FULL_ARGS(args)) \
  -> decltype ( (*g_libc.name ## _fn) ( ARGS(args))) \
  {                                                             \
    return (*g_libc.name ## _fn) (ARGS (args));              \
  } \
//  name =  c99_count(args),args|FULL_ARGS(args)|ARGS(args)


//template <typename T, typename... Args> \
// T wrapper (Args&&... args) -> decltype(  (*g_libc. name ## _fn) ( std::forward<Args>(args)...) ) \
//{ \
//  return g_libc. name ## _fn (std::forward<Args>(args)...); \
//}

/* C++ VARIADIC one */
//#define NATIVE(name,...) extern "C++" \
//template <typename... Args> \
// auto name (Args&&... args) -> decltype(  (*g_libc. name ## _fn) ( std::forward<Args>(args)...) ) \
//{ \
//  return g_libc. name ## _fn (std::forward<Args>(args)...); \
//}
  
#define NATIVE(name) decltype(&name) name ## _fn ;
//#define NATIVE_EXPLICIT(name, type) decltype( (type) &name) name ## _fn ;
// return DCE
// TODO do nothing, the function is called straightaway
#define DCE(rtype,name, args...) 
//#define DCE(rtype,name, args...)  rtype  __attribute__((weak)) name (args) {};

// TODO generate fake entry too ? mark it as weak ?
//#define NATIVE(name) extern decltype(name) name;
//std::declval
//#define NATIVE(name) 
//#define NATIVE(name) decltype(name) name __attribute__((weak));
//#define NATIVE_EXPLICIT(name, type) 
//#define NATIVE_WITH_ALIAS(name) 
//#define NATIVE_WITH_ALIAS2(name, alias) 
//#define DCE_WITH_ALIAS2(name, internal)
//#define DCE_WITH_ALIAS(name)

//#define DCE_ALIAS(name, internal)  weak_alias (name, internal);
//#define DCE_ALIAS(name, internal) 
//#define DCE_ALIAS(name, internal) extern decltype(name) internal __attribute__((weak));
//NATIVE




extern "C" {

//__locale_t __attribute((weak)) newlocale (int __category_mask, const char *__locale,__locale_t __base)
//{
//}

/* 
this code should generate functions with the name
Assign final we should ha
For instance if we have
DCE(__cxa_finalize), it should generate a function with __cxa_finalize
*/
//#include "libc-ns3.generated.h" // do the work

#undef DCE
#undef NATIVE
#undef DCE_WITH_ALIAS
#undef DCE_WITH_ALIAS2
#undef NATIVE_WITH_ALIAS


// Written manually here since not public
//void __cxa_finalize (void *d)
//{
//  g_libc.__cxa_finalize_fn (d);
//}
//
//int __cxa_atexit (void (*func)(void *), void *arg, void *d)
//{
//  return g_libc.__cxa_atexit_fn (func, arg,d);
//}

// include wrappers functions
// TODO to reestablish when pygcxxml pbs fixed
//#include "libc.generated.cc"

#include "libc.generated.tmp.cc"





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


//const char * strpbrk (const char *s, const char *a)
//{
//  return g_libc.strpbrk_fn (s,a);
//}

//const char * strstr (const char *u, const char *d)
//{
//  return g_libc.strstr_fn (u,d);
//}

//int snprintf (char *s, size_t si, const char *f, ...)
//{
//  va_list vl;
//  va_start (vl, f);
//  int r =  g_libc.vsnprintf_fn (s, si, f, vl);
//  va_end (vl);
//
//  return r;
//}
//int vsnprintf (char *s, size_t si, const char *f, va_list v)
//{
//  return g_libc.vsnprintf_fn (s, si, f, v);
//}
//std::forward ()

// CA ca marche
//__locale_t newlocale (int __category_mask, const char *__locale,__locale_t __base) {
//
//  return g_libc.newlocale_fn(__category_mask, __locale, __base);
//}

#include "libc-globals.h"


/**
 * LIBSETUP is a define setup in DCE wscript depending on the lib
 * @see DceManager::LoadMain
 */
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
