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

#define SOCK_PATH "/tmp/socket"
#define BUF_LEN ((size_t) 1000001)

// TEST 1 : simple ping pong of data
static void *
client1 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;
  char *sendBuffer = (char*) malloc (BUF_LEN);
  char *readBuffer = (char*) malloc (BUF_LEN);
  size_t tot = 0;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT( sock >= 0 );

  char *crsr = sendBuffer;
  for (size_t i = 0; i < BUF_LEN; i++)
    {
      *crsr++ = i & 0xff;
    }
  //  memset (sendBuffer, 'F', BUF_LEN);
  while (tot < BUF_LEN)
    {
      status = send (sock, sendBuffer + tot, BUF_LEN - tot, 0);
      printf ("Client1: send %d / %ld\n", status, BUF_LEN - tot);

      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUF_LEN);

  tot = 0;

  while (tot < BUF_LEN)
    {
      status = recv (sock, readBuffer + tot, BUF_LEN - tot, 0);
      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUF_LEN);

  for (size_t i = 0; i < BUF_LEN; i++)
    {
      TEST_ASSERT_EQUAL( readBuffer[i], sendBuffer[i] );
    }

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  free (readBuffer);
  free (sendBuffer);

  return arg;
}

static void *
server1 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  size_t tot = 0;

  char *buffer = (char*) malloc (BUF_LEN);

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

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  while (tot < BUF_LEN)
    {
      status = recv (sockin, buffer + tot, BUF_LEN - tot, 0);
      printf ("Server1: received %d / %ld\n", status, BUF_LEN - tot);
      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUF_LEN);

  tot = 0;
  while (tot < BUF_LEN)
    {
      status = send (sockin, buffer + tot, BUF_LEN - tot, 0);
      TEST_ASSERT( status > 0 );
      tot += status;
    }
  TEST_ASSERT_EQUAL( tot, BUF_LEN);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  free (buffer);

  return arg;
}

#undef BUF_LEN
#define BUF_LEN 1024

// TEST 2 : receive timeout
static void *
client2 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;

  char readBuffer[BUF_LEN];

  size_t tot = 0;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status, 0);

  struct timeval tiout;

  tiout.tv_sec = 3;
  tiout.tv_usec = 42;
  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  tot = 0;
  status = recv (sock, readBuffer, BUF_LEN, 0);
  TEST_ASSERT( ( status == -1 ) && (errno == EAGAIN) );

  status = recv (sock, readBuffer, BUF_LEN, 0);
  TEST_ASSERT( status > 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}

static void *
server2 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  char buffer[BUF_LEN];

  size_t tot = 0;

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

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  char *crsr = buffer;
  for (size_t i = 0; i < BUF_LEN; i++)
    {
      *crsr++ = i & 0xff;
    }

  sleep (5);

  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT( status > 0 );
  tot += status;

  // Close after send : client must receive last data
  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  return arg;
}

// TEST 3 : write timeout
static void *
client3 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;

  char readBuffer[BUF_LEN];

  size_t tot = 0;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status, 0);

  tot = 0;
  do
    {
      // Slow read until remote closure
      sleep (10);
      status = recv (sock, readBuffer, BUF_LEN, 0);
    }
  while (status > 0);
  // printf("Client3: after recvs status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}

static void *
server3 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  char buffer[BUF_LEN];

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

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  char *crsr = buffer;
  for (size_t i = 0; i < BUF_LEN; i++)
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
      status = send (sockin, buffer, BUF_LEN, 0);
    }
  while (status > 0);
  TEST_ASSERT( ( status == -1 ) && (errno == EAGAIN) );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  return arg;
}

// TEST 4 : return errors
static void *
client4 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;

  char readBuffer[BUF_LEN];

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = recv (sock, readBuffer, BUF_LEN, 0);
  printf ("Client4: recv status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EINVAL );

  status = send (sock, readBuffer, BUF_LEN, 0);
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
  struct sockaddr_un address;
  int sock = -1;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  struct timeval tiout;

  tiout.tv_sec = 2;
  tiout.tv_usec = 0;
  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  tiout.tv_sec = 2;
  tiout.tv_usec = 0;
  status = setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT( status == -1 );
  TEST_ASSERT_EQUAL (errno, EAGAIN );

  sleep (15);

  printf ("Client5: second connection attempt.\n");
  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
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

  sleep (10);

  printf ("Server5: accepting...\n");
  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  sleep (1);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server5: end \n");

  return arg;
}

