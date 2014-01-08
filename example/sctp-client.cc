// 
// libstcp1-dev is needed
// 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>        /* for memset */
#include <unistd.h>        /* for memset */
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

int
main (int argc, char **argv)
{
  int connect_sock, stat, port, slen, i, flags;
  struct sctp_initmsg initmsg;
  struct sockaddr_in server_addr;
  struct sctp_event_subscribe s_events;
  struct sctp_status s_status;
  struct sctp_sndrcvinfo s_sndrcvinfo;
  char buffer[1024];

  port = 3007;

  connect_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP);
  memset (&initmsg, 0, sizeof (initmsg));
  initmsg.sinit_num_ostreams = 3;          // Number of Output Stream
  initmsg.sinit_max_instreams = 3;      // Number of Input Stream
  initmsg.sinit_max_attempts = 4;
  stat = setsockopt (connect_sock, IPPROTO_SCTP, SCTP_INITMSG,
                     &initmsg, sizeof (initmsg) );
  if (stat < 0)
    {
      perror ("setsockopt error");
      exit (-1);
    }

  memset (&server_addr, 0, sizeof (server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons (port);
  server_addr.sin_addr.s_addr = inet_addr (argv[1]);

  stat = connect (connect_sock, (struct sockaddr *)&server_addr, 
                  sizeof (server_addr) );
  if (stat < 0)
    {
      perror ("connect error");
      exit (-1);
    }

  memset (&s_events, 0, sizeof (s_events));
  s_events.sctp_data_io_event = 1;
  stat = setsockopt (connect_sock, SOL_SCTP, SCTP_EVENTS,
                     (const void *)&s_events, sizeof (s_events));
  if (stat < 0)
    {
      perror ("event error");
      exit (-1);
    }

  slen = sizeof (s_status);
  stat = getsockopt (connect_sock, SOL_SCTP, SCTP_STATUS,
                     (void *)&s_status, (socklen_t *)&slen);

  printf ("assoc id  = %d\n", s_status.sstat_assoc_id );
  printf ("state     = %d\n", s_status.sstat_state );
  printf ("instrms   = %d\n", s_status.sstat_instrms );
  printf ("outstrms  = %d\n", s_status.sstat_outstrms );


  for (i = 0 ; i < 100 ; i++)
    {
      stat = sctp_recvmsg (connect_sock, (void *)buffer, sizeof (buffer),
                           (struct sockaddr *)NULL, 0, &s_sndrcvinfo, &flags);
      printf ("stat = %d\n", stat);
      if (stat > 0)
        {
          buffer[stat] = 0;
          printf ("(Stream %d) %s\n", s_sndrcvinfo.sinfo_stream, buffer);
        }
    }
  /* Close our socket and exit */
  close (connect_sock);
  return 0;
}


