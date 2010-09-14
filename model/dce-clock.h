#ifndef SIMU_CLOCK_H
#define SIMU_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#define SIMU_CLOCKS_PER_SEC (1000000)

typedef int dce_clock_t;

dce_clock_t dce_clock (void);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_CLOCK_H */
