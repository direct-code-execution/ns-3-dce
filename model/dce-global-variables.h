#ifndef SIMU_GLOBAL_VARIABLES_H
#define SIMU_GLOBAL_VARIABLES_H

#ifdef __cplusplus
extern "C" {
#endif


struct SimuGlobalVariables
{
  FILE **pstdin;
  FILE **pstdout;
  FILE **pstderr;
  char ***penvp;
  char **poptarg;
  int *poptind;
  int *popterr;
  int *poptopt;
};

void dce_global_variables_setup (struct SimuGlobalVariables *variables);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_GLOBAL_VARIABLES_H */
