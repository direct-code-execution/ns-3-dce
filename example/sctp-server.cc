// 
// libstcp1-dev is needed
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

void
echo_main (int sock)
{
  int stat;
  char buffer[1024];

  printf ("sock: %d\n", sock);

  // Stream No.0
  sprintf (buffer, "This is a test of stream 0");
  stat = sctp_sendmsg (sock, buffer, (size_t)strlen (buffer),
                       NULL, 0, 0, 0, 0, 0, 0);

  // Stream No.1
  sprintf (buffer, "This is a test of stream 1");
  stat = sctp_sendmsg (sock, buffer, (size_t)strlen (buffer),
                       NULL, 0, 0, 0, 1, 0, 0);
}

int
main (int argc, char **argv)
{
  int sock_listen, sock_server, stat;
  struct sockaddr_in server_addr;
  struct sctp_initmsg s_initmsg;
  int echo_port;
  int i = 0;

  echo_port = 3007;

  sock_listen = socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP);

  memset (&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
  server_addr.sin_port = htons (echo_port);

  stat = bind (sock_listen, (struct sockaddr *)&server_addr, sizeof(server_addr));

  // SCTP parameter
  memset (&s_initmsg, 0, sizeof(s_initmsg));
  s_initmsg.sinit_num_ostreams = 5;
  s_initmsg.sinit_max_instreams = 5;
  s_initmsg.sinit_max_attempts = 5;

  stat = setsockopt (sock_listen, IPPROTO_SCTP, SCTP_INITMSG,
                     &s_initmsg, sizeof(s_initmsg));
  if (stat < 0)
    {
      perror ("Socket Option error");
      exit (-1);
    }

  listen (sock_listen, 5);
  while (1)
    {
      printf ("SCTP server accepting\n");
      sock_server = accept (sock_listen, (struct sockaddr *)NULL, (socklen_t *)NULL);
      if (sock_server == -1)
        {
          perror ("accept");
          exit (-1);
        }

      for (i = 0 ; i < 100 ; i++)
        {
          echo_main (sock_server);
        }
    }

  close (sock_listen);
  return 0;
}

