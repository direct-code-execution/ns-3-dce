#ifndef SIMU_STRING_H
#define SIMU_STRING_H

//extern "C" {
#ifdef __cplusplus
extern "C" {
#endif

//#undef __CORRECT_ISO_CPP_STRING_H_PROTO
#include <cstring>
#include "dce-guard.h"




// returns const char* in C++ version only 
//DCE(char *, strpbrk, (const char *s, const char *accept));
//DCE(const char *, strstr, (const char *h, const char *n));
NATIVE_EXPLICIT(strpbrk, const char* (*)(const char*, const char*) )
NATIVE_EXPLICIT( strstr, const char* (*)(const char*, const char*) )

DCE(char *, strdup, const char *s);
DCE(char *, strndup, const char *s, size_t n);
DCE(char *, __strcpy_chk, char *__restrict __dest, const char *__restrict __src, size_t __destlen);


NATIVE (bzero, void *__s, size_t __n)
NATIVE (strerror, int __errnum)
NATIVE (strerror_r, int __errnum, char *__buf, size_t __buflen)
NATIVE (strcoll, const char *__s1, const char *__s2)
NATIVE (memset, void *__s, int __c, size_t __n)
NATIVE (memcpy, void *__restrict __dest, const void *__restrict __src, size_t __n)
NATIVE (bcopy, const void *__src, void *__dest, size_t __n)
NATIVE (memcmp, const void *__s1, const void *__s2, size_t __n)
NATIVE (memmove, void *__dest, const void *__src, size_t __n)

NATIVE (strcpy, char *__restrict __dest, const char *__restrict __src)
NATIVE (strncpy, char *__restrict __dest, const char *__restrict __src, size_t __n)
NATIVE (strcat, char *__restrict __dest, const char *__restrict __src)
NATIVE (strncat, char *__restrict __dest, const char *__restrict __src, size_t __n)
NATIVE (strcmp, const char *__s1, const char *__s2)
NATIVE (strncmp, const char *__s1, const char *__s2, size_t __n)
NATIVE (strlen, const char *)
NATIVE (strnlen, const char *__string, size_t __maxlen)
NATIVE (strcspn, const char *__s, const char *__reject)
NATIVE (strspn, const char *__s, const char *__accept)
NATIVE (strcasecmp, const char *__s1, const char *__s2)
NATIVE (strncasecmp, const char *__s1, const char *__s2, size_t __n)
//DCE_WITH_ALIAS (strdup) // because C++ defines both const and non-const functions
NATIVE (strsep, char **__restrict __stringp, const char *__restrict __delim)

NATIVE_EXPLICIT (memchr, void * (*)(void *, int, size_t))
NATIVE_EXPLICIT (memrchr, void * (*)(void *, int, size_t))
NATIVE_EXPLICIT (index, char * (*)(char *, int))
NATIVE_EXPLICIT (rindex, char * (*)(char *, int))
NATIVE_EXPLICIT (strtok, char * (*)(char *, const char *))
NATIVE_EXPLICIT (strtok_r,  char * (*)(char *, const char *, char **))
NATIVE_EXPLICIT (strchr, char* (*)(char *, int))
NATIVE_EXPLICIT (strrchr, const char * (*)(const char *, int))

#ifdef __cplusplus
}
#endif


#endif /* SIMU_STRING_H */
