#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define SND_CNT 50

static int
sv ()
{
  int ret;
  int accept_sock, sock;
  struct sockaddr_in addr, client;
  fd_set readfd;
  int val;
  socklen_t len = sizeof (client);
  char buf[1024];
  int i = 0;

#ifdef DEBUG
  printf ("Server Start \n");
#endif

  accept_sock = socket (AF_INET, SOCK_STREAM, 0);
  if (accept_sock < 0)
    {
      perror ("socket");
      return -1;
    }

  memset (&addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (10001);
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  ret  = bind (accept_sock, (struct sockaddr *)&addr,
               sizeof (struct sockaddr_in));
  if (ret < 0)
    {
      perror ("bind");
      return -1;
    }

  ret = listen (accept_sock, 1);
  if (ret < 0)
    {
      perror ("listen");
      return -1;
    }

  sock = accept (accept_sock, (struct sockaddr *) &client, &len);
  if (sock < 0)
    {
      perror ("accept");
      return -1;
    }

#ifdef DEBUG
  printf ("accept\n");
#endif
  val = fcntl (sock, F_GETFL, 0);
  fcntl (sock, F_SETFL, (val | O_NONBLOCK));

  while (1)
    {
      struct timeval tv;
      memset (&tv, 0, sizeof(tv));
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      i++;
      if (i > SND_CNT * 3)
        {
          printf ("ERR: Giving up to recv data (try=%d)", i);
          return -1;
        }

      //      printf ("select\n");
      FD_ZERO (&readfd);
      FD_SET (sock, &readfd);
      // Multiple fds in readfd
      FD_SET (accept_sock, &readfd);
      ret = select (FD_SETSIZE, &readfd, NULL, NULL, &tv);
      if (ret == 0)
        {
#ifdef DEBUG
          printf ("timeout\n");
#endif
          continue;
        }

      if (ret < 0)
        {
          if (errno == EINTR || errno == EAGAIN)
            {
              continue;
            }

          perror ("select");
          return -1;
        }

      // To increase read buffer
      if (i % 3 == 0)
        {
          i++;
          continue;
        }

      ret = read (sock, buf, sizeof(buf));
      if (ret == 0)
        {
          return 0;
        }
      else if (ret < 0)
        {
          perror ("read, exit");
          return -1;
        }
#ifdef DEBUG
      printf ("recv[%d]: %s\n", i, buf);
#endif
    }


#ifdef DEBUG
  printf ("Server Stop \n");
#endif
  return 0;
}


static int
cl ()
{
  int ret;
  int sock;
  struct sockaddr_in addr;
  const char buf[512] = "01234567890123456789012345678901234567890123456789\0"; // 50 chars
  int i = 0, sndcnt = SND_CNT;

#ifdef DEBUG
  printf ("Client Start \n");
#endif

  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      return -1;
    }

  memset (&addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (10001);
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  ret  = connect (sock, (struct sockaddr *)&addr,
                  sizeof (struct sockaddr_in));
  if (ret < 0)
    {
      perror ("connect");
      return -1;
    }

  while (i < sndcnt)
    {
      ret = write (sock, buf, sizeof(buf));
      if (ret < 0)
        {
          perror ("write");
          return -1;
        }
      //      printf ("write [%d] \n", i);
      i++;
    }


#ifdef DEBUG
  printf ("Client Stop \n");
#endif
  return 0;
}

int
main (int argc, char *argv[])
{
  if (argc == 1)
    {
      return sv ();
    }
  else
    {
      return cl ();
    }

  return 0;
}
