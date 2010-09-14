#ifndef SIMU_RANDOM_H
#define SIMU_RANDOM_H


#ifdef __cplusplus
extern "C" {
#endif

long int dce_random (void);
int dce_rand (void);
void dce_srandom (unsigned int seed);
void dce_srand (unsigned int seed);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_RANDOM_H */