// TEST 6 : connect over deleted socket file
static void *
client6 (void *arg)
{
  int status = 42;
  struct sockaddr_un address;
  int sock = -1;
  int sock2 = -1;
  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status , 0 );

  status = unlink (SOCK_PATH);

  sock2 = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock2 >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock2, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status , -1 );
  TEST_ASSERT_EQUAL (errno , ENOENT );

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

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  sleep (10);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  return arg;
}

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

// TEST 8: shutdown and getsockname and getpeername
static void *
client8 (void *arg)
{
  int status = 42;
  struct sockaddr_un address, addr2;
  int sock = -1;
  char buf[1024];

  printf ("enter client8\n");

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status , 0 );

  socklen_t len = sizeof(struct sockaddr_un);
  status = getpeername (sock, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL ( status , 0 );
  TEST_ASSERT_EQUAL ( address.sun_family, addr2.sun_family );
  TEST_ASSERT_EQUAL ( strcmp(address.sun_path, addr2.sun_path) , 0 );

  len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getsockname (sock, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL ( status , 0 );
  TEST_ASSERT_EQUAL ( address.sun_family, addr2.sun_family );
  TEST_ASSERT_EQUAL ( strlen( addr2.sun_path) , 0 );

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

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status , 0 );

  status = write (sock, &status, sizeof(status));
  TEST_ASSERT_EQUAL(status, sizeof(status));

  status = shutdown (sock, SHUT_WR);
  TEST_ASSERT_EQUAL (status , 0 );

  status = write (sock, &status, sizeof(status));
  TEST_ASSERT_EQUAL(status, -1);
  TEST_ASSERT_EQUAL (errno , EPIPE );

  sleep (5);

  // status = close (sock);
  // TEST_ASSERT_EQUAL (status, 0);


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

  sock = socket (AF_UNIX, SOCK_STREAM, 0);

  TEST_ASSERT( sock >= 0 );

  struct sockaddr_un address, addr2;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status, 0);

  socklen_t len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getsockname (sock, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL ( status , 0 );
  TEST_ASSERT_EQUAL ( address.sun_family, addr2.sun_family );
  TEST_ASSERT_EQUAL ( strcmp(address.sun_path, addr2.sun_path) , 0 );

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getpeername (sockin, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL ( status , 0 );
  TEST_ASSERT_EQUAL ( address.sun_family, addr2.sun_family );
  TEST_ASSERT_EQUAL ( strlen(addr2.sun_path) , 0 );

  len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getsockname (sockin, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL ( status , 0 );
  TEST_ASSERT_EQUAL ( address.sun_family, addr2.sun_family );
  TEST_ASSERT_EQUAL ( strcmp(address.sun_path, addr2.sun_path) , 0 );

  status = write (sockin, &status, sizeof(status));
  TEST_ASSERT_EQUAL(status, sizeof(status));

  sleep (4);
  status = write (sockin, &status, sizeof(status));
  TEST_ASSERT_EQUAL(status, -1);
  TEST_ASSERT_EQUAL (errno , EPIPE );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  sleep (1);

  status = read (sockin, &status, sizeof(status));
  //  printf("server8: read %d errno %d\n",status, errno);
  TEST_ASSERT_EQUAL (status , sizeof(status) );

  status = read (sockin, &status, sizeof(status));
  //  printf("server8: read %d errno %d\n",status, errno);
  TEST_ASSERT_EQUAL (status , 0 );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

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

  unlink (SOCK_PATH);

  return arg;
}

#undef BUF_LEN
#define BUF_LEN ((size_t) 1000001)

// TEST 10 : O_NONBLOCK
static void *
client10 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;
  char *sendBuffer = (char*) malloc (BUF_LEN);
  char *readBuffer = (char*) malloc (BUF_LEN);
  size_t tot = 0;

  sleep (3);
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );
  int flag = 0;
  flag = fcntl (sock, F_GETFL, &flag);
  flag |= O_NONBLOCK;
  status = fcntl (sock, F_SETFL, flag);
  TEST_ASSERT_EQUAL (status, 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  printf ("Client10: 1st connect -> %d errno:%d\n", status, errno);
  TEST_ASSERT_EQUAL (status , 0 );

  do
    {
      int sock2 = socket (AF_UNIX, SOCK_STREAM, 0);

      flag = fcntl (sock2, F_GETFL, &flag);
      flag |= O_NONBLOCK;
      status = fcntl (sock2, F_SETFL, flag);
      TEST_ASSERT_EQUAL (status, 0);

      status = connect (sock2, (struct sockaddr *) &address, SUN_LEN(&address));
      printf ("Client10: connect -> %d errno:%d\n", status, errno);
      if (0 == status)
        close (sock2);
    }
  while (status >= 0);

  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  char *crsr = sendBuffer;
  for (size_t i = 0; i < BUF_LEN; i++)
    {
      *crsr++ = i & 0xff;
    }

  status = recv (sock, readBuffer + tot, BUF_LEN - tot, 0);
  printf ("Client10: recv -> %d errno:%d\n", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  //  memset (sendBuffer, 'F', BUF_LEN);
  do
    {
      status = send (sock, sendBuffer + tot, BUF_LEN - tot, 0);
      printf ("Client10: send %d / %ld\n", status, BUF_LEN - tot);
      tot += status;
    }
  while (status != -1);

  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  free (readBuffer);
  free (sendBuffer);

  printf ("Client10: end \n\n");

  return arg;
}

static void *
server10 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  char *buffer = (char*) malloc (BUF_LEN);

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

  int flag = 0;
  flag = fcntl (sock, F_GETFL, &flag);
  flag |= O_NONBLOCK;
  status = fcntl (sock, F_SETFL, flag);
  TEST_ASSERT_EQUAL (status, 0);
  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT_EQUAL (sockin, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  status = fcntl (sock, F_SETFL, 0);
  TEST_ASSERT_EQUAL (status, 0);
  sleep (2);
  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  do
    {
      sleep (1);
      status = recv (sockin, buffer, BUF_LEN, 0);
      printf ("Server10: recv -> %d errno:%d\n\n", status, errno);
      sleep (1);
    }
  while (status > 0);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  free (buffer);

  return arg;
}

#undef BUF_LEN
#define BUF_LEN ((size_t)1024)

// TEST 11 : Server Accept, write and close without waiting ...
static void *
client11 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
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
  struct sockaddr_un address;

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  char *buffer = (char*) malloc (BUF_LEN);
  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT( status >= 0 );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);
  free (buffer);

  printf ("Server11: end \n");

  return arg;
}

