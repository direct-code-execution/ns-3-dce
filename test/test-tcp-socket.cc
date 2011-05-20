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

static
void fill_addr (struct sockaddr_in &addr, int port)
{
  int res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL ( res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
}

// TEST 1 : simple ping pong of data
static void *
client1 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;
  size_t tot = 0;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1234);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL ( status, 0);

  char *crsr = sendBuffer;
  for (size_t i = 0; i < BUFF_LEN; i++)
    {
      *crsr++ = i & 0xff;
    }

  while (tot < BUFF_LEN)
    {
      status = send (sock, sendBuffer + tot, BUFF_LEN - tot, 0);
      printf ("Client1: send %d / %ld\n", status, BUFF_LEN - tot);

      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUFF_LEN);

  tot = 0;

  while (tot < BUFF_LEN)
    {
      status = recv (sock, readBuffer + tot, BUFF_LEN - tot, 0);
      printf ("Client1: received %d / %ld\n", status, BUFF_LEN - tot);
      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUFF_LEN);

  for (size_t i = 0; i < BUFF_LEN; i++)
    {
      TEST_ASSERT_EQUAL( readBuffer[i], sendBuffer[i] );
    }

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}

static void *
server1 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  size_t tot = 0;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);

  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1234);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

//  TEST_ASSERT ( false ); // TEMPOFUR

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  while (tot < BUFF_LEN)
    {
      status = recv (sockin, readBuffer + tot, BUFF_LEN - tot, 0);
      printf ("Server1: received %d / %ld\n", status, BUFF_LEN - tot);
      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUFF_LEN);

  tot = 0;
  while (tot < BUFF_LEN)
    {
      status = send (sockin, readBuffer + tot, BUFF_LEN - tot, 0);
      printf ("Server1: send %d / %ld\n", status, BUFF_LEN - tot);
      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUFF_LEN);

  status = close (sockin);
  printf("Server1: close -> %d \n ", status);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}

#define TEST2_LEN 1024

// TEST 2 : receive timeout
static void *
client2 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;

  size_t tot = 0;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1235);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status, 0);

  struct timeval tiout;

  tiout.tv_sec = 3;
  tiout.tv_usec = 42;
  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  tot = 0;
  status = recv (sock, readBuffer, TEST2_LEN, 0);
  TEST_ASSERT( ( status == -1 ) && (errno == EAGAIN) );

  status = recv (sock, readBuffer, TEST2_LEN, 0);
  TEST_ASSERT( status > 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf("Client2: end \n \n ");

  return arg;
}

static void *
server2 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  size_t tot = 0;

  sock = socket (AF_INET, SOCK_STREAM, 0);

  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1235);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  char *crsr = sendBuffer;
  for (size_t i = 0; i < TEST2_LEN; i++)
    {
      *crsr++ = i & 0xff;
    }

  sleep (5);
  status = send (sockin, sendBuffer, TEST2_LEN, 0);
  TEST_ASSERT( status > 0 );

  // Close after send : client must receive last data
  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  printf("Server2: end \n \n ");

  return arg;
}

#define TEST3_LEN BUFF_LEN

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
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1236);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status, 0);

  tot = 0;
  do
    {
      printf("Client3: before sleep time %ld \n ",time(0));
      // Slow read until remote closure
      sleep (10);
      printf("Client3: after sleep time %ld \n ",time(0));
      status = recv (sock, readBuffer, TEST3_LEN, 0);
      printf("Client3: recv -> %d \n ", status);
      fflush(stdout);
    }
  while (status > 0);
  // printf("Client3: after recvs status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf("Client3: end\n ");

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
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1236);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

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
      printf("Server3: send -> %d, errno: %d \n ",status, errno);
    }
  while (status > 0);
  TEST_ASSERT( ( status == -1 ) && (errno == EAGAIN) );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  printf("Server3: end\n ");

  return arg;
}

