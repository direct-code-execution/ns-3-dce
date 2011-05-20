#ifndef SIMU_RANDOM_H
#define SIMU_RANDOM_H


#ifdef __cplusplus
extern "C" {
#endif

long int dce_random (void);
int dce_rand (void);
void dce_srandom (unsigned int seed);
void dce_srand (unsigned int seed);
unsigned short int *dce_seed48 (unsigned short int seed16v[3]);
double dce_drand48 (void);
long int dce_nrand48 (unsigned short int xsubi[3]);
long int dce_lrand48 (void);
long int dce_mrand48 (void);
double dce_erand48 (unsigned short xsubi[3]);
long int dce_jrand48 (unsigned short int xsubi[3]);
void dce_srand48 (long int seedval);
void dce_lcong48 (unsigned short param[7]);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_RANDOM_H */
