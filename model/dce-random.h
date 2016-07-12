#ifndef SIMU_RANDOM_H
#define SIMU_RANDOM_H


//#ifdef __cplusplus
//extern "C" {
//#endif
#include "dce-guard.h"

DCE(long int, random, (void));
DCE(int, rand, (void));
DCE(void, srandom, (unsigned int seed));
DCE(void, srand, (unsigned int seed));
DCE(unsigned short int *, seed48, (unsigned short int seed16v[3]));
DCE(double, drand48, (void));
DCE(long int, nrand48, (unsigned short int xsubi[3]));
DCE(long int, lrand48, (void));
DCE(long int, mrand48, (void));
DCE(double, erand48, (unsigned short xsubi[3]));
DCE(long int, jrand48, (unsigned short int xsubi[3]));
DCE(void, srand48, (long int seedval));
DCE(void, lcong48, (unsigned short param[7]));

//#ifdef __cplusplus
//}
//#endif

#endif /* SIMU_RANDOM_H */
