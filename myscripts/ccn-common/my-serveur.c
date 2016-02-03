/*******************************************************************************/
/* TCP server , transmit file to multiple client using TCP one thread and poll */
/*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>

struct a_client
{
  int sockfd;
  int writeLen;
};
int totalClients = 0;
// Return ms.
long duration (struct timeval *st, struct timeval *ed)
{
  return ( ((ed->tv_sec * 1000000) + ed->tv_usec) - ((st->tv_sec * 1000000) + st->tv_usec)) / 1000 ;
}
void NewClient (int newFd, struct a_client **theClients, int *nbClient, int *maxClientFd)
{
  if (newFd > *maxClientFd)
    {
      int oldSize = *maxClientFd;
      *theClients = realloc (*theClients, sizeof (struct a_client) * (newFd + 1));
      *maxClientFd = newFd;
      int i=0;

      for (i=(1 + oldSize); i <= newFd; i++)
        {
          (*theClients)[i].sockfd = -1;
          (*theClients)[i].writeLen = 0;
        }
    }
  (*theClients)[newFd].sockfd = newFd;
  (*theClients)[newFd].writeLen = 0;
  *nbClient = 1 + (*nbClient);
}
void CloseClient (struct a_client *theClients, int fd, int *nbClient, long dur,long dataLen )
{
  if (theClients[fd].sockfd == fd )
    {
      close (fd);
      theClients[fd].sockfd = -1;
      *nbClient = (*nbClient) - 1;
      if (theClients[fd].writeLen >= dataLen )
        {
          FILE *f = fopen("/var/log/duration.txt","w");
          fprintf (f, "%ld\n", dur);
          fclose (f);
        }
      totalClients++;
    }
}
void initBuffer (void **buf, int *buflen)
{
  *buflen = 10*1024;
//  *buflen = 1*102;
  *buf = malloc (*buflen);
  memset (*buf, 'A', *buflen);
}
void WriteClient (struct a_client *theClients, int fd, int *nbClient, long dataLen, void *buf, int buflen, long dur)
{
  if (theClients[fd].writeLen >= dataLen)
    {
      CloseClient (theClients, fd, nbClient, dur, dataLen);
    }
  else
    {
      int len = buflen;

      if  (len > (dataLen - theClients[fd].writeLen))
        {
          len = (dataLen - theClients[fd].writeLen);
        }
      if (len > 0)
        {
          if (0 == fd)
            {
              int a = 100 / fd;
            }

          int e = write (fd, buf, len);

          printf ("Write -> %d\n",e);

          if (e > 0)
            {
              theClients[fd].writeLen += e;
/*              if (theClients[fd].writeLen >= dataLen)
                {
                  CloseClient (theClients, fd, nbClient, duration);
                } */
            }
          if (e < 0)
            {
              CloseClient (theClients, fd, nbClient, dur, dataLen);
            }

        }
    }
}

int main (int argc, char **argv)
{
  int listenPort = 5002;
  int listenFd;
  struct a_client *theClients=0;
  int nbClient=0;
  struct pollfd *fds=0;
  int maxClientFd=-1;
  long dataLen = 6 *1024 * 1024;
  long stopAfter = 0;
  void *buf = 0;
  int buflen = 0;
  struct timeval startTime;
  struct timeval endTime;


  initBuffer (&buf, &buflen);
  listenFd = socket (AF_INET, SOCK_STREAM, 0);

  if (listenFd < 0)
    {
      fprintf (stderr, "Unable to open socket ? errno %d\n",errno);
      exit (1);
    }
  if (argc >= 2)
    {
      listenPort = atoi (argv[1]);
    }
  if (argc >= 3)
    {
      dataLen= atoi (argv[2]);
    }
  if (argc >= 4)
    {
      stopAfter= atoi (argv[3]);
    }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (listenPort);
  addr.sin_addr.s_addr = INADDR_ANY;

  int status;
  status = bind (listenFd, (const struct sockaddr *) &addr, sizeof(addr));
  if (status)
    {
      fprintf (stderr, "Unable to open bind ? errno %d\n",errno);
      exit (2);
    }
  status = listen (listenFd, 256);
  if (status)
    {
      fprintf (stderr, "Unable to open listen ? errno %d \n",errno);
      exit (3);
    }

  while (1)
    {
      fds = realloc (fds, sizeof (struct pollfd) *  nbClient + 1 );

      fds[0].fd = listenFd;
      fds[0].events = POLLIN;
      fds[0].revents = 0;

      int j = 1;
      int c =0;
      for (c=0; c <= maxClientFd ; c++)
        {
          if (theClients[c].sockfd >= 0)
            {
              if (theClients[c].writeLen < dataLen)

                {
                  fds[j].fd = theClients[c].sockfd;
                  fds[j].events = POLLOUT;
                  fds[j].revents = 0;
                  j++;
                }
              else
                {
                  fds[j].fd = theClients[c].sockfd;
                  fds[j].events = POLLIN;
                  fds[j].revents = 0;
                  j++;
                }
            }
        }
      printf ("%lu: --> poll j=%d\n",time(NULL), j );
      int ret = poll (fds, j, 29000 * 1000);
      printf ("%lu: poll --> %d\n", time(NULL), ret);

      if (ret <= 0)
        {
          exit (0);
        }
      if (ret > 0)
        {
          int i=0;
          for (i=0; i < j; i++)
            {
              if ( fds[i].revents )
                {
                  if (i == 0)
                    { // a new cnxion
                      if ( fds[i].revents & POLLIN  )
                        {
                          int newFd = accept (listenFd, 0, 0);

                          if (newFd >= 0)
                            {
                              if ( 0 == nbClient)
                                {
                                  gettimeofday (&startTime, 0);
                                }
                              NewClient (newFd, &theClients, &nbClient, &maxClientFd);
                            }
                        }
                      if ( fds[i].revents & POLLERR  )
                        {
                          exit (4);
                        }
                    }
                  else
                    { // a client

                      printf ("Client %d revents %d \n", i, fds[i].revents );
                      gettimeofday (&endTime, 0);

                      if ( fds[i].revents & POLLOUT  )
                        {
                          WriteClient (theClients, fds[i].fd, &nbClient, dataLen, buf, buflen, duration (&startTime, &endTime));
                        }
                      if ( ( fds[i].revents & POLLHUP  ) || ( fds[i].revents & POLLIN  )  || ( fds[i].revents & POLLERR  ) )
                        {
                          gettimeofday (&endTime, 0);
                          CloseClient (theClients, fds[i].fd, &nbClient, duration (&startTime, &endTime), dataLen);
                        }
                      if ( stopAfter && ( totalClients >=  stopAfter ))
                        {
                          exit (0);
                        }
                    }
                }
            }
        }
    }
}
