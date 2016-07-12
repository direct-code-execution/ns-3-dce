#ifndef DCE
	#define DCE(rtype, name, args...) extern "C" rtype dce_ ## name args ;
#endif

#ifndef NATIVE
	#define NATIVE(name) 
#endif