#define TEST4_LEN BUFF_LEN
// TEST 4 : Error cases:
//  - read from not connected socket
//  - write to not connected socket.
static void *
client4 (void *arg)
{
  int status;
  int sock = -1;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  status = recv (sock, readBuffer, TEST4_LEN, 0);
  printf ("Client4: recv status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EINVAL );

  status = send (sock, readBuffer, TEST4_LEN, 0);
  printf ("Client4: send status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTCONN );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}

static void *
server4 (void *arg)
{
  sleep (1);
  return arg;
}

// TEST 5 : connect timeout
static void *
client5 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1236);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT( status == -1 );
  TEST_ASSERT_EQUAL (errno, ECONNREFUSED );

  sleep (15);

  printf ("Client5: second connection attempt.\n");
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  printf ("Client5: second connect result : %d, errno:%d \n \n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  sleep (1);
  printf ("Client5: end \n\n");

  return arg;
}
static void *
server5 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);

  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1236);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);
  printf("Server5: bind ok , sleeping 10s \n");

  sleep (10);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server5: accepting...\n");
  sockin = accept (sock, NULL, NULL);
  printf ("Server5: after accept ! \n ");
  TEST_ASSERT( sockin >= 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  sleep (1);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server5: end \n");

  return arg;
}

// TEST 6 : connect over closed socket file
static void *
client6 (void *arg)
{
  int status = 42;
  struct sockaddr_in ad;
  int sock = -1;
  int sock2 = -1;
  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1237);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status , 0 );

  sleep(1);

  sock2 = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock2 >= 0 );

  fill_addr(ad, 1237);
  status = connect (sock2, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status , -1 );
  TEST_ASSERT_EQUAL (errno , ECONNREFUSED );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock2);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client6: end \n\n");
  fflush (stdout);

  return arg;
}
static void *
server6 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);

  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1237);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}
/*
// TEST 7: getsockopt
static void *
client7 (void *arg)
{
  int status = 42;
  struct sockaddr_un address;
  int sock = -1;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status , 0 );

  int val = 42;
  socklen_t sz = sizeof(val);

  status = getsockopt (sock, SOL_SOCKET, SO_ACCEPTCONN, &val, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (val, 0);

  struct timeval tv;
  tv.tv_sec = 42;
  sz = sizeof(tv);
  status = getsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tv, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL(tv.tv_sec, 0);
  TEST_ASSERT_EQUAL(tv.tv_usec, 0);

  tv.tv_sec = 42;
  sz = sizeof(tv);
  status = getsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tv, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL(tv.tv_sec, 0);
  TEST_ASSERT_EQUAL(tv.tv_usec, 0);

  sleep (10);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client7: end \n\n");
  fflush (stdout);

  return arg;
}
static void *
server7 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;

  sock = socket (AF_UNIX, SOCK_STREAM, 0);

  TEST_ASSERT( sock >= 0 );

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  int val = 0;
  socklen_t sz = sizeof(val);

  status = getsockopt (sock, SOL_SOCKET, SO_ACCEPTCONN, &val, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_UNEQUAL(val, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  val = 0;
  sz = sizeof(val);
  status = getsockopt (sock, SOL_SOCKET, SO_ACCEPTCONN, &val, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_UNEQUAL(val, 0);

  sleep (10);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  return arg;
}
*/
// TEST 8: shutdown and getsockname and getpeername
static void *
client8 (void *arg)
{
  int status = 42;
  struct sockaddr_in ad;
  struct sockaddr peer;
  struct sockaddr loc;
  int sock = -1;
  char buf[1024];

  printf ("enter client8\n");

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1239);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status , 0 );

  socklen_t len = sizeof(struct sockaddr);
  status = getpeername (sock, (struct sockaddr *) &peer, &len);
  printf("Client8: peer port %d \n \n ", ntohs( ((struct sockaddr_in*)&peer)->sin_port ));
  TEST_ASSERT_EQUAL ( status , 0 );
