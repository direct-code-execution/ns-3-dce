#include "test-macros.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main (int argc, char *argv[])
{

  static const int MAX_MSDU_SIZE = 2304;
  int ioctlres = -1;
  struct ifreq m_ifreq;
  struct iwreq m_iwreq;
  memset (&m_ifreq, 0, sizeof (ifreq));
  memset (&m_iwreq, 0, sizeof (iwreq));

  int sock = socket (AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT_UNEQUAL (sock, -1);

  strncpy (m_ifreq.ifr_name, "ath0", IFNAMSIZ);
  strncpy (m_iwreq.ifr_ifrn.ifrn_name, "ath0", IFNAMSIZ);

  //SIOCGIFFLAGS
  ioctlres = ioctl (sock, SIOCGIFFLAGS, &m_ifreq);
  TEST_ASSERT_UNEQUAL (ioctlres, -1);
  TEST_ASSERT_UNEQUAL (m_ifreq.ifr_flags & (IFF_UP | IFF_BROADCAST), 0);

  //SIOCGIWNAME
  ioctlres = ioctl (sock, SIOCGIWNAME, &m_iwreq);
  TEST_ASSERT_UNEQUAL (ioctlres, -1);
  TEST_ASSERT_UNEQUAL (m_iwreq.u.name, 0);


  //SIOCGIFADDR
  {
    ioctlres = ioctl (sock, SIOCGIFADDR, &m_ifreq);
    TEST_ASSERT_UNEQUAL (ioctlres, -1);
    struct sockaddr* saddr = &m_ifreq.ifr_addr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *) saddr;
    struct in_addr re_addr;
    inet_aton ("10.1.1.1", &re_addr);
    TEST_ASSERT (memcmp (&(ipv4->sin_addr), &re_addr, sizeof (in_addr)) == 0);
  }

  //SIOCGIFBRDADDR
  ioctlres = ioctl (sock, SIOCGIFBRDADDR, &m_ifreq);
  {
    ioctlres = ioctl (sock, SIOCGIFBRDADDR, &m_ifreq);
    TEST_ASSERT_UNEQUAL (ioctlres, -1);
    struct sockaddr* saddr = &m_ifreq.ifr_broadaddr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *) saddr;
    struct in_addr re_addr;
    inet_aton ("10.1.1.255", &re_addr);
    TEST_ASSERT (memcmp (&(ipv4->sin_addr), &re_addr, sizeof (in_addr)) == 0);
  }

  //SIOCGIFNETMASK
  {
    ioctlres = ioctl (sock, SIOCGIFNETMASK, &m_ifreq);
    TEST_ASSERT_UNEQUAL (ioctlres, -1);
    struct sockaddr* saddr = &m_ifreq.ifr_netmask;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *) saddr;
    struct in_addr re_addr;
    inet_aton ("255.255.255.0", &re_addr);
    TEST_ASSERT (memcmp (&(ipv4->sin_addr), &re_addr, sizeof (in_addr)) == 0);
  }

  //SIOCGIFMTU
  ioctlres = ioctl (sock, SIOCGIFMTU, &m_ifreq);
  TEST_ASSERT (m_ifreq.ifr_mtu <= MAX_MSDU_SIZE);

  close (sock);

  exit (0);
  // never reached.
  return -1;
}
