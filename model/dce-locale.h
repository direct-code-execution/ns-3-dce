#ifndef DCE_LOCALE_H
#define DCE_LOCALE_H

#include <locale.h>

#include "dce-guard.h"
//#include "dce-locale.i"

DCE(char *, setlocale, (int category, const char *locale));

//NATIVE_WITH_ALIAS (newlocale)
//NATIVE_WITH_ALIAS (uselocale)

#endif // DCE_LOCALE_H
