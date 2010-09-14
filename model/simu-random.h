#ifndef SIMU_RANDOM_H
#define SIMU_RANDOM_H


#ifdef __cplusplus
extern "C" {
#endif

long int simu_random (void);
int simu_rand (void);
void simu_srandom (unsigned int seed);
void simu_srand (unsigned int seed);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_RANDOM_H */
