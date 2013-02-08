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
#include <sys/stat.h>
#include <sys/types.h>

#define SOCK_PATH "/tmp/socket"
#define BUF_LEN 256

int
main (int argc, char *argv[])
{
  int sock = -1;
  int sockin = -1;
  bool use_select = 0;

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

      struct sockaddr_un address;

      memset (&address, 0, sizeof(address));

      address.sun_family = AF_UNIX;
      strcpy (address.sun_path, SOCK_PATH);

      int status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));

      if (status < 0)
        {
          perror ("bind() failed");
          break;
        }

      std::cout << "before listen " << std::endl;

      status = listen (sock, 1);

      std::cout << "after listen " << std::endl;

      if (status < 0)
        {
          std::cout << "Errno:" << errno << std::endl;
          perror ("listen() failed");
          break;
        }

      if (use_select)
        {
          fd_set lect;
          struct timeval timeOut;

          FD_ZERO (&lect);
          FD_SET (sock, &lect);

          timeOut.tv_sec = 10;
          timeOut.tv_usec = 0;

          std::cout << "before select " << std::endl;

          int ret = select (1 + sock, &lect, 0, 0, &timeOut);

          if (ret != 1)
            {
              perror ("select() failed.");
              break;
            }
        }

      std::cout << "Server: " << time (0)  << " sleeping 15s before accepting ! " << std::endl;

      sleep (15);

      std::cout << "before accept " << std::endl;

      sockin = accept (sock, NULL, NULL);

      std::cout <<  "Server: " << time (0)  << " Unix-Server: after accept : " << sockin << std::endl;

      if (sockin < 0)
        {
          perror ("accept() failed");
          break;
        }

      lg = BUF_LEN;
      status = 0; // setsockopt (sockin, SOL_SOCKET, SO_RCVLOWAT, (char *) &lg, sizeof(lg));

      if (status < 0)
        {
          perror ("setsockopt(SO_RCVLOWAT) failed");
          break;
        }

      std::cout << "after accept before recv" << std::endl;

      status = recv (sockin, buffer, sizeof(buffer), 0);

      std::cout << "Server: recv -> " << status << std::endl;

      if (status < 0)
        {
          perror ("recv() failed");
          break;
        }

      if (status < ((int) sizeof(buffer)))
        {
          std::cout
              << "The client closed the connection before all of the data was sent"
              << std::endl;
          break;
        }

      std::cout << "Server: about to send" << std::endl;

      status = send (sockin, buffer, sizeof(buffer), 0);

      std::cout << "Server: send->" << status << std::endl;

      if (status < 0)
        {
          perror ("Server: send() failed");
          break;
        }
      else
        {
          std::cout << "Server: ALL is right" << std::endl;
          sleep (50);
        }
    }
  while (0);

  if (sock != -1)
    {
      close (sock);
    }

  if (sockin != -1)
    {
      close (sockin);
    }

  unlink (SOCK_PATH);

  std::cout << "server end." << std::endl;

  return 0;
}
