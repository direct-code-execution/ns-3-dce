
#include "libc.h"

struct Libc g_libc;

// macros stolen from glibc.
#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)))

extern "C" {

// Step 2.  Very dirty trick to force redirection to library functions
// This will work only with GCC. Number 256 was picked to be arbitrarily large to allow
// function calls with a large number of arguments.
// \see http://tigcc.ticalc.org/doc/gnuexts.html#SEC67___builtin_apply_args
#define NATIVE DCE
#define NATIVE_WITH_ALIAS DCE_WITH_ALIAS

#define GCC_BUILTIN_APPLY(export_symbol, func_to_call)					\
	void export_symbol (...) {											\
		void *args =  __builtin_apply_args();							\
		void *result = __builtin_apply( g_libc.func_to_call ## _fn, args, 256 ); \
		__builtin_return (result);										\
	}

#define DCE(name)								\
	GCC_BUILTIN_APPLY(name,name)
	
#define DCE_WITH_ALIAS(name)					\
	GCC_BUILTIN_APPLY(__ ## name,name)			\
	weak_alias(__ ## name, name);

#define DCE_WITH_ALIAS2(name, internal)			\
	GCC_BUILTIN_APPLY(internal,name)			\
	weak_alias(internal, name);
   

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

} // extern "C"
