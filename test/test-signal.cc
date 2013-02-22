#include <stdio.h>
#include <stdlib.h>
#include "test-macros.h"
#include <sys/time.h>
#include <errno.h>
#include <signal.h>

int
main (int argc, char *argv[])
{
  sigset_t sigcatch;
  int signum;
  sigemptyset (&sigcatch);
  sigaddset (&sigcatch, SIGALRM);

  {
    struct itimerval it;
    memset (&it, 0, sizeof (it));
    it.it_value.tv_sec = 1;
    it.it_value.tv_usec = 500000;


    setitimer (ITIMER_REAL, &it, NULL);
    while (1)
      {
        sigwait (&sigcatch, &signum);
        switch (signum)
          {
          case SIGALRM:
            printf ("SIG rcvd\n");
            return 0;
            break;
          default:
            break;
          }
        break;
      }
  }
  // never reached
  TEST_ASSERT (0);
  return 0;
}
