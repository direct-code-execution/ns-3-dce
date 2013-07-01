#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include "test-macros.h"
#include <sys/un.h>

static bool
test_poll_read (int fd, int timeOutSec, bool needSuccess)
{
  struct pollfd fds;
  int ret = -1;

  fds.fd = fd;
  fds.events = POLLIN;
  fds.revents = 0;

  ret = poll (&fds, 1, timeOutSec * 1000);

  if (needSuccess)
    {
      return (ret == 1) && (fds.revents & POLLIN);
    }
  else
    {
      return (ret == 0);
    }
}

// test, poll over stdin
static void
test_poll_stdin (void)
{
  // check only if we use tty
  if (isatty (0))
    {
      TEST_ASSERT (test_poll_read (0, 1, false));
    }
}

static bool
wait_write (int fd, int timeOutSec)
{
  struct pollfd fds;
  int ret = -1;

  fds.fd = fd;
  fds.events = POLLOUT;
  fds.revents = 0;

  ret = poll (&fds, 1, timeOutSec * 1000);

  return (ret == 1) && (fds.revents & POLLOUT);
}

static void
test_poll_write (int fd, int timeOutSec, bool needSuccess)
{
  struct pollfd fds;
  int ret = -1;

  fds.fd = fd;
  fds.events = POLLOUT;
  fds.revents = 0;

  ret = poll (&fds, 1, timeOutSec * 1000);
  if (needSuccess)
    {
      TEST_ASSERT_EQUAL (ret, 1);
      TEST_ASSERT (fds.revents & POLLOUT);
    }
  else
    {
      TEST_ASSERT_EQUAL (ret, 0);
    }
}

// test, poll over stdout
static void
test_poll_stdout (void)
{
  test_poll_write (1, 1, true);
}

#undef BUF_LEN
#define BUF_LEN ((size_t) 1024 * 1000)

static char* readBuf[BUF_LEN];
static char* writeBuf[BUF_LEN];

// Test, poll over TCP Socket.
static void *
client1 (void *arg)
{
  int sock, sock2;
  struct sockaddr_in addr;
  int res;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);
  sock2 = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock2 >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1234);

  sleep (1);

  res = connect (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);
  sleep (1);
  res = connect (sock2, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);

  // Can I Write ?
  test_poll_write (sock, 1, true);
  // Read ?
  TEST_ASSERT (test_poll_read (sock, 1, false));
  sleep (5);
  // Read ?
  TEST_ASSERT (test_poll_read (sock, 10, true));

  res = read (sock2, readBuf, BUF_LEN);
  printf ("Client1: read -> %d \n ",res);

  close (sock);
  close (sock2);

  printf ("Client1: end\n ");

  return arg;
}

static void *
server1 (void *arg)
{
  int sock, sockin, sockin2;
  struct sockaddr_in addr;
  int res;
  int on = 1;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1234);
  res = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  printf ("Server1: bind -> %d, errno:%d\n ", res, errno);
  TEST_ASSERT_EQUAL (res, 0);

  res = listen (sock, 10);
  TEST_ASSERT_EQUAL (res, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);
  sleep (1);
  sockin2 = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin2 >= 0);

  sleep (4);
  res = write (sockin, writeBuf, 1024);
  sleep (10);

  struct timeval tiout;
  tiout.tv_sec = 2;
  tiout.tv_usec = 0;
  res = setsockopt (sockin, SOL_SOCKET, SO_SNDTIMEO, &tiout, sizeof(tiout));
  TEST_ASSERT_EQUAL (res, 0);

  while (wait_write (sockin, 10))
    { //  Fill the buffer
      res = write (sockin, writeBuf, BUF_LEN);
      printf ("Server1: write->%d\n ",res);
      fflush (stdout);
    }

  // buffer is full
  test_poll_write (sockin, 0, false);

  res = write (sockin2, writeBuf, 1024);

  close (sock);
  close (sockin);
  close (sockin2);

  sleep (1);

  printf ("Server1: end\n ");

  return arg;
}

#undef BUF_LEN
#define BUF_LEN ((size_t) 1024)

// Test, poll over UDP Socket.
static void *
client2 (void *arg)
{
  int sock;
  struct sockaddr_in addr;
  int res;

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);
  addr.sin_family = AF_INET;
  addr.sin_port = htons (1235);

  sleep (1);

  res = sendto (sock, writeBuf, BUF_LEN, 0, (const struct sockaddr *) &addr, sizeof(addr));

  TEST_ASSERT_EQUAL (res, BUF_LEN);

  close (sock);

  printf ("Client2: end\n ");

  return arg;
}

