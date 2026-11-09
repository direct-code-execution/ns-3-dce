#define _GNU_SOURCE 1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "test-macros.h"

// Sends one datagram with an IPV6_PKTINFO control message selecting
// interface ifindex; controllen is the length of the control buffer given.
static ssize_t
SendWithPktinfo (int sock, struct sockaddr_in6 *to, unsigned ifindex, size_t controllen)
{
  char data[] = "hello";
  struct iovec iov;
  iov.iov_base = data;
  iov.iov_len = sizeof (data);
  union
  {
    char buf[CMSG_SPACE (sizeof (struct in6_pktinfo))];
    struct cmsghdr align;
  } control;
  memset (&control, 0, sizeof (control));
  struct msghdr msg;
  memset (&msg, 0, sizeof (msg));
  msg.msg_name = to;
  msg.msg_namelen = to ? sizeof (*to) : 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control.buf;
  msg.msg_controllen = controllen;
  struct cmsghdr *c = (struct cmsghdr *)control.buf;
  c->cmsg_level = IPPROTO_IPV6;
  c->cmsg_type = IPV6_PKTINFO;
  c->cmsg_len = CMSG_LEN (sizeof (struct in6_pktinfo));
  struct in6_pktinfo *info = (struct in6_pktinfo *)CMSG_DATA (c);
  info->ipi6_ifindex = ifindex;
  return sendmsg (sock, &msg, 0);
}

int
main (int argc, char *argv[])
{
  int sock = socket (AF_INET6, SOCK_DGRAM, 0);
  TEST_ASSERT (sock >= 0);
  struct sockaddr_in6 to;
  memset (&to, 0, sizeof (to));
  to.sin6_family = AF_INET6;
  to.sin6_port = htons (9);
  to.sin6_addr = in6addr_loopback;
  size_t controllen = CMSG_SPACE (sizeof (struct in6_pktinfo));

  // lo is interface 1.
  TEST_ASSERT_EQUAL (SendWithPktinfo (sock, &to, 1, controllen), 6);
  // No such interface.
  TEST_ASSERT_EQUAL (SendWithPktinfo (sock, &to, 99, controllen), -1);
  TEST_ASSERT_EQUAL (errno, ENODEV);
  // A control message longer than the control buffer.
  TEST_ASSERT_EQUAL (SendWithPktinfo (sock, &to, 1, CMSG_LEN (sizeof (struct in6_pktinfo)) - 4), -1);
  TEST_ASSERT_EQUAL (errno, EINVAL);

  // The same on a connected socket.
  TEST_ASSERT_EQUAL (connect (sock, (struct sockaddr *)&to, sizeof (to)), 0);
  TEST_ASSERT_EQUAL (SendWithPktinfo (sock, 0, 1, controllen), 6);
  TEST_ASSERT_EQUAL (SendWithPktinfo (sock, 0, 99, controllen), -1);
  TEST_ASSERT_EQUAL (errno, ENODEV);

  TEST_ASSERT_EQUAL (close (sock), 0);
  return 0;
}
