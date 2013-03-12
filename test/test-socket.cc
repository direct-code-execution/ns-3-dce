#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <unistd.h>
#include "test-macros.h"

#undef OUTPUT
#define OUTPUT(s) (std::cout << s << std::endl)

void test_raw (void)
{
  int sock;
  const char buf[12] = "0123456789\0";
  struct msghdr msg;
  struct iovec iov[2];
  static struct sockaddr_in dst;
  int ret;
  int on = 1;

  // ICMP Raw sock
  sock = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);
  TEST_ASSERT_UNEQUAL (sock, -1);

  // sockopt IP_PKTINFO
  ret = setsockopt (sock, IPPROTO_IP, IP_PKTINFO, &on, sizeof (on));
  TEST_ASSERT_UNEQUAL (sock, -1);

  // sendmsg
  memset (&dst, 0, sizeof (dst));
  dst.sin_family = AF_INET;
  dst.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  iov[0].iov_base = (void *) buf;
  iov[0].iov_len = sizeof (buf);
  msg.msg_name = &dst;
  msg.msg_namelen = sizeof (dst);
  msg.msg_iov = &iov[0];
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  ret = sendmsg (sock, &msg, 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
  OUTPUT ("RAW send ret = " << ret);

  // recvmsg with MSG_PEEK
  iov[0].iov_base = (void *) buf;
  iov[0].iov_len = 2;
  ret = recvmsg (sock, &msg, MSG_PEEK);
  TEST_ASSERT_EQUAL (ret, 2);
  OUTPUT ("RAW recv PEEK ret = " << ret);

  // recvmsg
  struct in_pktinfo *pktinfo;
  struct cmsghdr *cmsg;
  char cbuff [CMSG_SPACE (sizeof (struct in_pktinfo))];
  char buf2[32];
  iov[0].iov_base = (void *) buf2;
  iov[0].iov_len = sizeof (buf2);
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov[0];
  msg.msg_iovlen = 1;
  msg.msg_control = cbuff;
  msg.msg_controllen = sizeof (cbuff);
  ret = recvmsg (sock, &msg, 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf2));
  OUTPUT ("RAW recv ret = " << ret);
  //  std::cout <<  "RAW recv ret = " << ret << std::endl;

  // RECV interface via PKTINFO
  cmsg = CMSG_FIRSTHDR (&msg);
  pktinfo = (struct in_pktinfo *)CMSG_DATA (cmsg);
#ifdef NO_SENSE_TO_INDEX
  TEST_ASSERT_EQUAL (pktinfo->ipi_ifindex, 1); // Loopback Interface
#endif


  // sockopt IP_HDRINCL
  ret = setsockopt (sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof (on));
  TEST_ASSERT_UNEQUAL (sock, -1);

  // sendmsg with IP_HDRINCL
  struct ip iph;
  memset (&iph, 0, sizeof (iph));
  iph.ip_hl = sizeof (struct ip) >> 2;
  iph.ip_src = dst.sin_addr;
  iph.ip_dst = dst.sin_addr;
  iph.ip_p = IPPROTO_ICMP;
  iph.ip_len = iph.ip_hl * 4 + sizeof (buf);
  iov[0].iov_base = (char*)&iph;
  iov[0].iov_len = iph.ip_hl * 4;
  iov[1].iov_base = (void *) buf;
  iov[1].iov_len = sizeof (buf);
  msg.msg_name = &dst;
  msg.msg_namelen = sizeof (dst);
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  ret = sendmsg (sock, &msg, 0);
  // XXX: FIXME to handle header size
  TEST_ASSERT_EQUAL (ret, (int)sizeof (buf) + iph.ip_hl * 4);
  //  TEST_ASSERT_EQUAL (ret, (int)sizeof (buf) +);
  OUTPUT ("RAW send ret = " << ret);

  // recvmsg with long buf
  char recvbuf[512];
  iov[0].iov_base = (void *) recvbuf;
  iov[0].iov_len = sizeof (recvbuf);
  ret = recvmsg (sock, &msg, 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf) + 20);
  OUTPUT ("RAW recv ret = " << ret);

  // close
  // FIXME!! close () cause WakeupSend and died cause non-context switching
  // close (sock);
  // TEST_ASSERT_UNEQUAL (sock, -1);
}

