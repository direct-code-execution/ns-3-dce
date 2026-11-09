#include <stdio.h>
#include "dce-global-variables.h"
#include "process.h"
#include "utils.h"
#include "task-manager.h"
#include "dce-stdio.h"

using namespace ns3;

void dce_global_variables_setup (struct DceGlobalVariables *variables)
{
  struct Process *process = Current ()->process;
  if (process->pstdin != 0)
    {
      // protect against multiple calls to this function.
      return;
    }
  process->pstdin = variables->pstdin;
  process->pstdout = variables->pstdout;
  process->pstderr = variables->pstderr;
  process->penvp = variables->penvp;
  // Now, we initialize the process variables
  *process->pstdin = dce_fdopen (0, "r");
  *process->pstdout = dce_fdopen (1, "a"); // Append in order to not erase content while doing an exec
  *process->pstderr = dce_fdopen (2, "a"); // Append in order to not erase content while doing an exec
  *process->penvp = process->originalEnvp;

  process->poptarg = variables->poptarg;
  process->poptind = variables->poptind;
  process->popterr = variables->popterr;
  process->poptopt = variables->poptopt;

  *variables->pprogname = process->originalProgname;
  *variables->pprogram_invocation_name = process->originalProgname;
  char *crsr = process->originalProgname;
  char *slash = crsr;
  while (*crsr)
    {
      if (*crsr == '/')
        {
          crsr++;
          slash = crsr;
        }
      else
        {
          crsr++;
        }
    }
  *variables->pprogram_invocation_short_name = slash;
  // the top of the main thread's stack (V8 and Oilpan scan the stack up to it)
  TaskManager *manager = TaskManager::Current ();
  Task *task = Current ()->task;
  uint8_t *bottom = (uint8_t *) manager->GetStackBottom (task);
  *variables->plibc_stack_end = bottom != 0 ? bottom + manager->GetStackSize (task) : 0;
}
