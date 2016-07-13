
#include "dce-guard.h"

DCE(void *, dlopen, (const char *filename, int flag));
DCE(void *, dlsym, (void *handle, const char *symbol));

DCE_WITH_ALIAS2 (dlopen, __dlopen)
