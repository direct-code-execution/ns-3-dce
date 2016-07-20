#ifndef SIMU_UIO_H
#define SIMU_UIO_H

#include <termio.h>
#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(int, tcgetattr, int fd, struct termios *termios_p);
DCE(int, tcsetattr, int fd, int optional_actions, const struct termios *termios_p);

#ifdef __cplusplus
}
#endif


#endif /* SIMU_UIO_H */
