#ifndef SIMU_CXA_H
#define SIMU_CXA_H

#ifdef __cplusplus
extern "C" {
#endif

int simu__cxa_atexit (void (*func) (void *), void *arg, void *d);
void simu__cxa_finalize (void *d);

#ifdef __cplusplus
}
#endif

#endif /* SIMU_CXA_H */
