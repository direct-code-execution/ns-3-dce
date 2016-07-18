#ifndef DCE_LOCALE_H
#define DCE_LOCALE_H

#include <locale.h>

#include "dce-guard.h"

DCE(char *, setlocale, (int category, const char *locale));

NATIVE(newlocale, int __category_mask, const char *__locale, __locale_t __base)
//NATIVE(newlocale)
//NATIVE(uselocale)

// REGRESSION
DCE_ALIAS(newlocale, __newlocale)
DCE_ALIAS(uselocale, __uselocale)

#endif // DCE_LOCALE_H
