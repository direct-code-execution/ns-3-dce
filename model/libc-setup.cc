#undef __OPTIMIZE__
#include <stdio.h>
#include <unistd.h>

#include "libc-globals.h"
#include "dce-global-variables.h"
#include "libc.h"

extern Libc g_libc; // this is initialized in libc.c

extern "C" {

void setup_global_variables ()
{
  struct SimuGlobalVariables globals;
  globals.pstdin = &stdin;
  globals.pstdout = &stdout;
  globals.pstderr = &stderr;
  globals.penvp = &environ;
  globals.poptarg = &optarg;
  globals.popterr = &opterr;
  globals.poptind = &optind;
  globals.poptopt = &optopt;

  typedef void (*dce_global_variables_setup_t) (struct SimuGlobalVariables *);
  ((dce_global_variables_setup_t)g_libc.dce_global_variables_setup_fn) (&globals);
}

}
