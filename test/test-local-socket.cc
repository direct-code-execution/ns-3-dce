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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT (sock >= 0);

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

      TEST_ASSERT (status > 0);
      tot += status;
    }
  TEST_ASSERT_EQUAL (tot, BUF_LEN);

  tot = 0;

  while (tot < BUF_LEN)
    {
      status = recv (sock, readBuffer + tot, BUF_LEN - tot, 0);
      TEST_ASSERT (status > 0);
      tot += status;
    }
  TEST_ASSERT_EQUAL (tot, BUF_LEN);

  for (size_t i = 0; i < BUF_LEN; i++)
    {
      TEST_ASSERT_EQUAL (readBuffer[i], sendBuffer[i]);
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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  unlink (SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  while (tot < BUF_LEN)
    {
      status = recv (sockin, buffer + tot, BUF_LEN - tot, 0);
      printf ("Server1: received %d / %ld\n", status, BUF_LEN - tot);
      TEST_ASSERT (status > 0);
      tot += status;
    }
  TEST_ASSERT_EQUAL (tot, BUF_LEN);

  tot = 0;
  while (tot < BUF_LEN)
    {
      status = send (sockin, buffer + tot, BUF_LEN - tot, 0);
      TEST_ASSERT (status > 0);
      tot += status;
    }
  TEST_ASSERT_EQUAL (tot, BUF_LEN);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  struct timeval tiout;

  tiout.tv_sec = 3;
  tiout.tv_usec = 42;
  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  tot = 0;
  status = recv (sock, readBuffer, BUF_LEN, 0);
  TEST_ASSERT ((status == -1) && (errno == EAGAIN));

  status = recv (sock, readBuffer, BUF_LEN, 0);
  TEST_ASSERT (status > 0);

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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  char *crsr = buffer;
  for (size_t i = 0; i < BUF_LEN; i++)
    {
      *crsr++ = i & 0xff;
    }

  sleep (5);

  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT (status > 0);
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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

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
  TEST_ASSERT ((status == -1) && (errno == EAGAIN));

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = recv (sock, readBuffer, BUF_LEN, 0);
  printf ("Client4: recv status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);

  status = send (sock, readBuffer, BUF_LEN, 0);
  printf ("Client4: send status %d and errno %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTCONN);

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
  TEST_ASSERT (sock >= 0);

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

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT (status == -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  sleep (15);

  printf ("Client5: second connection attempt.\n");
  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sleep (10);

  printf ("Server5: accepting...\n");
  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = unlink (SOCK_PATH);

  sock2 = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT (sock2 >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock2, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOENT);

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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

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
  TEST_ASSERT_EQUAL (tv.tv_sec, 0);
  TEST_ASSERT_EQUAL (tv.tv_usec, 0);

  tv.tv_sec = 42;
  sz = sizeof(tv);
  status = getsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tv, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (tv.tv_sec, 0);
  TEST_ASSERT_EQUAL (tv.tv_usec, 0);

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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  int val = 0;
  socklen_t sz = sizeof(val);

  status = getsockopt (sock, SOL_SOCKET, SO_ACCEPTCONN, &val, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_UNEQUAL (val, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  val = 0;
  sz = sizeof(val);
  status = getsockopt (sock, SOL_SOCKET, SO_ACCEPTCONN, &val, &sz);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_UNEQUAL (val, 0);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  socklen_t len = sizeof(struct sockaddr_un);
  status = getpeername (sock, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (address.sun_family, addr2.sun_family);
  TEST_ASSERT_EQUAL (strcmp (address.sun_path, addr2.sun_path), 0);

  len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getsockname (sock, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (address.sun_family, addr2.sun_family);
  TEST_ASSERT_EQUAL (strlen (addr2.sun_path), 0);

  sleep (3);

  status = shutdown (sock, SHUT_RD);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("client8: 2 reads\n");

  status = read (sock, buf, 1024);
  TEST_ASSERT_EQUAL (status, sizeof(int));

  status = read (sock, buf, 1024);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = write (sock, &status, sizeof(status));
  TEST_ASSERT_EQUAL (status, sizeof(status));

  status = shutdown (sock, SHUT_WR);
  TEST_ASSERT_EQUAL (status, 0);

  status = write (sock, &status, sizeof(status));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EPIPE);

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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address, addr2;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  socklen_t len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getsockname (sock, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (address.sun_family, addr2.sun_family);
  TEST_ASSERT_EQUAL (strcmp (address.sun_path, addr2.sun_path), 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getpeername (sockin, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (address.sun_family, addr2.sun_family);
  TEST_ASSERT_EQUAL (strlen (addr2.sun_path), 0);

  len = sizeof(struct sockaddr_un);
  memset (&addr2, 0, len);
  status = getsockname (sockin, (struct sockaddr *) &addr2, &len);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (address.sun_family, addr2.sun_family);
  TEST_ASSERT_EQUAL (strcmp (address.sun_path, addr2.sun_path), 0);

  status = write (sockin, &status, sizeof(status));
  TEST_ASSERT_EQUAL (status, sizeof(status));

  sleep (4);
  status = write (sockin, &status, sizeof(status));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EPIPE);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  sleep (1);

  status = read (sockin, &status, sizeof(status));
  //  printf("server8: read %d errno %d\n",status, errno);
  TEST_ASSERT_EQUAL (status, sizeof(status));

  status = read (sockin, &status, sizeof(status));
  //  printf("server8: read %d errno %d\n",status, errno);
  TEST_ASSERT_EQUAL (status, 0);

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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  struct timeval tiout;

  tiout.tv_sec = 1;
  tiout.tv_usec = 0;

  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT_EQUAL (sockin, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

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
  TEST_ASSERT (sock >= 0);
  int flag = 0;
  flag = fcntl (sock, F_GETFL, &flag);
  flag |= O_NONBLOCK;
  status = fcntl (sock, F_SETFL, flag);
  TEST_ASSERT_EQUAL (status, 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  printf ("Client10: 1st connect -> %d errno:%d\n", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  do
    {
      int sock2 = socket (AF_UNIX, SOCK_STREAM, 0);

      flag = fcntl (sock2, F_GETFL, &flag);
      flag |= O_NONBLOCK;
      status = fcntl (sock2, F_SETFL, flag);
      TEST_ASSERT_EQUAL (status, 0);

      status = connect (sock2, (struct sockaddr *) &address, SUN_LEN (&address));
      printf ("Client10: connect -> %d errno:%d\n", status, errno);
      if (0 == status)
        {
          close (sock2);
        }
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

  TEST_ASSERT (sock >= 0);

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));

  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
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
  TEST_ASSERT (sockin >= 0);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  char *buffer = (char*) malloc (BUF_LEN);
  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT (status >= 0);

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
    {
      sock = 0;
    }

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  sleep (1);

  fd_set readFd;
  fd_set writeFd;
  int maxFd = sock + 1;

  FD_ZERO (&readFd);
  FD_SET (sock, &readFd);
  FD_ZERO (&writeFd);
  FD_SET (1, &writeFd);
  FD_SET (sock, &writeFd);

  status = select (maxFd, &readFd, &writeFd, NULL, NULL);
  TEST_ASSERT (status >= 2);
  TEST_ASSERT (FD_ISSET (sock, &readFd));

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT (status >= 0);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  fd_set readFd;

  struct timeval timeOut;
  int maxFd = sock + 1;

  FD_ZERO (&readFd);

  timeOut.tv_sec = 1;
  timeOut.tv_usec = 0;

  FD_SET (sock, &readFd);
  // check only if we use tty
  if (isatty (0))
    {
      FD_SET (0, &readFd);
    }

  status = select (maxFd, &readFd, NULL, NULL, &timeOut);
  printf ("Client14: select -> %d, errno: %d \n", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  sleep (2);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  sleep (2);

  status = send (sockin, buffer, BUF_LEN, 0);
  TEST_ASSERT (status >= 0);

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
CloseAll (std::vector<int> &list)
{
  while (!list.empty ())
    {
      int toClose = list.back ();

      list.pop_back ();

      printf ("Closing fd %d \n ",toClose);
      close (toClose);
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

  do
    {
      sock = socket (AF_UNIX, SOCK_STREAM, 0);
      if (sock < 0)
        {
          break;
        }
      closeList.push_back (sock);

      memset (&address, 0, sizeof(address));
      address.sun_family = AF_UNIX;
      strcpy (address.sun_path, SOCK_PATH);

      status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
      printf ("Client15: connect -> %d \n ", status);
    }
  while (true);

  printf ("Client15: out of do-while\n ");

  CloseAll (closeList);

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
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  unlink (SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  closeList.push_back (sockin);

  do
    {
      fd_set readFd;
      int maxFd = (sock > sockin) ? sock : sockin;

      FD_ZERO (&readFd);

      FD_SET (sock, &readFd);
      FD_SET (sockin, &readFd);

      printf ("Server15: selecting ... \n ");

      status = select (1 + maxFd, &readFd, NULL, NULL, NULL);
      printf ("Server15: select -> %d \n ", status);
      if (status > 0)
        {
          if (FD_ISSET (sockin, &readFd))
            {
              // The client close it !
              break;
            }
          if (FD_ISSET (sock, &readFd))
            {
              // Incoming Connexion
              int newSock = accept (sock, NULL, NULL);
              if (newSock >= 0)
                {
                  closeList.push_back (newSock);
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

  CloseAll (closeList);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server15: end \n ");

  return arg;
}

static int
CreateDgramConnect (void)
{
  int status;
  struct sockaddr_un address;
  int sock = socket (AF_UNIX, SOCK_DGRAM, 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));

  if (status == 0)
    {
      return sock;
    }
  close (sock);

  return -1;
}

static int
CreateDgramBind (void)
{
  int status;
  int sock = -1;
  struct sockaddr_un address;

  sock = socket (AF_UNIX, SOCK_DGRAM, 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  if (status == 0)
    {
      return sock;
    }
  printf ("bind failed: errno %d\n \n ", errno);
  close (sock);
  return -1;
}

// SOCK_DGRAM familly
// TEST 16, listen should failed, only one bind over same file, accept should failed too
// write should failed, read too
// connect should success

static void *
client16 (void *arg)
{
  int sock = -1;
  int status = 0;

  sleep (1);
  sock = CreateDgramBind ();
  TEST_ASSERT_EQUAL (sock, -1);
  TEST_ASSERT_EQUAL (errno, EADDRINUSE);

  sock = CreateDgramConnect ();
  TEST_ASSERT (sock >= 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client16: end \n\n ");

  return arg;
}

static void *
server16 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int buf = -1;
  int status;

  TEST_ASSERT (sock >= 0);

  status = listen (sock, 12);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EOPNOTSUPP);

  status = accept (sock, NULL, NULL);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EOPNOTSUPP);

  status = write (sock, &buf, sizeof(buf));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTCONN);

  sleep (5);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server16: end \n\n ");

  return arg;
}

// Simple data exchange using connected socket, bind, write and read.
#define BUF_LEN17 ((size_t) 1024)

static char readBuf17[BUF_LEN17];
static char sendBuf17[BUF_LEN17];

static void *
client17 (void *arg)
{
  int sock = -1;
  int status = 0;

  sleep (1);
  sock = CreateDgramConnect ();
  TEST_ASSERT (sock > 0);

  memset (sendBuf17, 17, sizeof(sendBuf17));

  status = write (sock, sendBuf17, sizeof(sendBuf17));
  TEST_ASSERT_EQUAL (status, sizeof(sendBuf17));

  sleep (1);
  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client17: end \n\n ");

  return arg;
}

static void *
server17 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;

  TEST_ASSERT (sock >= 0);

  status = read (sock, readBuf17, sizeof (readBuf17));
  TEST_ASSERT_EQUAL (status, sizeof(readBuf17));

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server17: end \n\n ");

  return arg;
}

static struct sockaddr_un stAddress;

static struct sockaddr *
PrepAddr (void)
{
  memset (&stAddress, 0, sizeof(stAddress));
  stAddress.sun_family = AF_UNIX;
  strcpy (stAddress.sun_path, SOCK_PATH);
  return (struct sockaddr *) &stAddress;
}

// Simple data exchange using bind, sendto and recvfrom
#define BUF_LEN18 ((size_t) 1024)

static char readBuf18[BUF_LEN18];
static char sendBuf18[BUF_LEN18];

static void *
client18 (void *arg)
{
  int sock = -1;
  int status = 0;

  sleep (1);
  sock = socket (AF_UNIX, SOCK_DGRAM, 0);
  TEST_ASSERT (sock > 0);

  memset (sendBuf18, 18, sizeof(sendBuf18));

  status = sendto (sock, sendBuf18, sizeof(sendBuf18), 0, PrepAddr (), sizeof(sockaddr_un));
  printf ("Server18: sendto -> %d, errno: %d \n \n ", status, errno);
  TEST_ASSERT_EQUAL (status, sizeof(sendBuf18));

  sleep (1);
  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client18: end \n\n ");

  return arg;
}

static void *
server18 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  socklen_t l = sizeof(sockaddr_un);
  struct sockaddr_un exp;


  TEST_ASSERT (sock >= 0);

  memset (&exp, 0, l);

  status = recvfrom (sock, readBuf18, sizeof (readBuf18), 0, (struct sockaddr *) &exp, &l);
  printf ("Server18: recvfrom -> %d, errno: %d FROM: %s \n \n ", status, errno, exp.sun_path);
  TEST_ASSERT_EQUAL (status, sizeof (readBuf18));

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server18: end \n\n ");

  return arg;
}

// Simple data exchange using  sendmsg, bind and recvmsg
#define BUF_LEN19 ((size_t) 1024)
static char readBuf19[BUF_LEN19];
static char sendBuf19[BUF_LEN19];
static void *
client19 (void *arg)
{
  int sock = -1;
  int status = 0;
  struct msghdr mes;
  struct iovec msg1;

  sleep (1);
  sock = socket (AF_UNIX, SOCK_DGRAM, 0);
  TEST_ASSERT (sock > 0);

  memset (sendBuf19, 19, sizeof(sendBuf19));

  msg1.iov_base = &sendBuf19;
  msg1.iov_len = sizeof(sendBuf19);

  mes.msg_name = (void*) PrepAddr ();
  mes.msg_namelen = sizeof(sockaddr_un);
  mes.msg_iov = &msg1;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;

  ssize_t res =  sendmsg (sock, &mes, 0);
  printf ("sendmsg --> %ld, errno: %d \n \n ", res, errno);
  TEST_ASSERT_EQUAL (res, sizeof(sendBuf19));

  sleep (1);
  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client19: end \n\n ");

  return arg;
}

static void *
server19 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  struct msghdr mes;
  struct iovec msgs[3];

  TEST_ASSERT (sock >= 0);

  msgs[0].iov_base = &(readBuf19[sizeof (readBuf19) / 2  ]);
  msgs[0].iov_len = sizeof (readBuf19) / 2;
  msgs[1].iov_base = readBuf19;
  msgs[1].iov_len = sizeof (readBuf19) / 2;
  msgs[2].iov_base = readBuf19;
  msgs[2].iov_len = sizeof (readBuf19) / 2;
  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = msgs;
  mes.msg_iovlen = 3;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;

  ssize_t lu = recvmsg (sock, &mes, 0);
  printf ("recvmsg -> %ld, errno: %d \n\n ", lu, errno);
  TEST_ASSERT_EQUAL (lu, sizeof (sendBuf19));

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server19: end \n\n ");

  return arg;
}

// Data exchange without same buffer size , TEST MSG_TRUNC flag
#define BUF_LEN20 ((size_t) 1024)
static char readBuf20[BUF_LEN20];
static char sendBuf20[BUF_LEN20];
static void *
client20 (void *arg)
{
  sleep (1);

  int sock = CreateDgramConnect ();
  int status = 0;
  struct msghdr mes;
  struct iovec msg1;

  TEST_ASSERT (sock > 0);

  memset (sendBuf20, 20, sizeof(sendBuf20));
  msg1.iov_base = &sendBuf20;
  msg1.iov_len = sizeof(sendBuf20);

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = &msg1;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;

  ssize_t res =  sendmsg (sock, &mes, 0);
  printf ("sendmsg --> %ld, errno: %d \n \n ", res, errno);
  TEST_ASSERT_EQUAL (res, sizeof (sendBuf20));

  sleep (1);
  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client20: end \n\n ");

  return arg;
}

static void *
server20 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  struct msghdr mes;
  struct iovec msgs[2];

  TEST_ASSERT (sock >= 0);

  msgs[0].iov_base = &(readBuf20[ sizeof(readBuf20) / 2  ]);
  msgs[0].iov_len = sizeof (readBuf20) / 2;

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = msgs;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;
  ssize_t lu = recvmsg (sock, &mes, 0);
  printf ("recvmsg -> %ld, errno: %d \n\n ", lu, errno);
  TEST_ASSERT_EQUAL (lu, sizeof (sendBuf20) / 2);
  TEST_ASSERT_EQUAL (mes.msg_flags & MSG_TRUNC, MSG_TRUNC);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server20: end \n\n ");

  return arg;
}

// TEST test MSG_PEEK behavior, and MSG_DONWAIT
#define BUF_LEN21 ((size_t) 1024)
static char readBuf21[BUF_LEN21];
static char sendBuf21[BUF_LEN21];
static void *
client21 (void *arg)
{
  sleep (1);
  int sock = CreateDgramConnect ();
  int status = 0;
  struct msghdr mes;
  struct iovec msg1;

  TEST_ASSERT (sock > 0);

  memset (sendBuf21, 21, sizeof(sendBuf21));
  msg1.iov_base = &sendBuf21;
  msg1.iov_len = sizeof(sendBuf21);

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = &msg1;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;

  ssize_t res =  sendmsg (sock, &mes, 0);
  printf ("sendmsg --> %ld, errno: %d \n \n ", res, errno);
  TEST_ASSERT_EQUAL (res, sizeof (sendBuf21));

  sleep (1);
  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client21: end \n\n ");

  return arg;
}

static void *
server21 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  struct msghdr mes;
  struct iovec msgs[2];

  TEST_ASSERT (sock >= 0);

  msgs[0].iov_base = readBuf21;
  msgs[0].iov_len = sizeof(readBuf21);

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = msgs;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;
  ssize_t lu = recvmsg (sock, &mes, MSG_PEEK);
  printf ("First recvmsg -> %ld, errno: %d \n\n ", lu, errno);
  TEST_ASSERT_EQUAL (lu, sizeof(sendBuf21));
  TEST_ASSERT_EQUAL (mes.msg_flags & MSG_TRUNC, 0);

  msgs[0].iov_base = readBuf21;
  msgs[0].iov_len = sizeof(readBuf21);

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = msgs;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;
  lu = recvmsg (sock, &mes, 0);
  printf ("Second recvmsg -> %ld, errno: %d \n\n ", lu, errno);
  TEST_ASSERT_EQUAL (lu, sizeof(sendBuf21));
  TEST_ASSERT_EQUAL (mes.msg_flags & MSG_TRUNC, 0);

  msgs[0].iov_base = readBuf21;
  msgs[0].iov_len = sizeof(readBuf21);

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = msgs;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;
  lu = recvmsg (sock, &mes, MSG_DONTWAIT);
  printf ("Third recvmsg -> %ld, errno: %d \n\n ", lu, errno);
  TEST_ASSERT_EQUAL (lu, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server21: end \n\n ");

  return arg;
}

// Some Timeouts...
// Receive Timeout
#define BUF_LEN22 ((size_t) 1024)
static char readBuf22[BUF_LEN22];
static void *
client22 (void *arg)
{
  printf ("Client22: end \n\n ");

  return arg;
}

static void *
server22 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  struct msghdr mes;
  struct iovec msgs[2];

  TEST_ASSERT (sock >= 0);
  struct timeval tiout;

  tiout.tv_sec = 3;
  tiout.tv_usec = 42;
  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  msgs[0].iov_base = readBuf22;
  msgs[0].iov_len = sizeof(readBuf22);

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = msgs;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;
  ssize_t lu = recvmsg (sock, &mes, 0);
  printf ("First recvmsg -> %ld, errno: %d \n\n ", lu, errno);
  TEST_ASSERT_EQUAL (lu, -1);
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server22: end \n\n ");

  return arg;
}

// Some Timeouts...
// Send Timeout
#define BUF_LEN23 ((size_t) 1024)
static char sendBuf23[BUF_LEN23];
static void *
client23 (void *arg)
{
  sleep (1);

  int sock = CreateDgramConnect ();
  int status = 0;
  struct msghdr mes;
  struct iovec msg1;

  TEST_ASSERT (sock > 0);
  struct timeval tiout;

  tiout.tv_sec = 1;
  tiout.tv_usec = 0;
  status = setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  while (true)
    {
      memset (sendBuf23, 23, sizeof(sendBuf23));
      msg1.iov_base = &sendBuf23;
      msg1.iov_len = sizeof(sendBuf23);

      mes.msg_name = 0;
      mes.msg_namelen = 0;
      mes.msg_iov = &msg1;
      mes.msg_iovlen = 1;
      mes.msg_control = 0;
      mes.msg_controllen = 0;
      mes.msg_flags = 0;

      ssize_t res = sendmsg (sock, &mes, 0);
      printf ("sendmsg --> %ld, errno: %d \n \n ", res, errno);

      if (res < 0)
        {
          break;
        }

    }
  TEST_ASSERT_EQUAL (errno, EAGAIN);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client23: end \n\n ");

  return arg;
}

static void *
server23 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  struct msghdr mes;
  struct iovec msgs[2];

  TEST_ASSERT (sock >= 0);
  struct timeval tiout;

  tiout.tv_sec = 2;
  tiout.tv_usec = 0;
  status = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  while (true)
    {
      msgs[0].iov_base = readBuf22;
      msgs[0].iov_len = sizeof(readBuf22);

      mes.msg_name = 0;
      mes.msg_namelen = 0;
      mes.msg_iov = msgs;
      mes.msg_iovlen = 1;
      mes.msg_control = 0;
      mes.msg_controllen = 0;
      mes.msg_flags = 0;
      ssize_t lu = recvmsg (sock, &mes, 0);
      printf ("recvmsg -> %ld, errno: %d \n\n ", lu, errno);
      if (lu < 0)
        {
          break;
        }
      sleep (2);
    }

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server23: end \n\n ");

  return arg;
}

// Client Write Until blocage, then block in a last send
// Server wait long then close.
#define BUF_LEN24 ((size_t) 1024)
static char sendBuf24[BUF_LEN24];
static void *
client24 (void *arg)
{
  sleep (1);
  int sock = CreateDgramConnect ();
  int status = 0;
  struct msghdr mes;
  struct iovec msg1;

  TEST_ASSERT (sock > 0);

  struct timeval tiout;

  tiout.tv_sec = 1;
  tiout.tv_usec = 0;
  status = setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (status, 0);

  while (true)
    {
      memset (sendBuf24, 24, sizeof(sendBuf24));
      msg1.iov_base = &sendBuf24;
      msg1.iov_len = sizeof(sendBuf24);

      mes.msg_name = 0;
      mes.msg_namelen = 0;
      mes.msg_iov = &msg1;
      mes.msg_iovlen = 1;
      mes.msg_control = 0;
      mes.msg_controllen = 0;
      mes.msg_flags = 0;

      ssize_t res = sendmsg (sock, &mes, 0);
      printf ("C24: sendmsg --> %ld, errno: %d \n \n ", res, errno);

      if (res < 0)
        {
          tiout.tv_sec = 0;
          tiout.tv_usec = 0;
          status = setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tiout, sizeof(tiout));
          TEST_ASSERT_EQUAL (status, 0);
          mes.msg_name = 0;
          mes.msg_namelen = 0;
          mes.msg_iov = &msg1;
          mes.msg_iovlen = 1;
          mes.msg_control = 0;
          mes.msg_controllen = 0;
          mes.msg_flags = 0;

          ssize_t res = sendmsg (sock, &mes, 0);
          printf ("C24: LAST sendmsg --> %ld, errno: %d \n \n ", res, errno);
          break;
        }

    }
  TEST_ASSERT_EQUAL (errno, ECONNREFUSED);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client24: end \n\n ");

  return arg;
}

static void *
server24 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;

  TEST_ASSERT (sock >= 0);

  sleep (30);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server24: end \n\n ");

  return arg;
}

//
//
#define BUF_LEN25 ((size_t) 1024)
static char sendBuf25[BUF_LEN25];
static void *
client25 (void *arg)
{
  sleep (1);
  int sock = CreateDgramConnect ();
  int status = 0;
  struct msghdr mes;
  struct iovec msg1;

  TEST_ASSERT (sock > 0);

  sleep (4);

  memset (sendBuf25, 25, sizeof(sendBuf25));
  msg1.iov_base = &sendBuf25;
  msg1.iov_len = sizeof(sendBuf25);

  mes.msg_name = 0;
  mes.msg_namelen = 0;
  mes.msg_iov = &msg1;
  mes.msg_iovlen = 1;
  mes.msg_control = 0;
  mes.msg_controllen = 0;
  mes.msg_flags = 0;

  ssize_t res = sendmsg (sock, &mes, 0);
  printf ("C25: sendmsg --> %ld, errno: %d \n \n ", res, errno);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client25: end \n\n ");

  return arg;
}

static void *
server25 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;

  TEST_ASSERT (sock >= 0);

  sleep (2);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);
  sock = CreateDgramBind ();
  TEST_ASSERT (sock >= 0);

  sleep (10);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server25: end \n\n ");

  return arg;
}

// TEST 26: lot of connexions lot of fd whitout close
static void *
client26 (void *arg)
{
  int status;
  struct sockaddr_un address;
  int sock = -1;
  std::vector<int> closeList;
  int first = 1;
  int first_socket = -1;

  do
    {
      sock = socket (AF_UNIX, SOCK_STREAM, 0);
      if (sock < 0)
        {
          break;
        }
      closeList.push_back (sock);

      memset (&address, 0, sizeof(address));
      address.sun_family = AF_UNIX;
      strcpy (address.sun_path, SOCK_PATH);

      status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
      printf ("Client26: connect -> %d \n ", status);

      if (first)
        {
          first = 0;
          first_socket = sock;
        }

    }
  while (true);

  printf ("Client26: out of do-while\n ");

  close (first_socket);

  printf ("Client26: end \n\n ");

  return arg;
}

static void *
server26 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_un address;
  std::vector<int> closeList;

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH);
  unlink (SOCK_PATH);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  closeList.push_back (sockin);

  do
    {
      fd_set readFd;
      int maxFd = (sock > sockin) ? sock : sockin;

      FD_ZERO (&readFd);

      FD_SET (sock, &readFd);
      FD_SET (sockin, &readFd);

      printf ("Server26: selecting ... \n ");

      status = select (1 + maxFd, &readFd, NULL, NULL, NULL);
      printf ("Server26: select -> %d \n ", status);
      if (status > 0)
        {
          if (FD_ISSET (sockin, &readFd))
            {
              // The client close it !
              break;
            }
          if (FD_ISSET (sock, &readFd))
            {
              // Incoming Connexion
              int newSock = accept (sock, NULL, NULL);
              if (newSock >= 0)
                {
                  closeList.push_back (newSock);
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

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server26: end \n ");

  return arg;
}

#define SOCK_PATH2 "/tmp/socket2"
// TEST 27: lot of connexions lot of fd whitout close using DATAGRAM
static void *
client27 (void *arg)
{
  printf ("Client27: start \n\n ");

  int status;
  struct sockaddr_un address;
  int sock = -1;
  int first_socket = socket (AF_UNIX, SOCK_STREAM, 0);
  std::vector<int> closeList;


  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH2);

  status = connect (first_socket, (struct sockaddr *) &address, SUN_LEN (&address));
  printf ("Client27: first connect -> %d \n ", status);

  do
    {
      sock = CreateDgramConnect ();
      if (sock < 0)
        {
          break;
        }
      closeList.push_back (sock);
    }
  while (true);

  printf ("Client27: out of do-while\n ");



  close (first_socket);
  CloseAll (closeList);
  printf ("Client27: end \n\n ");

  return arg;
}

static void *
server27 (void *arg)
{
  printf ("Server27: start \n\n ");

  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_un address;


  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, SOCK_PATH2);
  unlink (SOCK_PATH2);
  status = bind (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  do
    {
      int r = read (sockin, &status, sizeof(status));
      if (r <= 0)
        {
          break;
        }
    }
  while (true);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server27: end \n ");

  return arg;
}

// Shutdown test and getpeername and ...
static void *
client28 (void *arg)
{
  int sock = -1;
  int status = 0;
  int data = 42;
  struct sockaddr_un peerAddr, locAddr;

  sleep (1);
  sock = CreateDgramConnect ();
  TEST_ASSERT (sock > 0);


  socklen_t len = sizeof(struct sockaddr_un);
  memset (&peerAddr, 0, sizeof(peerAddr));
  status = getpeername (sock, (struct sockaddr *) &peerAddr, &len);
  printf ("client28: getpeername -> %d, errno %d  %s  \n\n ", status, errno, peerAddr.sun_path);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (peerAddr.sun_family, AF_UNIX);
  TEST_ASSERT_EQUAL (strcmp (peerAddr.sun_path, SOCK_PATH), 0);

  len = sizeof(struct sockaddr_un);
  memset (&locAddr, 0, len);
  status = getsockname (sock, (struct sockaddr *) &locAddr, &len);
  printf ("client28: getsockname -> %d, errno %d  %s  \n\n ", status, errno, locAddr.sun_path);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (locAddr.sun_family, AF_UNIX);
  TEST_ASSERT_EQUAL (strlen (locAddr.sun_path), 0);

  status = write (sock, &data, sizeof(data));
  TEST_ASSERT_EQUAL (status, sizeof(data));

  status = shutdown (sock, SHUT_RD);
  printf ("client28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = write (sock, &data, sizeof(data));
  TEST_ASSERT_EQUAL (status, sizeof(data));

  sleep (1);

  status = write (sock, &data, sizeof(data));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EPIPE);
  printf ("client28: write -> %d, errno %d\n\n ", status, errno);

  status = shutdown (sock, SHUT_RD);
  printf ("client28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = shutdown (sock, SHUT_WR);
  printf ("client28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = write (sock, &data, sizeof(data));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EPIPE);

  status = shutdown (sock, SHUT_WR);
  printf ("client28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  sleep (3);
  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  status = shutdown (sock, SHUT_WR);
  printf ("client28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EBADF);

  printf ("Client28: end \n\n ");

  return arg;
}

static void *
server28 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  int data = 0;
  struct sockaddr_un peerAddr, locAddr;

  TEST_ASSERT (sock >= 0);

  socklen_t len = sizeof(struct sockaddr_un);
  memset (&peerAddr, 0, sizeof(peerAddr));
  status = getpeername (sock, (struct sockaddr *) &peerAddr, &len);
  printf ("server28: getpeername -> %d, errno %d  %s  \n\n ", status, errno, peerAddr.sun_path);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ENOTCONN);

  len = sizeof(struct sockaddr_un);
  memset (&locAddr, 0, len);
  status = getsockname (sock, (struct sockaddr *) &locAddr, &len);
  printf ("server28: getsockname -> %d, errno %d  %s  \n\n ", status, errno, locAddr.sun_path);
  TEST_ASSERT_EQUAL (status, 0);
  TEST_ASSERT_EQUAL (locAddr.sun_family, AF_UNIX);
  TEST_ASSERT_EQUAL (strcmp (locAddr.sun_path, SOCK_PATH), 0);

  status = shutdown (sock, SHUT_WR);
  printf ("server28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = shutdown (sock, SHUT_WR);
  printf ("server28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = read (sock, &data, sizeof(data));
  TEST_ASSERT_EQUAL (status, sizeof(data));

  status = read (sock, &data, sizeof(data));
  TEST_ASSERT_EQUAL (status, sizeof(data));

  status = shutdown (sock, SHUT_RD);
  printf ("server28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  status = read (sock, &data, sizeof(data));
  TEST_ASSERT_EQUAL (status, 0);

  sleep (5);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  status = shutdown (sock, SHUT_WR);
  printf ("server28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, EBADF);

  sock = socket (AF_UNIX, SOCK_DGRAM, 0);
  status = shutdown (sock, SHUT_WR);
  printf ("server28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);
  status = shutdown (sock, SHUT_RD);
  printf ("server28: shutdown -> %d, errno %d\n\n ", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server28: end \n\n ");

  return arg;
}

#define BUF_LEN29 ((size_t) 1024)
static char sendBuf29[BUF_LEN29];

// Test if memory leak when NS3 stop simulation with existing processes and threads
static void *
client29 (void *arg)
{
  sleep (1);
  int sock = CreateDgramConnect ();
  struct msghdr mes;
  struct iovec msg1;

  TEST_ASSERT (sock > 0);

  while (true)
    {
      memset (sendBuf29, 29, sizeof(sendBuf29));
      msg1.iov_base = &sendBuf29;
      msg1.iov_len = sizeof(sendBuf29);

      mes.msg_name = 0;
      mes.msg_namelen = 0;
      mes.msg_iov = &msg1;
      mes.msg_iovlen = 1;
      mes.msg_control = 0;
      mes.msg_controllen = 0;
      mes.msg_flags = 0;

      ssize_t res = sendmsg (sock, &mes, 0);
      if (res < 0)
        {
          sleep (1);
          close (sock);
          break;
        }
    }

  return arg;
}
static void *
server29 (void *arg)
{
  printf ("Server29: start \n\n ");
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status = 0;

  TEST_ASSERT (sock >= 0);

  sleep (30000);

//  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  // unlink (SOCK_PATH);

  printf ("Server29: end \n\n ");

  return arg;
}

// test30
// test readv(2) and writev(2)
#define BUF_LEN30 ((size_t) 1024)
static char readBuf30[BUF_LEN30];
static char sendBuf30[BUF_LEN30];
static void *
client30 (void *arg)
{
  int sock = -1;
  int status = 0;
  struct msghdr msghdr;
  struct iovec iov[2];

  printf ("client30: start\n");
  sleep (1);
  sock = socket (AF_UNIX, SOCK_DGRAM, 0);
  TEST_ASSERT (sock > 0);

  memset (sendBuf30, 30, sizeof(sendBuf30));

  iov[0].iov_base = &sendBuf30;
  iov[0].iov_len = sizeof(sendBuf30);
  iov[1].iov_base = &sendBuf30;
  iov[1].iov_len = sizeof(sendBuf30);

  msghdr.msg_name = (void*) PrepAddr ();
  msghdr.msg_namelen = sizeof(sockaddr_un);
  msghdr.msg_iov = iov;
  msghdr.msg_iovlen = 1;
  msghdr.msg_control = 0;
  msghdr.msg_controllen = 0;
  msghdr.msg_flags = 0;

  ssize_t res =  sendmsg (sock, &msghdr, 0);
  printf ("sendmsg --> %ld, errno: %d \n \n ", res, errno);
  TEST_ASSERT_EQUAL (res, sizeof(sendBuf30) * msghdr.msg_iovlen);

  sleep (1);
  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client30: end \n\n ");

  return arg;
}

static void *
server30 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int status;
  struct msghdr msghdr;
  struct iovec iov[3];

  printf ("server30: start\n");
  TEST_ASSERT (sock >= 0);

  iov[0].iov_base = &(readBuf30[sizeof (readBuf30) / 2  ]);
  iov[0].iov_len = sizeof (readBuf30) / 2;
  iov[1].iov_base = readBuf30;
  iov[1].iov_len = sizeof (readBuf30) / 2;
  iov[2].iov_base = readBuf30;
  iov[2].iov_len = sizeof (readBuf30);

  errno = 0;
  ssize_t lu = readv (sock, iov, 3);
  printf ("readv -> %ld, errno: %d \n\n ", lu, errno);
  TEST_ASSERT_EQUAL (lu, sizeof (sendBuf30));

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server30: end \n\n ");

  return arg;
}

static void
launch (void * (*clientStart)(void *), void *(*serverStart)(void *))
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

  if ((1 < 2))
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
      launch (client15, server15);

      // DGRAMs
      launch (client16, server16);
      launch (client17, server17);
      launch (client18, server18);
      launch (client19, server19);
      launch (client20, server20);
      launch (client21, server21);
      launch (client22, server22);
      launch (client23, server23);

      launch (client24, server24);
      launch (client25, server25);
      launch (client27, server27);
      launch (client28, server28);
      launch (client26, server26);
      launch (client30, server30);
      // should be the last test: dunno...
      launch (client29, server29);
    }
  else
    {

    }

  printf ("That's All Folks ....\n \n ");
  fflush (stdout);
  fclose (stdout);
  sleep (1);

  return 0;
}
