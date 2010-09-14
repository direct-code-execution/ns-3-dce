#ifndef SIMU_CLOCK_H
#define SIMU_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#define SIMU_CLOCKS_PER_SEC (1000000)

typedef int simu_clock_t;

simu_clock_t simu_clock (void);


#ifdef __cplusplus
}
#endif

#endif /* SIMU_CLOCK_H */
