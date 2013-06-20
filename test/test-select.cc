#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
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
#include <sys/un.h>

#include "test-macros.h"

// test, that select () with timeout={0,0} exits immediately
static void
test_select_null_null (void)
{
  // create timer to check whether a followixng call to select() blocks
  int timerfd = timerfd_create (CLOCK_MONOTONIC, 0);
  TEST_ASSERT_UNEQUAL (timerfd, -1);

  // arm timer to fire in 5 seconds
  struct itimerspec new_value;
  new_value.it_value.tv_sec = 5;
  new_value.it_value.tv_nsec = 0;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;
  int status = timerfd_settime (timerfd, 0, &new_value, 0);
  TEST_ASSERT_EQUAL (status, 0);

  // prepare fd_set to select() on it
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (timerfd, &fds);

  // doing select() with timeout = {0, 0}
  struct timeval timeout =
  {
    0, 0
  };
  int nfds = select (timerfd + 1, &fds, NULL, NULL, &timeout);
  // no fds must be ready and select() should complete without errors
  TEST_ASSERT_EQUAL (nfds, 0);

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  // select(2):
  // Some  code  calls  select() with all three sets empty, nfds zero, and a
  // non-NULL timeout as a fairly portable way to sleep with subsecond
  // precision.
  nfds = select (0, &fds, NULL, NULL, &timeout);
  // no fds must be ready and select() should complete without errors
  TEST_ASSERT_EQUAL (nfds, 0);

  close (timerfd);
}

static bool
test_select_read (int fd, int timeOutSec, bool needSuccess)
{
  fd_set readFd;
  struct timeval timeOut;
  int maxFd = fd + 1;
  int ret = 0;

  timeOut.tv_sec = timeOutSec;
  timeOut.tv_usec = 0;
  FD_ZERO (&readFd);
  FD_SET (fd, &readFd);

  ret = select (maxFd, &readFd, NULL, NULL, (timeOutSec > 0) ? &timeOut : NULL);
  printf ("read select -> %d, errno:%d\n\n ", ret, errno);
  if (needSuccess)
    {
      return (ret == 1) && (FD_ISSET (fd, &readFd));
    }
  else
    {
      return (ret <= 0);
    }
}

// test, select over stdin
static void
test_select_stdin (void)
{
  // check only if we use tty
  if (isatty (0))
    {
      TEST_ASSERT (test_select_read (0, 1, false));
    }
}

static bool
wait_write (int fd, int timeOutSec)
{
  fd_set writeFd;
  struct timeval timeOut;
  int maxFd = fd + 1;
  int ret = 0;

  timeOut.tv_sec = timeOutSec;
  timeOut.tv_usec = 0;
  FD_ZERO (&writeFd);
  FD_SET (fd, &writeFd);

  ret = select (maxFd, NULL, &writeFd, NULL, &timeOut);
  return (1 == ret);
}

static void
test_select_write (int fd, int timeOutSec, bool needSuccess)
{
  fd_set writeFd;
  struct timeval timeOut;
  int maxFd = fd + 1;
  int ret = 0;

  timeOut.tv_sec = timeOutSec;
  timeOut.tv_usec = 0;
  FD_ZERO (&writeFd);
  FD_SET (fd, &writeFd);

  ret = select (maxFd, NULL, &writeFd, NULL, &timeOut);
  if (needSuccess)
    {
      TEST_ASSERT_EQUAL (ret, 1);
      TEST_ASSERT (FD_ISSET (fd, &writeFd));
    }
  else
    {
      TEST_ASSERT (ret <= 0);
    }
}

// test, select over stdout
static void
test_select_stdout (void)
{
  test_select_write (1, 1, true);
}

#undef BUF_LEN
#define BUF_LEN ((size_t) 1024 * 1000)

static char* readBuf[BUF_LEN];
static char* writeBuf[BUF_LEN];

// Test, select over TCP Socket.
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
  test_select_write (sock, 1, true);
  // Read ?
  TEST_ASSERT (test_select_read (sock, 1, false));
  sleep (5);
  // Read ?
  TEST_ASSERT (test_select_read (sock, 10, true));

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

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1234);

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
  test_select_write (sockin, 0, false);

//  res = write (sockin2, writeBuf, 1024);

  close (sock);
  close (sockin);
  close (sockin2);

  sleep (1);

  printf ("Server1: end\n ");

  return arg;
}

#undef BUF_LEN
#define BUF_LEN ((size_t) 1024)

// Test, select over UDP Socket.
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

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  // TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1235);

  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);

  sleep (2);

  // Read ?
  TEST_ASSERT (test_select_read (sock, 2, true));

  len = sizeof(addr);
  res = recvfrom (sock, readBuf, BUF_LEN, 0, (struct sockaddr *) &addr, &len);
  printf ("Server2: recvfrom -> %d\n ", res);
  TEST_ASSERT_EQUAL (res, BUF_LEN);
  sleep (1);

  close (sock);

  printf ("Server2: end\n ");

  return arg;
}

