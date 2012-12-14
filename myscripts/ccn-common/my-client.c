/************************************************************************************************/
/* 1 Connect
 * 2 read until end
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

int main (int argc, char *argv[])
{
  int sock=-1;
  int port = 5000;
  char *address = "10.1.1.1";
  long dataLen = 6 *1024 * 1024;
  time_t dep = time(NULL);

  if (argc >= 2)
    {
      address = argv [1];
    }
  if (argc >= 3)
    {
      port = atoi (argv[2]);
    }
  if (argc >= 4)
    {
      dataLen= atoi (argv[3]);
    }

  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      fprintf (stderr, "Unable to open socket ? errno %d\n",errno);
      exit (1);
    }
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);

  struct hostent *host = gethostbyname (address);
  if (!host)
    {
      fprintf (stderr, "gethostbyname error. errno %d\n",errno);
      exit (2);
    }
  memcpy (&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

  int result;
  result = connect (sock, (const struct sockaddr *) &addr, sizeof (addr));
  printf ("connect -> %d\n", result);
  if (result)
    {
      fprintf (stderr, "connect error. errno %d\n",errno);
      exit (3);
    }

  uint8_t buf[1024];
  long l = 0;

  while (sock >=0)
    {
      int ret = read (sock, buf, sizeof(buf));
      printf ("T:%ld read --> %d\n", (time(NULL)-dep),    ret);
      if (ret > 0)
        {
          l += ret;
//          printf ("T:%ld l %ld dataLen %ld\n", (time(NULL)-dep),l, dataLen);
          if (l >= dataLen)
            {
              close (sock);
              sock = -1;
            }
        }
      else
        {
          close (sock);
          sock = -1;
        }
    }
  printf ("T:%ld Total size %ld\n", (time(NULL)-dep),l);
  exit (0);
}