//  TEST_ASSERT_EQUAL ( memcmp (&ad, &peer, sizeof(ad)) , 0 );

  len = sizeof(struct sockaddr);
  memset (&loc, 0, len);
  status = getsockname (sock, (struct sockaddr *) &loc, &len);
  TEST_ASSERT_EQUAL ( status , 0 );
  printf("Client8: local port %d \n \n ", ntohs( ((struct sockaddr_in*)&loc)->sin_port ));

  sleep (3);

  status = shutdown (sock, SHUT_RD);
  TEST_ASSERT_EQUAL (status , 0 );

  printf ("client8: 2 reads\n");

  status = read (sock, buf, 1024);
  TEST_ASSERT_EQUAL (status , sizeof(int) );

  status = read (sock, buf, 1024);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1239);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status , 0 );

  status = write (sock, &status, sizeof(status));
  TEST_ASSERT_EQUAL(status, sizeof(status));

  status = shutdown (sock, SHUT_WR);
  TEST_ASSERT_EQUAL (status , 0 );

  status = write (sock, &status, sizeof(status));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno , EPIPE );

  sleep (5);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client8: end \n\n");
  fflush (stdout);

  return arg;
}

static void *
server8 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;
  struct sockaddr_in peerName;
  struct sockaddr_in locName;

  sock = socket (AF_INET, SOCK_STREAM, 0);

  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1239);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  socklen_t len = sizeof(struct sockaddr_in);
  memset (&locName, 0, len);
  status = getsockname (sock, (struct sockaddr *) &locName, &len);
  printf("Server8: getsockname -> %d\n\n ", status);
  printf("Server8: local port %d \n \n ", ntohs( locName.sin_port ));

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  len = sizeof(struct sockaddr_in);
  memset (&peerName, 0, len);
  status = getpeername (sockin, (struct sockaddr *) &peerName, &len);
  printf("Server8: getpeername -> %d\n\n ", status);
  printf("Server8: peer port %d \n \n ", ntohs( peerName.sin_port ));

  status = write (sockin, &status, sizeof(status));
  TEST_ASSERT_EQUAL(status, sizeof(status));

  sleep (4);
  status = write (sockin, &status, sizeof(status));

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);
  sockin = -1;

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  sleep (1);

  status = read (sockin, &status, sizeof(status));
  printf("server8: read %d errno %d \n \n ",status, errno);
  TEST_ASSERT_EQUAL (status , sizeof(status) );

  status = read (sockin, &status, sizeof(status));
  printf("server8: read %d errno %d \n \n ",status, errno);
  TEST_ASSERT_EQUAL (status , 0 );

  sleep (10);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf("server8: end\n \n ");

  return arg;
}

// TEST 9 : accept timeout
static void *
client9 (void *arg)
{
  return arg;
}

static void *
server9 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1238);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad) );
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  struct timeval tiout;

  tiout.tv_sec = 1;
  tiout.tv_usec = 0;

  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT_EQUAL( sockin , -1 );
  TEST_ASSERT_EQUAL( errno , EAGAIN );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf("Server9: end\n\n ");

  return arg;
}


#define LEN_10 BUFF_LEN

// TEST 10 : O_NONBLOCK
static void *
client10 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;
  size_t tot = 0;

  sleep (1);
  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1240);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  printf ("Client10: 1st connect -> %d errno:%d\n", status, errno);
  TEST_ASSERT_EQUAL (status , 0 );

  char *crsr = sendBuffer;
  for (size_t i = 0; i < LEN_10; i++)
    {
      *crsr++ = i & 0xff;
    }

  int flag = 0;
  flag = fcntl (sock, F_GETFL, &flag);
  flag |= O_NONBLOCK;
  status = fcntl (sock, F_SETFL, flag);
  TEST_ASSERT_EQUAL (status, 0);

  do
    {
      status = send (sock, sendBuffer + tot, LEN_10 - tot, 0);
      printf ("Client10: send %d / %ld\n", status, LEN_10 - tot);
      tot += status;
      if (tot >= LEN_10) tot = 0;
    }
  while (status != -1);

  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client10: end \n\n");

  return arg;
}

static void *
server10 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr(ad, 1240);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  do
    {
      sleep (1);
      status = recv (sockin, readBuffer, LEN_10, 0);
      printf ("Server10: recv -> %d errno:%d\n\n", status, errno);
      sleep (1);
    }
  while (status > 0);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server10: end\n \n ");

  return arg;
}

