
#include "libc.h"

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


#if 0


#define DCE_EXPLICIT(name,rtype,...)                                    \
  rtype name (FULL_ARGS (__VA_ARGS__))    \
  {                                                             \
    return g_libc.name ## _fn (ARGS (__VA_ARGS__));              \
  }

#endif // if 0



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

//#define NATIVE(name) decltype(&name) name ## _fn ;
//#define NATIVE_EXPLICIT(name, type) decltype( (type) &name) name ## _fn ;
// return DCE
#define DCE(rtype,name, args...)  rtype  __attribute__((weak)) name args {};
// TODO generate fake entry too ? mark it as weak ?
//#define NATIVE(name) extern decltype(name) name;
#define NATIVE(name) 
//#define NATIVE(name) decltype(name) name __attribute__((weak)) {};
#define NATIVE_EXPLICIT(name, type) 
#define NATIVE_WITH_ALIAS(name) 
#define NATIVE_WITH_ALIAS2(name, alias) 
#define DCE_WITH_ALIAS2(name, internal)
#define DCE_WITH_ALIAS(name)

#define DCE_ALIAS(name, internal)  weak_alias (name, internal);


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
#include "libc-ns3.generated.h" // do the work

#undef DCE
#undef NATIVE
#undef DCE_WITH_ALIAS
#undef DCE_WITH_ALIAS2
#undef NATIVE_WITH_ALIAS

extern void __cxa_finalize (void *d);
extern int __cxa_atexit (void (*func)(void *), void *arg, void *d);

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
