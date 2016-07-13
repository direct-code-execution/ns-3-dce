

DCE(char *, strdup, (const char *s));
DCE(char *, strndup, (const char *s, size_t n));
DCE(char *, __strcpy_chk, (char *__restrict __dest,
                         const char *__restrict __src,
                         size_t __destlen));
DCE(char *, strpbrk, (const char *s, const char *accept));
DCE(char *, strstr, (const char *h, const char *n));

NATIVE (bzero)
NATIVE (strerror)
NATIVE (strerror_r)
NATIVE (strcoll)
NATIVE (memset)
NATIVE (memcpy)
NATIVE (bcopy)
NATIVE (memcmp)
NATIVE (memmove)
NATIVE_EXPLICIT (memchr, void * (*)(void *, int, size_t))
NATIVE_EXPLICIT (memrchr, void * (*)(void *, int, size_t))
NATIVE (strcpy)
NATIVE (strncpy)
NATIVE (strcat)
NATIVE (strncat)
NATIVE (strcmp)
NATIVE (strncmp)
NATIVE (strlen)
NATIVE (strnlen)
NATIVE (strcspn)
NATIVE (strspn)
NATIVE_EXPLICIT (strchr, char* (*)(char *, int))
NATIVE_EXPLICIT (strrchr, const char * (*)(const char *, int))
NATIVE (strcasecmp)
NATIVE (strncasecmp)
//DCE_WITH_ALIAS (strdup) // because C++ defines both const and non-const functions

NATIVE_EXPLICIT (index, char * (*)(char *, int))
NATIVE_EXPLICIT (rindex, char * (*)(char *, int))
NATIVE_EXPLICIT (strtok, char * (*)(char *, const char *))
NATIVE_EXPLICIT (strtok_r,  char * (*)(char *, const char *, char **))
NATIVE (strsep)