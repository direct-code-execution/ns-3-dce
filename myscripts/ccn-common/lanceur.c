#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <wait.h>

// Return ms.
long duration (struct timeval *st, struct timeval *ed)
{
  return ( ((ed->tv_sec * 1000000) + ed->tv_usec) - ((st->tv_sec * 1000000) + st->tv_usec)) / 1000 ;
}


/* My goal is to launch a CCN transfert and verify success or failure and calculate the time taken */
int main (int argc, char **argv)
{
  struct timeval startTime;

  gettimeofday (&startTime, 0);
  pid_t fils = fork ();

  if ( -1 == fils )
    {
      fprintf (stderr,"fork failed !\n");
      exit (1);
    }
  if (fils)
    {
      // I am the father
      int status = 0;
      pid_t ret = waitpid (fils, &status, 0);
      struct timeval endTime;

      gettimeofday (&endTime, 0);
      if ( ret < 0 )
        {
          printf ("waitpid FAILURE\n");
          exit (3);
        }

      if (ret == fils)
        {
          if (WIFEXITED (status))
            {
              printf ("SUCCESS Exit Code %d ,Duration %ld\n", WEXITSTATUS(status), duration (&startTime, &endTime) );
              if ( WEXITSTATUS(status) )
                {
                  FILE *f  = fopen ("/var/log/FAILURE.TXT", "w+");
                  fprintf (f, "Failure PID %d, exit %d, duration %ld\n", fils, WEXITSTATUS(status), duration (&startTime, &endTime));
                  fclose (f);
                }
              else
                {
                  FILE *f  = fopen ("/var/log/SUCCESS.TXT", "w+");
                  fprintf (f, "%ld\n", duration (&startTime, &endTime));
                  fclose (f);
                }
              exit (WEXITSTATUS(status));
            }
          if (WIFSIGNALED (status))
            {
              printf ("SIGNALED : %d \n",  WTERMSIG(status));
              exit  (5);
            }
        }
      else
        {
          printf ("waitpid FAILURE\n");
          exit (4);
        }
      exit (ret);
    }
  else
    {
        int ret = execlp ("ccncatchunks2", "ccncatchunks2", "-d", "-a", "ccnx:/DATAFILE", NULL);
        printf ("exec return %d errno %d\n",ret , errno);
        exit (2);
    }
  exit (0);
}