// Test, select over TCP Socket do select before accept.
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

  // sleep (2);

  res = read (sock, readBuf, BUF_LEN);
  printf ("client3: read -> %d \n ", res);

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

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1236);

  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  printf ("Server3: bind -> %d, errno:%d\n ", res, errno);
  TEST_ASSERT_EQUAL (res, 0);

  res = listen (sock, 1);
  TEST_ASSERT_EQUAL (res, 0);

  TEST_ASSERT (test_select_read (sock, 10, true));

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);
  sleep (5);

  close (sock);
  close (sockin);

  printf ("Server3: end\n ");

  return arg;
}

// Test select after remote TCPIP close
static void *
client4 (void *arg)
{
  int sock;
  struct sockaddr_in addr;
  int res;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1237);

  sleep (1);

  res = connect (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);

  sleep (2);

  // Select over remote closed socket
  test_select_read (sock, 1, true);
  test_select_write (sock, 1, true);

  close (sock);

  // Select over closed socket
  test_select_read (sock, 1, false);
  test_select_write (sock, 1, false);


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

  // Select over just created socket
  test_select_read (sock, 1, true);
  test_select_write (sock, 1, true);

  res = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (1237);
  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  printf ("Server4: bind -> %d, errno:%d\n ", res, errno);
  TEST_ASSERT_EQUAL (res, 0);

  res = listen (sock, 1);
  TEST_ASSERT_EQUAL (res, 0);

  TEST_ASSERT (test_select_read (sock, 10, true));

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);
  sleep (1);

  close (sock);
  close (sockin);

  printf ("Server4: end\n ");

  return arg;
}

