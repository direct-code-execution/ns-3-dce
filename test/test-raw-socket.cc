#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include "test-macros.h"
#include <unistd.h>

#define BUFF_LEN ((size_t) 1024)

static char sendBuffer [BUFF_LEN];
static char readBuffer [BUFF_LEN];

static
void
fill_addr (struct sockaddr_in &addr, int port)
{
  int res = inet_aton ("127.0.0.1", &(addr.sin_addr));
  TEST_ASSERT_EQUAL (res, 1);

  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
}

// TEST 1 : send an HTTP GET
static void *
test1_client (void *arg)
{
  int status;
  struct sockaddr_in ad;
  int sock = -1;

  sleep (1);

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  fill_addr (ad, 4567);

  status = connect (sock, (struct sockaddr *) &ad, sizeof(ad));
  printf ("test1_client: connect --> %d errno: %d\n", status, errno);
  TEST_ASSERT_EQUAL (status, 0);

  int l = sprintf (sendBuffer, "GET / HTTP/1.1%c%c", 13, 10);

  status = send (sock, sendBuffer, l, 0);

  TEST_ASSERT_EQUAL (l, status);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}

// Pseudo web server
static void *
test1_server (void *arg)
{
  int status;
  int sock = -1;
  int sockin = -1;
  size_t tot = 0;
  struct sockaddr_in ad;

  sock = socket (AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT (sock >= 0);

  fill_addr (ad, 4567);
  status = bind (sock, (struct sockaddr *) &ad, sizeof(ad));
  TEST_ASSERT_EQUAL (status, 0);

  status = listen (sock, 1);
  TEST_ASSERT_EQUAL (status, 0);

  sockin = accept (sock, NULL, NULL);
  TEST_ASSERT (sockin >= 0);

  status = close (sock);
  TEST_ASSERT_EQUAL (status, 0);

  while (tot < BUFF_LEN)
    {
      status = recv (sockin, readBuffer + tot, BUFF_LEN - tot, 0);
      printf ("test1_server: received %d / %ld\n", status, BUFF_LEN - tot);
      if (0 == status)
        {
          break;
        }
      TEST_ASSERT (status > 0);
      tot += status;
    }
  TEST_ASSERT (tot > 0);

  status = close (sockin);
  printf ("test1_server: close -> %d \n ", status);
  TEST_ASSERT_EQUAL (status, 0);

  return arg;
}

// TEST1 Thread: snif raw packets search for GET string
void*
test1_reader (void *ctxt)
{
  int rawFd = *((int*)ctxt);

  do
    {
      printf ("test1_reader: waiting 4 raw Data ....\n");

      char buffer [1024 * 10];
      struct sockaddr_ll from;
      socklen_t l =  sizeof(from);
      int st = recvfrom (rawFd, buffer, sizeof (buffer), 0, (sockaddr*) &from, &l);

      printf ("recvfrom -> %d , Familly, proto %d, 0x%x, Interface Number %d, Packet Type %d, Header type %d 0x%x \n",
              st, from.sll_family, ntohs (from.sll_protocol),
              from.sll_ifindex, from.sll_pkttype, from.sll_hatype, from.sll_hatype);

      for (int i = 14; i < st - 2; i++)
        {
          if ((buffer[i] == 'G') && (buffer[i + 1] == 'E') && (buffer[i + 2] == 'T'))
            {
              printf ("we win !\n");
              return ctxt;   // GET found :)
            }
        }
    }
  while (true);

  return ctxt;
}

// GOAL: sniff packets waiting for DATA.
void
test1 ()
{
  pthread_t t1r, t1c, t1s;
  int rawFd, st;

  rawFd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
//  rawFd = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
  printf ("rawFd %d, errno %d\n", rawFd, errno);
  TEST_ASSERT (rawFd >= 0);
  st = pthread_create (&t1r, 0, test1_reader, &rawFd);
  TEST_ASSERT_EQUAL (st, 0);
  st = pthread_create (&t1c, 0, test1_client, 0);
  TEST_ASSERT_EQUAL (st, 0);
  st = pthread_create (&t1s, 0, test1_server, 0);
  TEST_ASSERT_EQUAL (st, 0);

  pthread_join (t1s, 0);
  printf ("test1 server joined.\n");
  pthread_join (t1c, 0);
  printf ("test1 client joined.\n");
  pthread_join (t1r, 0);
  printf ("test1 reader joined.\n");

  close (rawFd);
}

void*
test2_client (void *arg)
{
  int rawFd, st;
  struct sockaddr_ll dest;
  char buffer [1024 * 1];

  memset (&dest, 0, sizeof (struct sockaddr_ll));
  rawFd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
  // rawFd = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
  TEST_ASSERT (rawFd >= 0);

  dest.sll_family = AF_PACKET;
  dest.sll_halen = 6;
  dest.sll_ifindex = 1;

  sleep (1);
  for (uint32_t i = 0; i < sizeof (buffer); i++)
    {
      buffer [i] = i & 0xff;
    }

  st = sendto (rawFd, buffer, sizeof(buffer), 0, (sockaddr*) &dest, sizeof(dest));

  TEST_ASSERT (rawFd >= 0);

  return arg;
}

void*
test2_server (void *arg)
{
  int rawFd, st;
  char buffer [1024 * 10];
  struct sockaddr_ll from;
  socklen_t l =  sizeof(from);

  rawFd = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL));
  // rawFd = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
  TEST_ASSERT (rawFd >= 0);

  st = recvfrom (rawFd, buffer, sizeof (buffer), 0, (sockaddr*) &from, &l);
  TEST_ASSERT (st >= 0);
  printf ("recvfrom -> %d\n", st);

  return arg;
}

// GOAL: send a packet and received it using socket of domain AF_PACKET
void
test2 ()
{
  pthread_t t2c, t2s;
  int st;

  st = pthread_create (&t2c, 0, test2_client, 0);
  TEST_ASSERT_EQUAL (st, 0);
  st = pthread_create (&t2s, 0, test2_server, 0);
  TEST_ASSERT_EQUAL (st, 0);

  pthread_join (t2s, 0);
  pthread_join (t2c, 0);
}

int
main (int argc, char *argv[])
{
  test2 ();
  test1 ();

  return 0;
}
