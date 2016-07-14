
#include "dce-guard.h"
//#undef DCE 

//#define DCE(rtype, name, args...) rtype dce_ ## name args ;

// TODO pas forcement externes
DCE(void *, dlopen, (const char *filename, int flag));
DCE(void *, dlsym, (void *handle, const char *symbol));

// REGRESSION
//DCE_WITH_ALIAS2 (dlopen, __dlopen)
