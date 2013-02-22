/**
* \brief Test the behavior of close a fd after a fork in multiple case.
*
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "test-macros.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

static
void fill_addr (struct sockaddr_in &addr, int port)
{
  int res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
}

void*
second_thread1 (void *arg)
{
  int sock = *((int*)arg);
  printf ("second_thread1: enter\nsecond_thread1:sleeping 1 sec\n");
  sleep (1);
  printf ("second_thread1: closing fd:%d\n",sock);
  int status = close (sock);
  printf ("second_thread1: close -> %d, errno %d\n", status, errno);
  int sock2 = socket (AF_INET, SOCK_STREAM, 0);
  printf ("second_thread1: create new socket: %d\n", sock2);
  close (sock2);

  return 0;
}

void
father1 ()
{
  int status = -1;
  int sock = -1;
  struct sockaddr_in ad;
  char buf[1024];
  pthread_t brother;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  fill_addr (ad, 2000);

  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  printf ("father1: connect -> %d\n",status);
  TEST_ASSERT_EQUAL (status, 0)

  status = pthread_create (&brother, NULL, second_thread1, (void*)&sock);
  TEST_ASSERT_EQUAL (status, 0);

//  printf ("father1: sleeping 5s\n");
// sleep (5);

  if (true)
    {
      struct pollfd p;

      p.fd = sock;
      p.events = POLLIN;
      p.revents = 0;
      printf ("father1: about to poll on sock fd:%d with a timeout of -5 s \n",sock);
      status = poll (&p, 1, -5000);
      printf ("father1: poll result : %d, revents %d POLLIN %d, POLLOUT %d, POLLHUP %d POLLERR %d POLLNVAL %d\n",
              status, p.revents,  POLLIN & p.revents, POLLOUT & p.revents,  POLLHUP & p.revents,
              POLLERR & p.revents,  POLLNVAL & p.revents);
    }
  else
    {
      printf ("father1: about to read on sock fd:%d \n",sock);
      status = read (sock, buf, sizeof(buf));
      printf ("father1: read result : %d\n",status);
    }
  int sock2 = socket (AF_INET, SOCK_STREAM, 0);
  printf ("father1: second socket: %d\n", sock2);

  void *threadResult = 0;

  status = pthread_join (brother, &threadResult);
  TEST_ASSERT_EQUAL (status, 0);

}

void
first_child1 ()
{
  // Create a Socket for listen on port 2000
  int status;
  int sock = -1;
  int sockin = -1;
  struct sockaddr_in ad;
  char buf[1024];

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  fill_addr (ad, 2000);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  printf ("first_child1 accept -> %d\n", sockin);
  TEST_ASSERT (sockin >= 0);

  status = read (sockin, buf, sizeof(buf));
  printf ("first_child1: read result : %d\n",status);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  printf ("first_child1 end\n");
}


void test1 (void)
{
  if (fork ())
    {
      father1 ();
    }
  else
    {
      first_child1 ();
    }
}


int main (int argc, char *argv[])
{
  test1 ();
  return 0;
}
