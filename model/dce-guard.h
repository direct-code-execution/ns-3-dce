//#ifdef DCE
//  #pragma error DCE should be defined
//#else
	#define DCE(rtype, name, args...) extern "C" rtype dce_ ## name args ;
//#endif

//#ifndef NATIVE
	#define NATIVE(name) 
	#define NATIVE_EXPLICIT(name, proto)
	#define NATIVE_WITH_ALIAS (name)
//#endif