#define LEN_11 BUFF_LEN
// TEST 11 : Server Accept, write and close without waiting ...
static void *
client11 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr ( ad, 1241 );
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL( status,0 );

  sleep (1);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client11: end \n\n");

  return arg;
}

static void *
server11 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr ( ad, 1241 );
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = send (sockin, sendBuffer, LEN_11, 0);
  printf("Server11: send -> %d / %d \n \n ", status, LEN_11);
  TEST_ASSERT( status >= 0 );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server11: end \n");

  return arg;
}

// TEST 12 : last test do not close socket fd
static void *
client12 (void *arg)
{
  int sock = socket (AF_INET, SOCK_STREAM, 0);

  if (sock > 0)
    sock = 0;

  return 0;
}

#define LEN_13 BUFF_LEN
// TEST 13: Select success
static void *
client13 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr( ad, 1242);
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL( status, 0 );

  sleep (1);

  fd_set readFd;
  fd_set writeFd;
  int maxFd = sock + 1;

  FD_ZERO(&readFd);
  FD_SET( sock, &readFd );
  FD_ZERO(&writeFd);
  FD_SET( 1, &writeFd );
  FD_SET( sock, &writeFd );

  status = select (maxFd, &readFd, &writeFd, NULL, NULL);
  TEST_ASSERT ( status >= 2 );
  TEST_ASSERT( FD_ISSET(sock, &readFd) );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client13: end \n\n");

  return arg;
}

static void *
server13 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr( ad, 1242);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = send (sockin, sendBuffer, LEN_13, 0);
  TEST_ASSERT( status >= 0 );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server13: end \n");

  return arg;
}

#define LEN_14 BUFF_LEN
// TEST 14: Select timeout
static void *
client14 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr ( ad, 1243 );
  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL( status, 0 );

  fd_set readFd;

  struct timeval timeOut;
  int maxFd = sock + 1;

  FD_ZERO(&readFd);

  timeOut.tv_sec = 1;
  timeOut.tv_usec = 0;

  FD_SET( sock, &readFd );
  FD_SET( 0, &readFd );

  status = select (maxFd, &readFd, NULL, NULL, &timeOut);
  printf("Client14: select -> %d, errno: %d \n", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  sleep(2);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client14: end \n\n");

  return arg;
}

static void *
server14 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr ( ad, 1243 );
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  sleep(2);

  status = send (sockin, sendBuffer, LEN_14, 0);
  TEST_ASSERT( status >= 0 );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server14: end \n");

  return arg;
}

static void
CloseAll(std::vector<int> &list)
{
  while(!list.empty())
    {
      int toClose = list.back();

      list.pop_back();

      printf("Closing fd %d \n ",toClose);
      close(toClose);
    }
}

// TEST 15: lot of connexions lot of fd
static void *
client15 (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;
  std::vector<int> closeList;

  do {
      sock = socket (AF_INET, SOCK_STREAM, 0);
      if (sock < 0) break;
      closeList.push_back(sock);

      fill_addr ( ad, 1244 );
      status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
      printf("Client15: connect -> %d \n ", status);
  } while(true);

  printf("Client15: out of do-while\n ");

  CloseAll(closeList);

  printf ("Client15: end \n\n ");

  return arg;
}

static void *
server15 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;
  std::vector<int> closeList;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  fill_addr ( ad, 1244 );
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  closeList.push_back(sockin);

  do
    {
      fd_set readFd;
      int maxFd = (sock > sockin) ? sock : sockin;

      FD_ZERO(&readFd);

      FD_SET( sock, &readFd );
      FD_SET( sockin, &readFd );

      printf ("Server15: selecting ... \n ");

      status = select ( 1 + maxFd, &readFd, NULL, NULL, NULL);
      printf ("Server15: select -> %d \n ", status);
      if (status > 0)
        {
          if ( FD_ISSET( sockin, &readFd) )
            {
              // The client close it !
              break;
            }
          if ( FD_ISSET( sock, &readFd) )
            {
              // Incoming Connexion
              int newSock = accept (sock, NULL, NULL);
              printf ("Server15: accept -> %d \n\n ",newSock );
              if ( newSock >= 0 )
                {
                  closeList.push_back( newSock );
                }
              else
                {
                  break;
                }
            }
        }
      else
        {
          // workaround
          break;
        }
    }
  while (true);

  CloseAll(closeList);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Server15: end \n ");

  return arg;
}


