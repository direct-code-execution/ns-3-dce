#include "dce-global-variables.h"
#include "process.h"
#include "utils.h"
#include "dce-stdio.h"

using namespace ns3;

void dce_global_variables_setup (struct SimuGlobalVariables *variables)
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
  *process->pstdout = dce_fdopen (1, "a");
  *process->pstderr = dce_fdopen (2, "a");
  *process->penvp = process->originalEnvp;
}

