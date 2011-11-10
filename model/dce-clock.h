#ifndef DCE_CLOCK_H
#define DCE_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#define DCE_CLOCKS_PER_SEC (1000000)

typedef int dce_clock_t;

dce_clock_t dce_clock (void);


#ifdef __cplusplus
}
#endif

#endif /* DCE_CLOCK_H */
