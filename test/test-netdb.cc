#define _GNU_SOURCE 1
#include <netdb.h>
#include "test-macros.h"

void test_gethostbyname (void)
{
  struct hostent *host;
  host = gethostbyname ("127.0.0.1");
  TEST_ASSERT_UNEQUAL (host, 0);
  TEST_ASSERT_EQUAL (host->h_addrtype, AF_INET);
}

void test_gethostbyname2 (void)
{
  struct hostent *host;
  host = gethostbyname2 ("127.0.0.1", AF_INET);
  TEST_ASSERT_UNEQUAL (host, 0);
  TEST_ASSERT_EQUAL (host->h_addrtype, AF_INET);
}

void test_getaddrinfo (void)
{
  struct addrinfo *info;
  int status = getaddrinfo ("127.0.0.1", "http", 0, &info);
  TEST_ASSERT_EQUAL (status, 0);
  freeaddrinfo (info);
  status = getaddrinfo ("127.0.0.1", "htt", 0, &info);
  TEST_ASSERT_UNEQUAL (status, 0);
  TEST_ASSERT_EQUAL (status, EAI_SERVICE);
  TEST_ASSERT (gai_strerror (status) != 0);
}


int main (int argc, char *argv[])
{
  test_gethostbyname ();
  test_gethostbyname2 ();
  test_getaddrinfo ();
  return 0;
}
