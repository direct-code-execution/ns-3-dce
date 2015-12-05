#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <errno.h>

int main (int argc, char *argv[])
{
  if (argc != 3)
    {
      std::cout << "Usage:" << std::endl;
      std::cout << argv[0] << " host message" << std::endl;
      return 0;
    }
  sleep (2);

  int sock;
  sock = socket (PF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (2000);

  struct hostent *host = gethostbyname (argv[1]);
  memcpy (&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

  size_t bLen = 1 + strlen (argv[2]);
  char *buffer = (char *)malloc (bLen);

  memset (buffer, 0, bLen*sizeof (char));

  socklen_t aLen = sizeof(addr);

//  while (true)
  {
    ssize_t wLen = sendto (sock, argv[2], bLen, 0, (struct sockaddr *)&addr, aLen);

    std::cout << "len:" << wLen << "errno:" << errno << std::endl;

    if (wLen > 0)
      {
        memset (buffer, 0, bLen);
        ssize_t n = read (sock, buffer, bLen);

        if (n > 0)
          {
            std::cout << "echo:" << buffer << std::endl;
          }
      }
  }
  close (sock);

  return 0;
}
