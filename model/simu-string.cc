#include "simu-string.h"
#include "simu-stdlib.h"
#include <string.h>

char *simu_strdup(const char *s)
{
  size_t len = strlen (s);
  char *str = (char*)simu_malloc (len+1);
  memcpy (str, s, len+1);
  return str;
}
char *simu_strndup(const char *s, size_t n)
{
  size_t len = strlen (s);
  len = (len < n)?len:n;
  char *str = (char*)simu_malloc (len+1);
  memcpy (str, s, len);
  str[len] = 0;
  return str;
}
