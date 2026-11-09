#ifndef SIMU_UIO_H
#define SIMU_UIO_H

#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

int dce_tcgetattr (int fd, struct termios *termios_p);
int dce_tcsetattr (int fd, int optional_actions, const struct termios *termios_p);

#ifdef __cplusplus
}
#endif


#endif /* SIMU_UIO_H */
