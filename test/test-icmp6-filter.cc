#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "test-macros.h"

// A raw ICMPv6 socket with ICMP6_FILTER only receives the types it lets
// pass: here the echo reply, not the echo request sent to ::1 before it.
int
main (int argc, char *argv[])
{
  int sock = socket (AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
  TEST_ASSERT (sock >= 0);
  struct icmp6_filter filter;
  ICMP6_FILTER_SETBLOCKALL (&filter);
  ICMP6_FILTER_SETPASS (ICMP6_ECHO_REPLY, &filter);
  TEST_ASSERT_EQUAL (setsockopt (sock, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof (filter)), 0);
  struct icmp6_filter got;
  socklen_t len = sizeof (got);
  TEST_ASSERT_EQUAL (getsockopt (sock, IPPROTO_ICMPV6, ICMP6_FILTER, &got, &len), 0);
  TEST_ASSERT_EQUAL (memcmp (&got, &filter, sizeof (filter)), 0);
  struct timeval timeout = { 1, 0 };
  TEST_ASSERT_EQUAL (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof (timeout)), 0);

  struct sockaddr_in6 to;
  memset (&to, 0, sizeof (to));
  to.sin6_family = AF_INET6;
  to.sin6_addr = in6addr_loopback;
  struct icmp6_hdr request;
  memset (&request, 0, sizeof (request));
  request.icmp6_type = ICMP6_ECHO_REQUEST;
  request.icmp6_id = htons (1);
  request.icmp6_seq = htons (1);
  TEST_ASSERT_EQUAL (sendto (sock, &request, sizeof (request), 0, (struct sockaddr *)&to, sizeof (to)),
                     (ssize_t)sizeof (request));

  unsigned char buf[1500];
  ssize_t n = recv (sock, buf, sizeof (buf), 0);
  TEST_ASSERT (n >= (ssize_t)sizeof (struct icmp6_hdr));
  TEST_ASSERT_EQUAL ((int)((struct icmp6_hdr *)buf)->icmp6_type, ICMP6_ECHO_REPLY);

  TEST_ASSERT_EQUAL (close (sock), 0);
  return 0;
}