static void *
server2 (void *arg)
{
  int sock;
  struct sockaddr_in addr;
  int res;
  socklen_t len;
  int on = 1;

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  // TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1235);
  res = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  TEST_ASSERT_EQUAL (res, 0);
  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);

  sleep (2);

  // Read ?
  TEST_ASSERT (test_poll_read (sock, 2, true));

  len = sizeof(addr);
  res = recvfrom (sock, readBuf, BUF_LEN, 0, (struct sockaddr *) &addr, &len);
  printf ("Server2: recvfrom -> %d\n ", res);
  TEST_ASSERT_EQUAL (res, BUF_LEN);
  sleep (1);

  close (sock);

  printf ("Server2: end\n ");

  return arg;
}

// Test, poll over TCP Socket do poll before accept.
static void *
client3 (void *arg)
{
  int sock;
  struct sockaddr_in addr;
  int res;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1236);

  sleep (1);

  res = connect (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);

  sleep (1);

  close (sock);

  printf ("Client3: end\n ");

  return arg;
}

static void *
server3 (void *arg)
{
  int sock, sockin;
  struct sockaddr_in addr;
  int res;
  int on = 1;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1236);
  res = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  printf ("Server3: bind -> %d, errno:%d\n ", res, errno);
  TEST_ASSERT_EQUAL (res, 0);

  res = listen (sock, 1);
  TEST_ASSERT_EQUAL (res, 0);

  TEST_ASSERT (test_poll_read (sock, 10, true));

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);
  sleep (1);

  close (sock);
  close (sockin);

  printf ("Server3: end\n ");

  return arg;
}

static void
test_poll_stdout_stdin (void)
{
  struct pollfd duo[2];
  int ret = -1;
  time_t before, after;

  memset (duo, 0, sizeof (duo));
  // check only if we use tty
  if (isatty (0))
    {
      duo[0].fd = 0;
      duo[0].events = POLLIN;
      duo[0].revents = 0;
    }

  duo[1].fd = 1;
  duo[1].events = POLLOUT;
  duo[1].revents = 0;

  // should immediately return 1 because stdout is writable.
  before = time (NULL);
  ret = poll (duo, 2, 10 * 1000);
  after = time (NULL);

  printf ("After poll -> %d before time %ld after time %ld errno:%d \n ", ret, before,after, errno);

  // time difference must not exceed 1 second
  TEST_ASSERT ((1 == ret) &&  ((after - before) <= 1));

}

// Some error cases
// Test POLLNVAL
/*
static void
test_nval (void)
{
  int d = dup (0);
  struct pollfd fd;
  int ret = -1;

  close (d);
  printf ("test_nval: fd:%d\n",d);
  fd.fd = d;
  fd.events = POLLIN;
  fd.revents = 0;

  ret = poll (&fd, 1, 10);

  TEST_ASSERT (1 == ret);
  TEST_ASSERT (fd.revents == POLLNVAL);

}
*/

// Test, POLLHUP not set on AF_INET socket on close but pollin yes !
static void *
client4 (void *arg)
{
  int sock;
  struct sockaddr_in addr;
  int res;
  struct pollfd fd;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1237);

  sleep (1);
  res = connect (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);
  sleep (1);

  fd.fd = sock;
  fd.events = POLLIN | POLLOUT;
  fd.revents = 0;
  printf ("Client4: polling\n");
  res = poll (&fd, 1, 0);
  printf ("res %d, revents : %d \n ", res, fd.revents);
  TEST_ASSERT (1 == res);
  TEST_ASSERT (fd.revents & POLLOUT);

  sleep (2);

  fd.fd = sock;
  fd.events = POLLIN | POLLOUT;
  fd.revents = 0;
  printf ("Client4: polling\n");
  res = poll (&fd, 1, 0);
  printf ("res %d, revents : %d \n ", res, fd.revents);
  TEST_ASSERT (1 == res);
  TEST_ASSERT (fd.revents & POLLIN);
  TEST_ASSERT (fd.revents & POLLOUT);

  close (sock);

  printf ("Client4: end\n ");

  return arg;
}

static void *
server4 (void *arg)
{
  int sock, sockin;
  struct sockaddr_in addr;
  int res;
  int on = 1;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);
  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1237);
  res = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);

  res = listen (sock, 1);
  TEST_ASSERT_EQUAL (res, 0);
  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  sleep (2);

  close (sock);
  close (sockin);

  printf ("Server4: end\n ");

  return arg;
}

#define SOCK_PATH "/tmp/socket"
// TEST 5: poll over unix socket
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

  status = connect (sock, (struct sockaddr *) &address, SUN_LEN (&address));
  TEST_ASSERT_EQUAL (status, 0);

  sleep (2);
  printf ("Client5: fd: %d\n", sock);

  struct pollfd fd;

  fd.fd = sock;
  fd.events = POLLIN | POLLOUT;
  fd.revents = 0;

  status = poll (&fd, 1, -1);
  printf ("Client5: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);

  TEST_ASSERT (status == 1);
  TEST_ASSERT (fd.revents & POLLIN);
  TEST_ASSERT (fd.revents & POLLOUT);
//  TEST_ASSERT(fd.revents & POLLHUP);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("Client5: end \n\n");

  return arg;
}

