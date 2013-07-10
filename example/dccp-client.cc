#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>

#define SOL_DCCP 269

int main (int argc, char *argv[])
{
  int sock;
  sock = socket (PF_INET, SOCK_DCCP, IPPROTO_DCCP);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (2000);

  struct hostent *host = gethostbyname (argv[1]);
  memcpy (&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

  int result;
  result = connect (sock, (const struct sockaddr *) &addr, sizeof (addr));
  if (result < 0)
    {
      printf ("connect errno=%d\n", errno);
    }

  uint8_t buf[1024];

  memset (buf, 0x66, 20);
  memset (buf + 20, 0x67, 1004);
  ssize_t tot = 0;
  fd_set w_fds;
  FD_ZERO (&w_fds);
  FD_SET (sock, &w_fds);

  for (uint32_t i = 0; i < 1000; i++)
    {
      ssize_t n = 1024;
      while (n > 0)
        {
          result = select (sock + 1, NULL, &w_fds, NULL, NULL);
          if (result == 0)
            {
              std::cout << "timeout" << std::endl;
              continue;
            }
          if (result < 0)
            {
              if (errno == EINTR || errno == EAGAIN)
                {
                  std::cout << "result < 0: " << errno << std::endl;
                  continue;
                }
              perror ("select");
              break;
            }
          if (!FD_ISSET (sock, &w_fds))
            {
              std::cout << "fd isn't set" << std::endl;
              continue;
            }

          ssize_t e  = write (sock, &(buf[1024 - n]), n);
          if (e < 0)
            {
              std::cout << "e < 0 : " << strerror (errno) << std::endl;
              break;
            }
          if (e < n)
            {
              //  sleep (1);
              std::cout << "e < n : " << e << "<" << n << std::endl;
            }
          n -= e;
          tot += e;
        }

      //    std::cout << "write: " << n << std::endl;
      usleep (100);
    }

  std::cout << "did write all buffers total:" << tot << std::endl;

  //  close (sock);

  return 0;
}
