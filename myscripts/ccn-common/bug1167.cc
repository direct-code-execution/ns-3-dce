#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "test-macros.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <vector>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define BUFF_LEN ((size_t) 1000001)

static char *sendBuffer = 0;
static char *readBuffer = 0;


#define TEST3_LEN BUFF_LEN

static
void fill_addr (struct sockaddr_in &addr, int port, char *address)
{
  char *ad = (char*)"127.0.0.1";

  if (0 != address)
    {
      ad = address;
    }
  int res = inet_aton (ad, &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
}

// TEST 3 : write timeout
// Server: fill the socket until write timeout, then close the socket
// Client: read socket slowly only every 10s in order to server to fill the buffers,
//         the close when then socket will be closed.
static void *
client3 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;
  size_t tot = 0;

  sleep (1);
  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  fill_addr (ad, 1236, (char*)arg);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status, 0);

  tot = 0;
  do
    {
      printf ("Client3: before sleep time %ld \n ",time (0));
      // Slow read until remote closure
      sleep (10);
      printf ("Client3: after sleep time %ld \n ",time (0));
      status = recv (sock, readBuffer, TEST3_LEN, 0);
      printf ("Client3: recv -> %d \n ", status);
      fflush (stdout);
    }
  while (status > 0);
  // printf("Client3: after recvs status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client3: end\n ");

  return arg;
}

static void *
server3 (void *arg)
{
  int status;
  int sock = -1;
  struct sockaddr_in ad;
  int sockin = -1;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  fill_addr (ad, 1236, (char*)arg);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  char *crsr = sendBuffer;
  for (size_t i = 0; i < TEST3_LEN; i++)
    {
      *crsr++ = i & 0xff;
    }

  struct timeval tiout;

  tiout.tv_sec = 3;
  tiout.tv_usec = 42;
  status = setsockopt (sockin, SOL_SOCKET, SO_SNDTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  do
    {
      status = send (sockin, sendBuffer, TEST3_LEN, 0);
      printf ("Server3: send -> %d, errno: %d \n ",status, errno);
    }
  while (status > 0);
  TEST_ASSERT ( (status == -1) && (errno == EAGAIN) );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server3: end\n ");

  return arg;
}

static void
launch (void *
        (*clientStart)(void *), void *
        (*serverStart)(void *))
{
  int status;
  pthread_t theClient;
  pthread_t theServer;

  printf ("launch\n\n ");

  status = pthread_create (&theServer, NULL, serverStart, 0);
  TEST_ASSERT_EQUAL (status, 0);

  status = pthread_create (&theClient, NULL, clientStart, 0);
  TEST_ASSERT_EQUAL (status, 0);

  void *threadResult = 0;

  status = pthread_join (theClient, &threadResult);
  TEST_ASSERT_EQUAL (status, 0);

  status = pthread_join (theServer, &threadResult);
  TEST_ASSERT_EQUAL (status, 0);

  fflush (stdout);
  fflush (stderr);
  sleep (1); // TEMPOFUR if removed can crash :(
}

int
main (int argc, char *argv[])
{
  signal (SIGPIPE, SIG_IGN);

  readBuffer = (char *)malloc (BUFF_LEN);
  sendBuffer = (char *)malloc (BUFF_LEN);

  if (argc > 1)
    {
      if (0 == strcasecmp (argv[1], "-s"))
        {
          server3 ((void*)argv[2]);
          return 0;
        }
      if (0 == strcasecmp (argv[1], "-c"))
        {
          client3 ((void*)argv[2]);
          return 0;
        }
    }

  launch (client3, server3);

  return 0;
}