static void *
server5 (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_un address;

  printf ("Server5: BUF_LEN:%ld\n", BUF_LEN);

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
  printf ("Server5: fd: %d\n", sockin);

  status = send (sockin, writeBuf, BUF_LEN, 0);
  TEST_ASSERT (status >= 0);

  sleep (1);

  status = close (sockin);
  TEST_ASSERT_EQUAL (status, 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  unlink (SOCK_PATH);

  printf ("Server5: end \n");

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

// Test, poll using unix socket datagram,
static void *
client6 (void *arg)
{
  sleep (1);
  int sock = CreateDgramConnect ();
  int res = 1;
  int status = -1;
  TEST_ASSERT (sock >= 0);

  sleep (2);

  struct pollfd fd;

  fd.fd = sock;
  fd.events = POLLOUT;
  fd.revents = 0;
  status = poll (&fd, 1, 2000);
  printf ("Client6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT (fd.revents & POLLOUT);

  status = write (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  sleep (6);

  fd.fd = sock;
  fd.events = POLLOUT;
  fd.revents = 0;
  status = poll (&fd, 1, -1);
  printf ("Client6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT (fd.revents & POLLOUT);

  status = write (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  sleep (2);
  fd.fd = sock;
  fd.events = POLLOUT;
  fd.revents = 0;
  status = poll (&fd, 1, -1);
  printf ("Client6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT (fd.revents & POLLOUT);

  status = write (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ECONNREFUSED);

  close (sock);

  sleep (1);
  sock = CreateDgramConnect ();
  TEST_ASSERT (sock >= 0);

  fd.fd = sock;
  fd.events = POLLOUT;
  fd.revents = 0;
  status = poll (&fd, 1, -1);
  printf ("Client6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT (fd.revents & POLLOUT);

  status = write (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  close (sock);

  printf ("Client6: end\n ");

  return arg;
}

static void *
server6 (void *arg)
{
  unlink (SOCK_PATH);
  int sock = CreateDgramBind ();
  int res = 1;
  int status = -1;

  TEST_ASSERT (sock >= 0);

  struct pollfd fd;

  fd.fd = sock;
  fd.events = POLLIN;
  fd.revents = 0;
  status = poll (&fd, 1, 2000);
  printf ("Server6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT ((fd.revents & POLLIN) == 0);

  sleep (3);

  fd.fd = sock;
  fd.events = POLLIN;
  fd.revents = 0;
  status = poll (&fd, 1, 1000);
  printf ("Server6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT (fd.revents & POLLIN);

  status = read (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  fd.fd = sock;
  fd.events = POLLIN;
  fd.revents = 0;
  status = poll (&fd, 1, -1);
  printf ("Server6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT (fd.revents & POLLIN);

  status = read (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  sleep (1);
  close (sock);

  unlink (SOCK_PATH);
  sock = CreateDgramBind ();
  TEST_ASSERT (sock >= 0);

  fd.fd = sock;
  fd.events = POLLIN;
  fd.revents = 0;
  status = poll (&fd, 1, -1);
  printf ("Server6: poll -> %d, revents : %d, errno %d \n ", status, fd.revents, errno);
  TEST_ASSERT (fd.revents & POLLIN);

  status = read (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  close (sock);
  unlink (SOCK_PATH);

  printf ("Server6: end\n ");

  return arg;
}

// Test, exit while polling.
static void *
client_last (void *arg)
{
  struct pollfd theInput;

  // check only if we use tty
  if (!isatty (0))
    {
      return arg;
    }

  theInput.fd = 0;
  theInput.events = POLLIN;
  theInput.revents = 0;

  int fd2 = socket (AF_UNIX, SOCK_STREAM, 0);
  int fd = open ("F", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);

  int status = poll (&theInput, 1, -1);

  TEST_ASSERT (false);

  printf ("fd %d fd2 %d status %d\n", fd,fd2, status);

  return arg;
}

static void *
server_last (void *arg)
{
  sleep (2);
  exit (0);
  TEST_ASSERT (false);

  return arg;
}

static void
launch (void *(*clientStart)(void *), void *(*serverStart)(void *))
{
  int status;
  pthread_t theClient;
  pthread_t theServer;

  printf ("launch\n ");

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

// XXX : add test POLLERR, POLLHUP, POLLVAL !
int
main (int argc, char *argv[])
{
  signal (SIGPIPE, SIG_IGN);
  //goto C4;
  test_poll_stdin ();
  test_poll_stdout_stdin ();
  test_poll_stdout ();
  launch (client1, server1);
  launch (client2, server2);
  launch (client3, server3);
  // test_nval ();

  launch (client5, server5);
  launch (client4, server4);
  launch (client6, server6);

  launch (client_last, server_last);


  printf ("test-poll end.\n ");
  fflush (stdout);
  fflush (stderr);
  sleep (1);

  return 0;
}