void test_raw6 (void)
{
  int sock;
  const char buf[12] = "0123456789\0";
  struct msghdr msg;
  struct iovec iov[2];
  static struct sockaddr_in6 dst;
  int ret;

  // ICMP Raw sock
  sock = socket (AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
  TEST_ASSERT_UNEQUAL (sock, -1);

  // sendmsg
  const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
  memset (&dst, 0, sizeof (dst));
  dst.sin6_family = AF_INET6;
  dst.sin6_addr = in6addr_loopback;

  iov[0].iov_base = (void *) buf;
  iov[0].iov_len = sizeof (buf);
  msg.msg_name = &dst;
  msg.msg_namelen = sizeof (dst);
  msg.msg_iov = &iov[0];
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  ret = sendmsg (sock, &msg, 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
  OUTPUT ("RAW6 send ret = " << ret);

  // recvmsg with MSG_PEEK
  iov[0].iov_base = (void *) buf;
  iov[0].iov_len = 2;
  ret = recvmsg (sock, &msg, MSG_PEEK);
  TEST_ASSERT_EQUAL (ret, 2);
  OUTPUT ("RAW6 recv PEEK ret = " << ret);

  // recvmsg
  char buf2[32];
  iov[0].iov_base = (void *) buf2;
  iov[0].iov_len = sizeof (buf2);
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov[0];
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  ret = recvmsg (sock, &msg, 0);
#ifdef IPV6_RAW_SOCK_IS_BROKEN
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
#endif
  OUTPUT ("RAW6 recv ret = " << ret);

  // close
  close (sock);
  TEST_ASSERT_UNEQUAL (sock, -1);
}

void test_udp (void)
{
  int sock;
  const char buf[12] = "0123456789\0";
  struct msghdr msg;
  struct iovec iov;
  static struct sockaddr_in dst;
  int ret;

  // UDP Sock
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT_UNEQUAL (sock, -1);

  memset (&dst, 0, sizeof (dst));
  dst.sin_family = AF_INET;
  dst.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  dst.sin_port = htons (30);

  // bind
  ret = bind (sock, (struct sockaddr *)&dst, sizeof (dst));
  TEST_ASSERT_UNEQUAL (ret, -1);

  // sendmsg
  iov.iov_base = (void *) buf;
  iov.iov_len = sizeof (buf);
  msg.msg_name = &dst;
  msg.msg_namelen = sizeof (dst);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  ret = sendmsg (sock, &msg, 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
  OUTPUT ("UDP send ret = " << ret);

  // recvmsg
  iov.iov_base = (void *) buf;
  iov.iov_len = sizeof (buf);
  ret = recvmsg (sock, &msg, 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
  OUTPUT ("UDP recv ret = " << ret);

  // close
  close (sock);
  TEST_ASSERT_UNEQUAL (sock, -1);
}

static void *
thread_recv (void *arg)
{
  int accept_sock = -1;
  int sock = *(int *)arg;
  int ret;
  char buf[12];

  while (1)
    {
      fd_set readfd;
      struct timeval wait;
      FD_ZERO (&readfd);
      if (accept_sock == -1)
        {
          FD_SET (sock, &readfd);
        }
      else
        {
          FD_SET (accept_sock, &readfd);
        }
      memset (&wait, 0, sizeof (struct timeval));
      wait.tv_sec = 1;

      ret = select (FD_SETSIZE, &readfd, NULL, NULL, &wait);
      TEST_ASSERT_UNEQUAL (ret, 0);
      if (errno == EINTR || errno == EAGAIN)
        {
          continue;
        }
      TEST_ASSERT_UNEQUAL (ret, -1);

      // accept
      if (accept_sock == -1)
        {
          accept_sock = accept (sock, NULL, 0);
          TEST_ASSERT_UNEQUAL (ret, -1);
          OUTPUT ("TCP accepted ret = " << ret);
        }
      else
        {
          break;
        }
    }

  // recv
  ret = recv (accept_sock, &buf, sizeof (buf), 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
  OUTPUT ("TCP recv ret = " << ret);

  pthread_exit (arg);
  // never reached.
  TEST_ASSERT (false);
  return arg;
}

void test_tcp (void)
{
  int socks[2];
  const char buf[12] = "0123456789\0";
  static struct sockaddr_in dst;
  int ret;
  pthread_t thread;

  // TCP Sock
  ret = socketpair (AF_INET, SOCK_STREAM, 0, socks);
  TEST_ASSERT_UNEQUAL (ret, -1);

  memset (&dst, 0, sizeof (dst));
  dst.sin_family = AF_INET;
  dst.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
  dst.sin_port = htons (30);

  // bind
  ret = bind (socks[0], (struct sockaddr *)&dst, sizeof (dst));
  TEST_ASSERT_UNEQUAL (ret, -1);

  // listen
  ret = listen (socks[0], 5);
  TEST_ASSERT_UNEQUAL (ret, -1);

  // recv thread
  ret = pthread_create (&thread, NULL,
                        &thread_recv,
                        (void*)&socks[0]);

  // connect
  ret = connect (socks[1], (struct sockaddr *)&dst, sizeof (dst));
  TEST_ASSERT_UNEQUAL (ret, -1);

  // send
  ret = send (socks[1], &buf, sizeof (buf), 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
  OUTPUT ("TCP send ret = " << ret);

  void *return_value;
  ret = pthread_join (thread, &return_value);

  // close
  close (socks[0]);
  TEST_ASSERT_UNEQUAL (socks[0], -1);
  close (socks[1]);
  TEST_ASSERT_UNEQUAL (socks[1], -1);
}

void test_netlink (void)
{
  int sock;
  int ret;
  struct sockaddr_nl snl;
  socklen_t namelen;
  int seq = 0;
  char buf[4096];
  struct iovec iov;
  struct msghdr msg;
  struct nlmsghdr *h;
  struct ifaddrmsg *ifa;

  // Netlink sock
  sock = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  TEST_ASSERT_UNEQUAL (sock, -1);

  // Bind
  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;
  snl.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_IFADDR;
  ret = bind (sock, (struct sockaddr *) &snl, sizeof snl);
  TEST_ASSERT_UNEQUAL (sock, -1);

  // getsockname
  namelen = sizeof (snl);
  ret = getsockname (sock, (struct sockaddr *) &snl, &namelen);
  TEST_ASSERT_UNEQUAL (ret, -1);
  TEST_ASSERT_EQUAL (namelen, sizeof (snl));

  // sendto
  struct
  {
    struct nlmsghdr nlh;
    struct rtgenmsg g;
  } req;

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  // Get IPv4 Address
  req.nlh.nlmsg_len = sizeof (req);
  req.nlh.nlmsg_type = RTM_GETADDR;
  req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
  req.nlh.nlmsg_pid = 0;
  req.nlh.nlmsg_seq = ++seq;
  req.g.rtgen_family = AF_INET;

  ret = sendto (sock, (void *) &req, sizeof (req), 0,
                (struct sockaddr *) &snl, sizeof (snl));
  TEST_ASSERT_EQUAL (ret, sizeof (req));

  // recvmsg
  iov.iov_base = (void *) buf;
  iov.iov_len = sizeof (buf);
  msg.msg_name = (void *)&snl;
  msg.msg_namelen = sizeof (snl);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  ret = recvmsg (sock, &msg, 0);
  TEST_ASSERT_UNEQUAL (ret, -1);
  TEST_ASSERT_EQUAL (msg.msg_namelen, sizeof (snl));

  // Code from zebra/rt_netlink.c
  for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (uint32_t) ret);
       h = NLMSG_NEXT (h, ret))
    {
      if (h->nlmsg_type == NLMSG_DONE)
        {
          break;
        }

      /* Error handling. */
      if (h->nlmsg_type == NLMSG_ERROR)
        {
          struct nlmsgerr *err = (struct nlmsgerr *) NLMSG_DATA (h);
          /* If the error field is zero, then this is an ACK */
          if (err->error == 0)
            {
              // ACK
              /* return if not a multipart message, otherwise continue */
              if (!(h->nlmsg_flags & NLM_F_MULTI))
                {
                  break;
                }
              continue;
            }

          TEST_ASSERT (h->nlmsg_len >= NLMSG_LENGTH (sizeof (struct nlmsgerr)));
        }

      ifa = (struct ifaddrmsg *) NLMSG_DATA (h);
      TEST_ASSERT (ifa->ifa_family == AF_INET || ifa->ifa_family == AF_INET6);
      OUTPUT ("NL: family =  " << (int)ifa->ifa_family);

      TEST_ASSERT (h->nlmsg_type == RTM_NEWADDR || h->nlmsg_type == RTM_DELADDR);
      TEST_ASSERT (h->nlmsg_len - NLMSG_LENGTH (sizeof (struct ifaddrmsg) >= 0));

      // We didn't get into detail to parse attribute
    }

  close (sock);
  TEST_ASSERT_UNEQUAL (sock, -1);

}

// The following test checks for the timestamp of the received data
void test_timestamp ()
{
  int sock;
  const char buf[12] = "0123456789\0";
  struct msghdr msg;
  struct iovec iov[2];
  static struct sockaddr_in dst;
  struct timeval recv_time;
  int on = 1;
  int ret = 0;

  // ICMP Raw sock
  sock = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);
  TEST_ASSERT_UNEQUAL (sock, -1);

  // sockopt IP_PKTINFO
  ret = setsockopt (sock, IPPROTO_IP, IP_PKTINFO, &on, sizeof (on));
  TEST_ASSERT_UNEQUAL (sock, -1);

  // sendmsg
  memset (&dst, 0, sizeof (dst));
  dst.sin_family = AF_INET;
  dst.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  iov[0].iov_base = (void *) buf;
  iov[0].iov_len = sizeof (buf);
  msg.msg_name = &dst;
  msg.msg_namelen = sizeof (dst);
  msg.msg_iov = &iov[0];
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  ret = sendmsg (sock, &msg, 0);
  TEST_ASSERT_EQUAL (ret, sizeof (buf));
  OUTPUT ("RAW send ret = " << ret);

  // recvmsg with MSG_PEEK
  iov[0].iov_base = (void *) buf;
  iov[0].iov_len = 2;
  ret = recvmsg (sock, &msg, MSG_PEEK);
  TEST_ASSERT_EQUAL (ret, 2);
  OUTPUT ("RAW recv PEEK ret = " << ret);


  if (0 == ioctl (sock, SIOCGSTAMP, &recv_time))
    {
      struct timeval now;

      // On LinuxKernel supports SIOCGSTAMP
      // the ioctl time must be very close to now !
      gettimeofday (&now, NULL);
      OUTPUT ("Now seconds : " << now.tv_sec << " and timestamp : " << recv_time.tv_sec );

      // The following assert is disabled waiting for bug 1536 to be fixed
      // TEST_ASSERT ( abs(recv_time.tv_sec - now.tv_sec) < 2 );
    }
}

int main (int argc, char *argv[])
{
  test_timestamp ();
  test_raw ();
  test_raw6 ();
  test_udp ();
  test_tcp ();
  test_netlink ();

  return 0;
}
