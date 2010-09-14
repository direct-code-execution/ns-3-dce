#include "simu-global-variables.h"
#include "process.h"
#include "utils.h"
#include "simu-stdio.h"

using namespace ns3;

void simu_global_variables_setup (struct SimuGlobalVariables *variables)
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
  *process->pstdin = simu_fdopen (0, "r");
  *process->pstdout = simu_fdopen (1, "w");
  *process->pstderr = simu_fdopen (2, "w");
  *process->penvp = process->originalEnvp;
}

