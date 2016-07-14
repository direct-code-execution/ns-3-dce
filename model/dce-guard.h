//#ifdef DCE
//  #pragma error DCE should be defined
//#else
	#define DCE(rtype, name, args...) extern "C" rtype dce_ ## name args ;
//#endif

//#ifndef NATIVE
	#define NATIVE(name) 
	#define NATIVE_EXPLICIT(name, proto)

//#endif

// macros stolen from glibc. does not seem to work with clang
//The weak attribute causes the declaration to be emitted as a weak symbol rather 
//than a global. This is primarily useful in defining library functions which can 
//be overridden in user code, though it can also be used with non-function declarations
#define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (# name)))
  
//#define DCE_WITH_ALIAS(name) weak_alias (__ ## name, name);
#define DCE_WITH_ALIAS(name)

#define NATIVE_WITH_ALIAS NATIVE

//#define DCE_WITH_ALIAS2(name, internal) weak_alias (internal, name);
#define DCE_WITH_ALIAS2(name, internal)

#define NATIVE_WITH_ALIAS2 DCE_WITH_ALIAS2