#define SOCK_PATH "/tmp/socket"
// Test select just after creation and after remote local socket close then after closed socket.
static void *
client5 (void *arg)
{
  int sock;
  struct sockaddr_un addr;
  int res;

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  // Select over just created socket
  test_select_read (sock, 1, true);
  test_select_write (sock, 1, true);

  memset (&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy (addr.sun_path, SOCK_PATH);

  sleep (1);

  res = connect (sock, (struct sockaddr *) &addr, sizeof(addr));
  TEST_ASSERT_EQUAL (res, 0);

  sleep (2);

  // Select over remote closed socket
  test_select_read (sock, 1, true);
  test_select_write (sock, 1, true);

  close (sock);

  // Select over closed socket
  test_select_read (sock, 1, false);
  test_select_write (sock, 1, false);

  printf ("Client5: end\n ");

  return arg;
}

static void *
server5 (void *arg)
{
  int sock, sockin;
  struct sockaddr_un addr;
  int res;

  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  memset (&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy (addr.sun_path, SOCK_PATH);
  unlink (SOCK_PATH);

  res = bind (sock, (struct sockaddr *) &addr, sizeof(addr));
  printf ("Server5: bind -> %d, errno:%d\n ", res, errno);
  TEST_ASSERT_EQUAL (res, 0);

  res = listen (sock, 1);
  TEST_ASSERT_EQUAL (res, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);
  sleep (1);

  close (sock);
  close (sockin);

  unlink (SOCK_PATH);

  printf ("Server5: end\n ");

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

// Test, select using unix socket datagram,
static void *
client6 (void *arg)
{
  sleep (1);
  int sock = CreateDgramConnect ();
  int res = 1;
  int status = -1;
  TEST_ASSERT (sock >= 0);

  sleep (2);

  test_select_write (sock, 2, true);

  status = write (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  sleep (6);
  test_select_write (sock, 0, true);
  status = write (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  sleep (2);
  test_select_write (sock, 0, true);
  status = write (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, -1);
  TEST_ASSERT_EQUAL (errno, ECONNREFUSED);

  close (sock);

  sleep (1);
  sock = CreateDgramConnect ();
  TEST_ASSERT (sock >= 0);
  test_select_write (sock, 0, true);
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
  TEST_ASSERT (test_select_read (sock, 2, false));

  sleep (3);

  TEST_ASSERT (test_select_read (sock, 1, true));
  status = read (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  TEST_ASSERT (test_select_read (sock, 0, true));
  status = read (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  sleep (1);

  close (sock);

  unlink (SOCK_PATH);
  sock = CreateDgramBind ();
  TEST_ASSERT (sock >= 0);

  TEST_ASSERT (test_select_read (sock, 0, true));
  status = read (sock, &res, sizeof (res));
  TEST_ASSERT_EQUAL (status, sizeof (res));

  close (sock);
  unlink (SOCK_PATH);

  printf ("Server6: end\n ");

  return arg;
}

static void
test_select_stdout_stdin (void)
{
  fd_set rFd;
  fd_set wFd;
  struct timeval timeOut;
  int ret = 0;
  time_t before, after;
  int sock = 0;

  timeOut.tv_sec = 10;
  timeOut.tv_usec = 0;
  // check only if we use tty
  FD_ZERO (&rFd);
  if (isatty (0))
    {
      FD_SET (sock, &rFd);
    }
  FD_ZERO (&wFd);
  FD_SET (1, &wFd);

  // should immediately return 1 because stdout is writable.
  before = time (NULL);
  ret = select (1 + 1, &rFd, &wFd, NULL, &timeOut);
  after = time (NULL);

  printf ("After select -> %d before time %ld after time %ld errno:%d, \n ", ret, before, after, errno);

  // time difference must not exceed 1 second
  TEST_ASSERT ((1 == ret) &&  ((after - before) <= 1));

  close (sock);
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

// test, that select () returns a fd if it is available for writing when it is not availabe for reading
// solved with else mustWait=false and take out the else in the if for writefds
static void test_select_rfds_wfds (void)
{
  int sockfd = socket (AF_INET, SOCK_DGRAM, 0);

  // prepare fd_set to select() on it
  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET (sockfd, &rfds);

  // prepare fd_set to select() on it
  fd_set wfds;
  FD_ZERO (&wfds);
  FD_SET (sockfd, &wfds);

  // doing select() with timeout = {0, 0}
  struct timeval timeout =
  {
    5, 0
  };
  int nfds = select (sockfd + 1, &rfds, &wfds, NULL, &timeout);
  close (sockfd);
  // no fds must be ready and select() should complete without errors
  TEST_ASSERT_EQUAL (nfds, 1);
  TEST_ASSERT (FD_ISSET (sockfd, &wfds));
}

// test, that select () returns correctly if there is two fds for reading and only one available
// solved with else mustWait=false
static void test_select_rfds (void)
{
  // create timer to check whether a following call to select() blocks
  int timerfd = timerfd_create (CLOCK_MONOTONIC, 0);
  TEST_ASSERT_UNEQUAL (timerfd, -1);

  // arm timer to fire in 5 seconds
  struct itimerspec new_value;
  new_value.it_value.tv_sec = 5;
  new_value.it_value.tv_nsec = 0;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;
  int status = timerfd_settime (timerfd, 0, &new_value, 0);
  TEST_ASSERT_EQUAL (status, 0);

  int filefd = open ("X1", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);

  // prepare fd_set to select() on it
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (timerfd, &fds);
  FD_SET (filefd, &fds);

  // doing select() with timeout = {0, 0}
  struct timeval timeout =
  {
    6, 0
  };
  int nfds = select (timerfd > filefd ? timerfd + 1 : filefd + 1, &fds, NULL, NULL, &timeout);
  close (timerfd);
  close (filefd);
  // no fds must be ready and select() should complete without errors
  TEST_ASSERT_EQUAL (nfds, 1);
}

// test, that select () with timeout not zero works correctly
// if select quits with timeout returns 0 and fds not set
static void test_select_timeout (void)
{
  int timerfd = timerfd_create (CLOCK_MONOTONIC, 0);
  TEST_ASSERT_UNEQUAL (timerfd, -1);

  // arm timer to fire in 5 seconds
  struct itimerspec new_value;
  new_value.it_value.tv_sec = 5;
  new_value.it_value.tv_nsec = 0;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;
  int status = timerfd_settime (timerfd, 0, &new_value, 0);
  TEST_ASSERT_EQUAL (status, 0);

  // prepare fd_set to select() on it
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (timerfd, &fds);

  // doing select() with timeout = {3, 0}
  struct timeval timeout = {3, 0};
  int nfds = select (timerfd + 1, &fds, NULL, NULL, &timeout);
  
  // no fds must be ready and select() should complete without errors
  TEST_ASSERT_EQUAL (nfds, 0);
  TEST_ASSERT_EQUAL (FD_ISSET (timerfd, &fds), 0)
  
  close (timerfd);
  
}

int
main (int argc, char *argv[])
{
  signal (SIGPIPE, SIG_IGN);


  if (1 < 2)
    {
      test_select_stdin ();
      test_select_stdout_stdin ();
      test_select_null_null ();
      test_select_stdout ();
      test_select_rfds_wfds ();
      test_select_rfds ();
      test_select_timeout();
      launch (client1, server1);
      launch (client2, server2);
      launch (client3, server3);
      launch (client4, server4);
      launch (client5, server5);
      launch (client6, server6);
    }

  printf ("test-select end.\n ");
  fflush (stdout);
  fflush (stderr);
  sleep (1);
  return 0;
}
