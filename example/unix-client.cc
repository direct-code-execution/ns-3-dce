#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/socket"
#define BUF_LEN 128 * 1024 + 255

int
main (int argc, char *argv[])
{
  struct sockaddr_un address;
  int status = 0;
  int sock = -1;
  char buffer[BUF_LEN];
  int lg = 0;

  do
    {
      sock = socket (AF_UNIX, SOCK_STREAM, 0);
      if (sock < 0)
        {
          perror ("socket() failed");
          break;
        }

      memset (&address, 0, sizeof(address));

      address.sun_family = AF_UNIX;
      strcpy (address.sun_path, SOCK_PATH);

      std::cout << "Client: " << time (0)  << " Before the Connect" << std::endl;

      status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));

      if (status < 0)
        {
          std::cout << "Client: " << time (0)  << " connect (timeout ?)" << std::endl;
          perror ("connect() failed");
          break;
        }

      std::cout << "Client: " << time (0)  << " Connected ! " << std::endl;

      memset (buffer, 'F', sizeof(buffer));

      std::cout << "Client: About to send ... " << std::endl;

      status = send (sock, buffer, sizeof(buffer), 0);

      std::cout << "Server: send -> " << status << std::endl;

      if (status < 0)
        {
          perror ("Client: send() failed");
          break;
        }

      std::cout << "Client: send done:" << status << std::endl;

      lg = 0;
      while (lg < BUF_LEN)
        {
          status = recv (sock, &buffer[lg], BUF_LEN - lg, 0);
          if (status < 0)
            {
              perror ("recv() failed");
              break;
            }
          else if (status == 0)
            {
              std::cout << "The server closed the connection" << std::endl;
              break;
            }
          lg += status;
        }
      std::cout << "read " << lg << " byte(s)" << std::endl;
    }
  while (0);

  if (sock != -1)
    {
      close (sock);
    }

  std::cout << "client end." << std::endl;

  return 0;
}
