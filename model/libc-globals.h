#ifndef LIBC_GLOBALS_H
#define LIBC_GLOBALS_H

#include <stdio.h>

struct LibcGlobals
{
  FILE *stdin;
  FILE *stdout;
  FILE *stderr;
  char **envp;
};

#endif /* LIBC_GLOBALS_H */
