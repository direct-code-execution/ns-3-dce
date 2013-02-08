#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <iostream>


int main (int argc, char *argv[])
{
  sleep (2);

  int sock;
  sock = socket (PF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons (2000);

  struct hostent *host = gethostbyname (argv[1]);
  memcpy (&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

  int result;
  result = connect (sock, (const struct sockaddr *) &addr, sizeof (addr));

  uint8_t buf[10240];

  memset (buf, 0x66, 20);
  memset (buf + 20, 0x67, 1004);

  for (uint32_t i = 0; i < 1000; i++)
    {
      ssize_t n;
      n = write (sock, buf, 1024);
      sleep (1);
    }

  std::cout << "did write all buffers" << std::endl;

  close (sock);

  return 0;
}
