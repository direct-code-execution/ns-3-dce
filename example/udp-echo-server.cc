#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[])
{
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

  if (argc > 1)
    {
      struct hostent *host = gethostbyname (argv[1]);
      memcpy (&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
    }

  int status;
  status = bind (sock, (const struct sockaddr *) &addr, sizeof (addr));
  if (status == -1)
    {
      std::cout << "error: " << strerror (errno) << std::endl;
      exit (-1);
    }

//  while (true)
  for (int i = 0; i < 1 ; i++)
    {
      struct sockaddr_in remoteAddr;
      socklen_t len = sizeof(remoteAddr);
      char buf[1024];

      memset (&remoteAddr, 0, len);
      memset (buf, 0, sizeof(buf));

      ssize_t rLen = recvfrom (sock, buf, sizeof(buf), 0, (struct sockaddr *)&remoteAddr, &len);

      std::cout << "recvfrom len:" << rLen << std::endl;

      if (rLen <= 0)
        {
          break;
        }

      ssize_t wLen = sendto (sock, buf, (size_t) rLen, 0, (struct sockaddr *)&remoteAddr, len);

      if (wLen <= 0)
        {
          break;
        }

      std::cout << "Echo sended, len:" << wLen << " over " << rLen << std::endl;
    }

  close (sock);

  return 0;
}