static void
launch (void *
(*clientStart) (void *), void *
(*serverStart) (void *))
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
}

int
main (int argc, char *argv[])
{
  signal (SIGPIPE, SIG_IGN);

  unsigned short int rac[3] = { 3 , 2, 1 };

  seed48(rac);

  for(int i=0;i < 20; i++)
    {
      unsigned short param[7];

      srand48( (unsigned long) i*i );

      for (int j=0; j < 7 ; j++)
        {
          param[j] = (unsigned short) lrand48() ;
        }

      lcong48(param);

      printf("nrand48 %ld %ld %ld %f %ld \n", nrand48 (rac), lrand48 (), mrand48 (), erand48 (rac), jrand48 (rac));
    }
  fflush(stdout);

  readBuffer = (char *)malloc( BUFF_LEN );
  sendBuffer = (char *)malloc( BUFF_LEN );

  launch (client1, server1);
  launch (client2, server2);
  launch (client4, server4);
  launch (client3, server3); // NS3 failed : tcp stack bug like bug 907
  // the server defer close because it is waiting for : Stop sending if we need to wait for a larger Tx window
  // But it is never came ....
  // The client is blocked in read ....      launch (client5, server5);
  launch (client6, server6);
  launch (client8, server8);
  launch (client9, server9);
  launch (client10, server10);
  launch (client11, server11);
  launch (client12, client12);
  launch (client13, server13);
  launch (client14, server14);
  launch (client15, server15); // XXX crash will invoking callback :
  /* TEST 15 : crash
   * Program received signal SIGSEGV, Segmentation fault.
0x00007ffff72b72dc in ns3::Callback<void, ns3::Ptr<ns3::Packet>, ns3::Ipv4Header, unsigned short, ns3::Ptr<ns3::Ipv4Interface>, ns3::empty, ns3::empty, ns3::empty, ns3::empty, ns3::empty>::operator() (
    this=0x7fffe8001c58, a1=..., a2=..., a3=49671, a4=...) at debug/ns3/callback.h:419
419         return (*(DoPeekImpl ())) (a1,a2,a3,a4);
(gdb) bt
#0  0x00007ffff72b72dc in ns3::Callback<void, ns3::Ptr<ns3::Packet>, ns3::Ipv4Header, unsigned short, ns3::Ptr<ns3::Ipv4Interface>, ns3::empty, ns3::empty, ns3::empty, ns3::empty, ns3::empty>::operator() (
    this=0x7fffe8001c58, a1=..., a2=..., a3=49671, a4=...) at debug/ns3/callback.h:419
#1  0x00007ffff72b665a in ns3::Ipv4EndPoint::DoForwardUp (this=0x7fffe8001c40, p=..., header=..., sport=49671, incomingInterface=...) at ../src/internet/model/ipv4-end-point.cc:124
#2  0x00007ffff72b6eaa in ns3::EventMemberImpl4::Notify (this=0x6d5b80) at debug/ns3/make-event.h:223
#3  0x00007ffff77accf8 in ns3::EventImpl::Invoke (this=0x6d5b80) at ../src/core/model/event-impl.cc:37
#4  0x00007ffff77b11ac in ns3::DefaultSimulatorImpl::ProcessOneEvent (this=0x62cba0) at ../src/core/model/default-simulator-impl.cc:128
#5  0x00007ffff77b135c in ns3::DefaultSimulatorImpl::Run (this=0x62cba0) at ../src/core/model/default-simulator-impl.cc:158
   *
   *
   *
   *
   */

  printf ("That's All Folks ....\n \n " );
  fflush (stdout);
  fclose(stdout);
  sleep (1);

  free(readBuffer);
  free(sendBuffer);
  readBuffer = sendBuffer = 0;

  return 0;
}
