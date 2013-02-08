#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <stdlib.h>


int main (int argc, char *argv[])
{
  std::cout << "Server: starting" << std::endl;
  int sock;
  sock = socket (PF_INET, SOCK_DGRAM, 0);
  if (sock == -1)
    {
      std::cout << "error: " << strerror (errno) << std::endl;
      exit (-1);
    }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (2000);
  addr.sin_addr.s_addr = INADDR_ANY;

  int status;
  status = bind (sock, (const struct sockaddr *) &addr, sizeof (addr));
  if (status == -1)
    {
      std::cout << "error: " << strerror (errno) << std::endl;
      exit (-1);
    }

  uint8_t buf[10240];

  for (uint32_t i = 0; i < 1000; i++)
    {
      ssize_t n;
      n = read (sock, buf, 1024);
      if (n == -1 && errno == EINTR)
        {
          std::cout << "timer interrupted read" << std::endl;
          exit (-1);
        }
    }

  std::cout << "did read all buffers" << std::endl;

  close (sock);

  sleep (10);

  return 0;
}
