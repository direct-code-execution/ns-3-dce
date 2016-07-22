#ifndef DCE_CLOCK_H
#define DCE_CLOCK_H

#include <time.h>
#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
define as extern const ? where is it used
*/
#define DCE_CLOCKS_PER_SEC (1000000)

typedef int dce_clock_t;

// Not implemented yet
//DCE(clock_t, clock);


#ifdef __cplusplus
}
#endif

#endif /* DCE_CLOCK_H */