// TEST 12 : last test do not close socket fd
static void *
client12 (void *arg)
{
  int sock = socket (AF_UNIX, SOCK_STREAM, 0);

  if (sock > 0)
    sock = 0;

  return 0;
}

// TEST 13: Select success
static void *
client13 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
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
  struct sockaddr_un address;
  char *buffer = (char*) malloc (BUF_LEN);

  printf ("Server13: BUF_LEN:%ld\n", BUF_LEN);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT( status >= 0 );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);
  free (buffer);

  printf ("Server13: end \n");

  return arg;
}

// TEST 14: Select timeout
static void *
client14 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;

  sleep (1);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
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
  struct sockaddr_un address;
  char *buffer = (char*) malloc (BUF_LEN);

  printf ("Server14: BUF_LEN:%ld\n", BUF_LEN);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN(&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT( sockin >= 0 );

  sleep(2);

  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT( status >= 0 );

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);
  free (buffer);

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
  struct sockaddr_un address;
  int sock = -1;
  std::vector<int> closeList;

  do {
      sock = socket (AF_UNIX, SOCK_STREAM, 0);
      if (sock < 0) break;
      closeList.push_back(sock);

      memset (&address, 0, sizeof(address));
      address.sun_family = AF_UNIX;
      strcpy (address.sun_path, SOCK_PATH);

      status = connect (sock, (struct sockaddr *) &address, SUN_LEN(&address));
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
  struct sockaddr_un address;
  std::vector<int> closeList;

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT( sock >= 0 );

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  unlink (SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN(&address));
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
              if ( newSock >= 0 )
                {
                  closeList.push_back( newSock );
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

  unlink (SOCK_PATH);

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

  printf ("launch\n");

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

  if (1 < 2)
    {
      launch (client15, server15);
    }
  else
    {
      launch (client1, server1);
      launch (client2, server2);
      launch (client3, server3);
      launch (client4, server4);
      launch (client5, server5);
      launch (client6, server6);
      launch (client7, server7);
      launch (client8, server8);
      launch (client9, server9);
      launch (client10, server10);
      launch (client11, server11);
      launch (client12, client12);
      launch (client13, server13);
      launch (client14, server14); // Failed
    }
  //

  printf ("That's All Folks ....\n \n " );
  fflush (stdout);
  fclose(stdout);
  sleep (1);

  return 0;
}
