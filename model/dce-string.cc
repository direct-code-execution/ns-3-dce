#include "dce-string.h"
#include "dce-stdlib.h"
#include <string.h>

char *dce_strdup (const char *s)
{
  size_t len = strlen (s);
  char *str = (char*)dce_malloc (len+1);
  memcpy (str, s, len+1);
  return str;
}
char *dce_strndup (const char *s, size_t n)
{
  size_t len = strlen (s);
  len = (len < n) ? len : n;
  char *str = (char*)dce_malloc (len+1);
  memcpy (str, s, len);
  str[len] = 0;
  return str;
}

char *dce___strcpy_chk (char *__restrict __dest,
						const char *__restrict __src,
						size_t __destlen)
{
  /// \todo Do actual checking
  strcpy (__dest, __src); 
}
