#include "dce-netdb.h"
#include "utils.h"
#include "dce-stdlib.h"
#include "dce-string.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include <string.h>
#include "process.h"
#include "errno.h"
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "sys/dce-socket.h"
#include "dce-unistd.h"
#include "dce-signal.h"

NS_LOG_COMPONENT_DEFINE ("DceNetdb");

using namespace ns3;

struct hostent * dce_gethostbyname (const char *name)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << name);
  NS_ASSERT (Current () != 0);
  static struct hostent host;
  static uint32_t addr;
  static char *alias_end = 0;
  static char *addr_list[2];


  Ipv4Address ipv4 = Ipv4Address (name);
  addr = htonl (ipv4.Get ());

  //XXX: We do not implement dns lookup here for now. We just
  // interpret simple ip strings.

  host.h_name = (char *)name;
  host.h_addrtype = AF_INET;
  host.h_aliases = &alias_end;
  host.h_length = 4;
  host.h_addr_list = addr_list;
  addr_list[0] = (char *)&addr;
  addr_list[1] = 0;
  return &host;
}
struct hostent * dce_gethostbyname2 (const char *name, int af)
{
  NS_ASSERT (af == AF_INET);
  return dce_gethostbyname (name);
}
int dce_getaddrinfo (const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **res)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << ((NULL == node) ? "" : node) << ((NULL == service) ? "" : service) << hints << res);
  NS_ASSERT (Current () != 0);
  struct addrinfo *tmp = 0;
  int status = ::getaddrinfo (node, service, hints, &tmp);
  // copy outgoing data structure so that the memory is allocated from the calling process memory pool.
  struct addrinfo *cur, *prev, *head;
  head = 0;
  prev = 0;
  for (cur = tmp; cur != 0; cur = cur->ai_next)
    {
      struct addrinfo *copy = (struct addrinfo*)dce_malloc (sizeof(struct addrinfo));
      memcpy (copy, cur, sizeof (struct addrinfo));
      copy->ai_addr = (struct sockaddr*)dce_malloc (cur->ai_addrlen);
      if (cur->ai_canonname != 0)
        {
          copy->ai_canonname = dce_strdup (cur->ai_canonname);
        }
      else
        {
          copy->ai_canonname = 0;
        }
      memcpy (copy->ai_addr, cur->ai_addr, cur->ai_addrlen);
      if (prev != 0)
        {
          prev->ai_next = copy;
        }
      else
        {
          head = copy;
        }
      prev = copy;
    }
  if (prev != 0)
    {
      prev->ai_next = 0;
    }
  if (status == 0)
    {
      *res = head;
    }
  else
    {
      *res = 0;
    }
  ::freeaddrinfo (tmp);
  return status;
}
void dce_freeaddrinfo (struct addrinfo *res)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << res);
  NS_ASSERT (Current () != 0);
  struct addrinfo *cur, *next;
  for (cur = res; cur != 0; cur = next)
    {
      next = cur->ai_next;
      dce_free (cur->ai_addr);
      if (cur->ai_canonname != 0)
        {
          dce_free (cur->ai_canonname);
        }
      dce_free (cur);
    }
}
const char * dce_gai_strerror (int errcode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << errcode);
  NS_ASSERT (Current () != 0);
  return ::gai_strerror (errcode);
}
int dce_getnameinfo (const struct sockaddr *sa, socklen_t salen, char *host,
                     socklen_t hostlen, char *serv, socklen_t servlen, unsigned int flags)
{
  NS_LOG_FUNCTION (Current ());

  if ((0 == sa) || (0 == salen))
    {
      Current ()->err = EINVAL;
      return EAI_SYSTEM;
    }

  switch (sa->sa_family)
    {
    case AF_INET:
      {
        if (salen < sizeof (struct sockaddr_in))
          {
            Current ()->err = EINVAL;
            return EAI_SYSTEM;
          }
        const struct sockaddr_in *inAddr = (const struct sockaddr_in *) sa;

        if (0 != serv)
          {
            int r = snprintf (serv, servlen, "%d",  htons (inAddr->sin_port));

            if (r > (int)servlen)
              {
                return EAI_OVERFLOW;
              }
            if (r < 0)
              {
                Current ()->err = errno;
                return EAI_SYSTEM;
              }
          }
        if  (0 != host)
          {
            Ipv4Address ipv4 = Ipv4Address (htonl (inAddr->sin_addr.s_addr));
            std::ostringstream oss;
            ipv4.Print (oss);

            int r = snprintf (host, hostlen, "%s", oss.str ().c_str ());

            if (r > (int)hostlen)
              {
                return EAI_OVERFLOW;
              }
            if (r < 0)
              {
                Current ()->err = errno;
                return EAI_SYSTEM;
              }
          }
        return 0;
      }
      break;

    default:
      return EAI_FAMILY;
    }

  return 0; // XXX : cheater
}

void dce_herror (const char *string)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);

  fprintf (*Current ()->process->pstderr, "%s : %s\n", string, "ERROR");
}

// Copy from glibc source
// eglibc-2.11.1/sysdeps/unix/sysv/linux/ifaddr.c
struct netlink_res
{
  struct netlink_res *next;
  struct nlmsghdr *nlh;
  size_t size;                  /* Size of response.  */
  uint32_t seq;                 /* sequential number we used.  */
};
struct netlink_handle
{
  int fd;                       /* Netlink file descriptor.  */
  pid_t pid;                    /* Process ID.  */
  uint32_t seq;                 /* The sequence number we use currently.  */
  struct netlink_res *nlm_list; /* Pointer to list of responses.  */
  struct netlink_res *end_ptr;  /* For faster append of new entries.  */
};
struct sockaddr_ll_max
{
  unsigned short int sll_family;
  unsigned short int sll_protocol;
  int sll_ifindex;
  unsigned short int sll_hatype;
  unsigned char sll_pkttype;
  unsigned char sll_halen;
  unsigned char sll_addr[24];
};
struct ifaddrs_storage
{
  struct ifaddrs ifa;
  union
  {
    /* Save space for the biggest of the four used sockaddr types and
       avoid a lot of casts.  */
    struct sockaddr sa;
    struct sockaddr_ll_max sl;
    struct sockaddr_in s4;
    struct sockaddr_in6 s6;
  } addr, netmask, broadaddr;
  char name[IF_NAMESIZE + 1];
};
#define PAGE_SIZE 4096
static int
netlink_request (struct netlink_handle *h, int type)
{
  int ret;
  struct sockaddr_nl snl;
  struct netlink_res *nlm_next;
  struct sockaddr_nl nladdr;
  struct nlmsghdr *nlmh;
  ssize_t read_len;
  bool done = false;

  struct
  {
    struct nlmsghdr nlh;
    struct rtgenmsg g;
  } req;

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  memset (&req, 0, sizeof req);
  req.nlh.nlmsg_len = sizeof req;
  req.nlh.nlmsg_type = type;
  req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
  req.nlh.nlmsg_pid = h->pid;
  req.nlh.nlmsg_seq = ++h->seq;
  req.g.rtgen_family = AF_UNSPEC;

  ret = dce_sendto (h->fd, (void *) &req, sizeof req, 0,
                    (struct sockaddr *) &snl, sizeof snl);
  if (ret < 0)
    {
      return -1;
    }

  char *buf;
  const size_t buf_size = PAGE_SIZE;
  buf = (char *)dce_malloc (buf_size);
  if (!buf)
    {
      return -1;
    }
  struct iovec iov =
  {
    buf, buf_size
  };

  while (!done)
    {
      struct msghdr msg =
      {
        (void *) &nladdr, sizeof (nladdr),
        &iov, 1,
        NULL, 0,
        0
      };

      read_len = dce_recvmsg (h->fd, &msg, 0);
      if (read_len < 0)
        {
          goto out_fail;
        }

      if (nladdr.nl_pid != 0)
        {
          continue;
        }

      if (__builtin_expect (msg.msg_flags & MSG_TRUNC, 0))
        {
          goto out_fail;
        }

      size_t count = 0;
      size_t remaining_len = read_len;
      for (nlmh = (struct nlmsghdr *) buf;
           NLMSG_OK (nlmh, remaining_len);
           nlmh = (struct nlmsghdr *) NLMSG_NEXT (nlmh, remaining_len))
        {
          if ((pid_t) nlmh->nlmsg_pid != h->pid
              || nlmh->nlmsg_seq != h->seq)
            {
              continue;
            }

          ++count;
          if (nlmh->nlmsg_type == NLMSG_DONE)
            {
              /* We found the end, leave the loop.  */
              done = true;
              break;
            }
          if (nlmh->nlmsg_type == NLMSG_ERROR)
            {
              struct nlmsgerr *nlerr = (struct nlmsgerr *) NLMSG_DATA (nlmh);
              if (nlmh->nlmsg_len < NLMSG_LENGTH (sizeof (struct nlmsgerr)))
                {
                  errno = EIO;
                }
              else
                {
                  errno = -nlerr->error;
                }
              goto out_fail;
            }
        }

      /* If there was nothing with the expected nlmsg_pid and nlmsg_seq,
         there is no point to record it.  */
      if (count == 0)
        {
          continue;
        }

      nlm_next = (struct netlink_res *) malloc (sizeof (struct netlink_res)
                                                + read_len);
      if (nlm_next == NULL)
        {
          goto out_fail;
        }
      nlm_next->next = NULL;
      nlm_next->nlh = (struct nlmsghdr *)memcpy (nlm_next + 1, buf, read_len);
      nlm_next->size = read_len;
      nlm_next->seq = h->seq;
      if (h->nlm_list == NULL)
        {
          h->nlm_list = nlm_next;
        }
      else
        {
          h->end_ptr->next = nlm_next;
        }
      h->end_ptr = nlm_next;
    }

  dce_free (buf);
  return 0;

out_fail:
  dce_free (buf);
  return -1;
}

static int
map_newlink (int index, struct ifaddrs_storage *ifas, int *map, int max)
{
  int i;

  for (i = 0; i < max; i++)
    {
      if (map[i] == -1)
        {
          map[i] = index;
          if (i > 0)
            {
              ifas[i - 1].ifa.ifa_next = &ifas[i].ifa;
            }
          return i;
        }
      else if (map[i] == index)
        {
          return i;
        }
    }
  /* This should never be reached. If this will be reached, we have
     a very big problem.  */
  dce_abort ();
  return 0;
}

static void
__netlink_free_handle (struct netlink_handle *h)
{
  struct netlink_res *ptr;
  int saved_errno = errno;

  ptr = h->nlm_list;
  while (ptr != NULL)
    {
      struct netlink_res *tmpptr;

      tmpptr = ptr->next;
      free (ptr);
      ptr = tmpptr;
    }

  Current ()->err = saved_errno;
}

/*
 * Try to emulate netlink socket query to work both ns3 stack and
 * linux stack.
 */
int
dce_getifaddrs (struct ifaddrs **ifap)
{
  struct netlink_handle nh =
  {
    0, 0, 0, NULL, NULL
  };
  struct sockaddr_nl nladdr;
  struct netlink_res *nlp;
  struct ifaddrs_storage *ifas;
  unsigned int i, newlink, newaddr, newaddr_idx;
  int *map_newlink_data;
  size_t ifa_data_size = 0;  /* Size to allocate for all ifa_data.  */
  char *ifa_data_ptr;   /* Pointer to the unused part of memory for
                           ifa_data.  */
  int result = 0;

  nh.fd = dce_socket (PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (nh.fd < 0)
    {
      Current ()->err = EINVAL;
      return -1;
    }

  memset (&nladdr, 0, sizeof (nladdr));
  nladdr.nl_family = AF_NETLINK;
  if (dce_bind (nh.fd, (struct sockaddr *) &nladdr, sizeof (nladdr)) < 0)
    {
      dce_close (nh.fd);
      Current ()->err = EINVAL;
      return -1;
    }
  socklen_t addr_len = sizeof (nladdr);
  if (dce_getsockname (nh.fd, (struct sockaddr *) &nladdr, &addr_len) < 0)
    {
      dce_close (nh.fd);
      Current ()->err = EINVAL;
      return -1;
    }
  nh.pid = nladdr.nl_pid;


  if (netlink_request (&nh, RTM_GETLINK) < 0)
    {
      dce_close (nh.fd);
      Current ()->err = EINVAL;
      return -1;
    }

  ++nh.seq;
  if (netlink_request (&nh, RTM_GETADDR) < 0)
    {
      dce_close (nh.fd);
      Current ()->err = EINVAL;
      return -1;
    }



  newlink = newaddr = 0;
  for (nlp = nh.nlm_list; nlp; nlp = nlp->next)
    {
      struct nlmsghdr *nlh;
      size_t size = nlp->size;

      if (nlp->nlh == NULL)
        {
          continue;
        }

      /* Walk through all entries we got from the kernel and look, which
         message type they contain.  */
      for (nlh = nlp->nlh; NLMSG_OK (nlh, size); nlh = NLMSG_NEXT (nlh, size))
        {
          /* Check if the message is what we want.  */
          if ((pid_t) nlh->nlmsg_pid != nh.pid || nlh->nlmsg_seq != nlp->seq)
            {
              continue;
            }

          if (nlh->nlmsg_type == NLMSG_DONE)
            {
              break;            /* ok */

            }
          if (nlh->nlmsg_type == RTM_NEWLINK)
            {
              /* A RTM_NEWLINK message can have IFLA_STATS data. We need to
                 know the size before creating the list to allocate enough
                 memory.  */
              struct ifinfomsg *ifim = (struct ifinfomsg *) NLMSG_DATA (nlh);
              struct rtattr *rta = IFLA_RTA (ifim);
              size_t rtasize = IFLA_PAYLOAD (nlh);

              while (RTA_OK (rta, rtasize))
                {
                  size_t rta_payload = RTA_PAYLOAD (rta);

                  if (rta->rta_type == IFLA_STATS)
                    {
                      ifa_data_size += rta_payload;
                      break;
                    }
                  else
                    {
                      rta = RTA_NEXT (rta, rtasize);
                    }
                }
              ++newlink;
            }
          else if (nlh->nlmsg_type == RTM_NEWADDR)
            {
              ++newaddr;
            }
        }
    }

  /* Return if no interface is up.  */
  if ((newlink + newaddr) == 0)
    {
      goto exit_free;
    }

  /* Allocate memory for all entries we have and initialize next
     pointer.  */
  ifas = (struct ifaddrs_storage *) calloc (1,
                                            (newlink + newaddr)
                                            * sizeof (struct ifaddrs_storage)
                                            + ifa_data_size);
  if (ifas == NULL)
    {
      result = -1;
      goto exit_free;
    }

  /* Table for mapping kernel index to entry in our list.  */
  map_newlink_data = (int *)alloca (newlink * sizeof (int));
  memset (map_newlink_data, '\xff', newlink * sizeof (int));

  ifa_data_ptr = (char *) &ifas[newlink + newaddr];
  newaddr_idx = 0;              /* Counter for newaddr index.  */

  /* Walk through the list of data we got from the kernel.  */
  for (nlp = nh.nlm_list; nlp; nlp = nlp->next)
    {
      struct nlmsghdr *nlh;
      size_t size = nlp->size;

      if (nlp->nlh == NULL)
        {
          continue;
        }

      /* Walk through one message and look at the type: If it is our
         message, we need RTM_NEWLINK/RTM_NEWADDR and stop if we reach
         the end or we find the end marker (in this case we ignore the
         following data.  */
      for (nlh = nlp->nlh; NLMSG_OK (nlh, size); nlh = NLMSG_NEXT (nlh, size))
        {
          int ifa_index = 0;

          /* Check if the message is the one we want */
          if ((pid_t) nlh->nlmsg_pid != nh.pid || nlh->nlmsg_seq != nlp->seq)
            {
              continue;
            }

          if (nlh->nlmsg_type == NLMSG_DONE)
            {
              break;            /* ok */

            }
          if (nlh->nlmsg_type == RTM_NEWLINK)
            {
              /* We found a new interface. Now extract everything from the
                 interface data we got and need.  */
              struct ifinfomsg *ifim = (struct ifinfomsg *) NLMSG_DATA (nlh);
              struct rtattr *rta = IFLA_RTA (ifim);
              size_t rtasize = IFLA_PAYLOAD (nlh);

              /* Interfaces are stored in the first "newlink" entries
                 of our list, starting in the order as we got from the
                 kernel.  */
              ifa_index = map_newlink (ifim->ifi_index - 1, ifas,
                                       map_newlink_data, newlink);
              ifas[ifa_index].ifa.ifa_flags = ifim->ifi_flags;

              while (RTA_OK (rta, rtasize))
                {
                  char *rta_data = (char *)RTA_DATA (rta);
                  size_t rta_payload = RTA_PAYLOAD (rta);

                  switch (rta->rta_type)
                    {
                    case IFLA_ADDRESS:
                      if (rta_payload <= sizeof (ifas[ifa_index].addr))
                        {
                          ifas[ifa_index].addr.sl.sll_family = AF_PACKET;
                          memcpy (ifas[ifa_index].addr.sl.sll_addr,
                                  (char *) rta_data, rta_payload);
                          ifas[ifa_index].addr.sl.sll_halen = rta_payload;
                          ifas[ifa_index].addr.sl.sll_ifindex
                            = ifim->ifi_index;
                          ifas[ifa_index].addr.sl.sll_hatype = ifim->ifi_type;

                          ifas[ifa_index].ifa.ifa_addr
                            = &ifas[ifa_index].addr.sa;
                        }
                      break;

                    case IFLA_BROADCAST:
                      if (rta_payload <= sizeof (ifas[ifa_index].broadaddr))
                        {
                          ifas[ifa_index].broadaddr.sl.sll_family = AF_PACKET;
                          memcpy (ifas[ifa_index].broadaddr.sl.sll_addr,
                                  (char *) rta_data, rta_payload);
                          ifas[ifa_index].broadaddr.sl.sll_halen = rta_payload;
                          ifas[ifa_index].broadaddr.sl.sll_ifindex
                            = ifim->ifi_index;
                          ifas[ifa_index].broadaddr.sl.sll_hatype
                            = ifim->ifi_type;

                          ifas[ifa_index].ifa.ifa_broadaddr
                            = &ifas[ifa_index].broadaddr.sa;
                        }
                      break;

                    case IFLA_IFNAME:   /* Name of Interface */
                      if ((rta_payload + 1) <= sizeof (ifas[ifa_index].name))
                        {
                          ifas[ifa_index].ifa.ifa_name = ifas[ifa_index].name;
                          *(char *) __mempcpy (ifas[ifa_index].name, rta_data,
                                               rta_payload) = '\0';
                        }
                      break;

                    case IFLA_STATS:    /* Statistics of Interface */
                      ifas[ifa_index].ifa.ifa_data = ifa_data_ptr;
                      ifa_data_ptr += rta_payload;
                      memcpy (ifas[ifa_index].ifa.ifa_data, rta_data,
                              rta_payload);
                      break;

                    case IFLA_UNSPEC:
                      break;
                    case IFLA_MTU:
                      break;
                    case IFLA_LINK:
                      break;
                    case IFLA_QDISC:
                      break;
                    default:
                      break;
                    }

                  rta = RTA_NEXT (rta, rtasize);
                }
            }
          else if (nlh->nlmsg_type == RTM_NEWADDR)
            {
              struct ifaddrmsg *ifam = (struct ifaddrmsg *) NLMSG_DATA (nlh);
              struct rtattr *rta = IFA_RTA (ifam);
              size_t rtasize = IFA_PAYLOAD (nlh);

              /* New Addresses are stored in the order we got them from
                 the kernel after the interfaces. Theoretically it is possible
                 that we have holes in the interface part of the list,
                 but we always have already the interface for this address.  */
              ifa_index = newlink + newaddr_idx;
              ifas[ifa_index].ifa.ifa_flags
                = ifas[map_newlink (ifam->ifa_index - 1, ifas,
                                    map_newlink_data, newlink)].ifa.ifa_flags;
              if (ifa_index > 0)
                {
                  ifas[ifa_index - 1].ifa.ifa_next = &ifas[ifa_index].ifa;
                }
              ++newaddr_idx;

              while (RTA_OK (rta, rtasize))
                {
                  char *rta_data = (char *)RTA_DATA (rta);
                  size_t rta_payload = RTA_PAYLOAD (rta);

                  switch (rta->rta_type)
                    {
                    case IFA_ADDRESS:
                      {
                        struct sockaddr *sa;

                        if (ifas[ifa_index].ifa.ifa_addr != NULL)
                          {
                            /* In a point-to-poing network IFA_ADDRESS
                               contains the destination address, local
                               address is supplied in IFA_LOCAL attribute.
                               destination address and broadcast address
                               are stored in an union, so it doesn't matter
                               which name we use.  */
                            ifas[ifa_index].ifa.ifa_broadaddr
                              = &ifas[ifa_index].broadaddr.sa;
                            sa = &ifas[ifa_index].broadaddr.sa;
                          }
                        else
                          {
                            ifas[ifa_index].ifa.ifa_addr
                              = &ifas[ifa_index].addr.sa;
                            sa = &ifas[ifa_index].addr.sa;
                          }

                        sa->sa_family = ifam->ifa_family;

                        switch (ifam->ifa_family)
                          {
                          case AF_INET:
                            /* Size must match that of an address for IPv4.  */
                            if (rta_payload == 4)
                              {
                                memcpy (&((struct sockaddr_in *) sa)->sin_addr,
                                        rta_data, rta_payload);
                              }
                            break;

                          case AF_INET6:
                            /* Size must match that of an address for IPv6.  */
                            if (rta_payload == 16)
                              {
                                memcpy (&((struct sockaddr_in6 *) sa)->sin6_addr,
                                        rta_data, rta_payload);
                                if (IN6_IS_ADDR_LINKLOCAL (rta_data)
                                    || IN6_IS_ADDR_MC_LINKLOCAL (rta_data))
                                  {
                                    ((struct sockaddr_in6 *) sa)->sin6_scope_id
                                      = ifam->ifa_index;
                                  }
                              }
                            break;

                          default:
                            if (rta_payload <= sizeof (ifas[ifa_index].addr))
                              {
                                memcpy (sa->sa_data, rta_data, rta_payload);
                              }
                            break;
                          }
                      }
                      break;

                    case IFA_LOCAL:
                      if (ifas[ifa_index].ifa.ifa_addr != NULL)
                        {
                          /* If ifa_addr is set and we get IFA_LOCAL,
                             assume we have a point-to-point network.
                             Move address to correct field.  */
                          ifas[ifa_index].broadaddr = ifas[ifa_index].addr;
                          ifas[ifa_index].ifa.ifa_broadaddr
                            = &ifas[ifa_index].broadaddr.sa;
                          memset (&ifas[ifa_index].addr, '\0',
                                  sizeof (ifas[ifa_index].addr));
                        }

                      ifas[ifa_index].ifa.ifa_addr = &ifas[ifa_index].addr.sa;
                      ifas[ifa_index].ifa.ifa_addr->sa_family
                        = ifam->ifa_family;

                      switch (ifam->ifa_family)
                        {
                        case AF_INET:
                          /* Size must match that of an address for IPv4.  */
                          if (rta_payload == 4)
                            {
                              memcpy (&ifas[ifa_index].addr.s4.sin_addr,
                                      rta_data, rta_payload);
                            }
                          break;

                        case AF_INET6:
                          /* Size must match that of an address for IPv6.  */
                          if (rta_payload == 16)
                            {
                              memcpy (&ifas[ifa_index].addr.s6.sin6_addr,
                                      rta_data, rta_payload);
                              if (IN6_IS_ADDR_LINKLOCAL (rta_data)
                                  || IN6_IS_ADDR_MC_LINKLOCAL (rta_data))
                                {
                                  ifas[ifa_index].addr.s6.sin6_scope_id =
                                    ifam->ifa_index;
                                }
                            }
                          break;

                        default:
                          if (rta_payload <= sizeof (ifas[ifa_index].addr))
                            {
                              memcpy (ifas[ifa_index].addr.sa.sa_data,
                                      rta_data, rta_payload);
                            }
                          break;
                        }
                      break;

                    case IFA_BROADCAST:
                      /* We get IFA_BROADCAST, so IFA_LOCAL was too much.  */
                      if (ifas[ifa_index].ifa.ifa_broadaddr != NULL)
                        {
                          memset (&ifas[ifa_index].broadaddr, '\0',
                                  sizeof (ifas[ifa_index].broadaddr));
                        }

                      ifas[ifa_index].ifa.ifa_broadaddr
                        = &ifas[ifa_index].broadaddr.sa;
                      ifas[ifa_index].ifa.ifa_broadaddr->sa_family
                        = ifam->ifa_family;

                      switch (ifam->ifa_family)
                        {
                        case AF_INET:
                          /* Size must match that of an address for IPv4.  */
                          if (rta_payload == 4)
                            {
                              memcpy (&ifas[ifa_index].broadaddr.s4.sin_addr,
                                      rta_data, rta_payload);
                            }
                          break;

                        case AF_INET6:
                          /* Size must match that of an address for IPv6.  */
                          if (rta_payload == 16)
                            {
                              memcpy (&ifas[ifa_index].broadaddr.s6.sin6_addr,
                                      rta_data, rta_payload);
                              if (IN6_IS_ADDR_LINKLOCAL (rta_data)
                                  || IN6_IS_ADDR_MC_LINKLOCAL (rta_data))
                                {
                                  ifas[ifa_index].broadaddr.s6.sin6_scope_id
                                    = ifam->ifa_index;
                                }
                            }
                          break;

                        default:
                          if (rta_payload <= sizeof (ifas[ifa_index].addr))
                            {
                              memcpy (&ifas[ifa_index].broadaddr.sa.sa_data,
                                      rta_data, rta_payload);
                            }
                          break;
                        }
                      break;

                    case IFA_LABEL:
                      if (rta_payload + 1 <= sizeof (ifas[ifa_index].name))
                        {
                          ifas[ifa_index].ifa.ifa_name = ifas[ifa_index].name;
                          *(char *) __mempcpy (ifas[ifa_index].name, rta_data,
                                               rta_payload) = '\0';
                        }
                      else
                        {
                          abort ();
                        }
                      break;

                    case IFA_UNSPEC:
                      break;
                    case IFA_CACHEINFO:
                      break;
                    default:
                      break;
                    }

                  rta = RTA_NEXT (rta, rtasize);
                }

              /* If we didn't get the interface name with the
                 address, use the name from the interface entry.  */
              if (ifas[ifa_index].ifa.ifa_name == NULL)
                {
                  ifas[ifa_index].ifa.ifa_name
                    = ifas[map_newlink (ifam->ifa_index - 1, ifas,
                                        map_newlink_data, newlink)].ifa.ifa_name;
                }

              /* Calculate the netmask.  */
              if (ifas[ifa_index].ifa.ifa_addr
                  && ifas[ifa_index].ifa.ifa_addr->sa_family != AF_UNSPEC
                  && ifas[ifa_index].ifa.ifa_addr->sa_family != AF_PACKET)
                {
                  uint32_t max_prefixlen = 0;
                  char *cp = NULL;

                  ifas[ifa_index].ifa.ifa_netmask
                    = &ifas[ifa_index].netmask.sa;

                  switch (ifas[ifa_index].ifa.ifa_addr->sa_family)
                    {
                    case AF_INET:
                      cp = (char *) &ifas[ifa_index].netmask.s4.sin_addr;
                      max_prefixlen = 32;
                      break;

                    case AF_INET6:
                      cp = (char *) &ifas[ifa_index].netmask.s6.sin6_addr;
                      max_prefixlen = 128;
                      break;
                    }

                  ifas[ifa_index].ifa.ifa_netmask->sa_family
                    = ifas[ifa_index].ifa.ifa_addr->sa_family;

                  if (cp != NULL)
                    {
                      char c;
                      unsigned int preflen;

                      if ((max_prefixlen > 0)
                          && (ifam->ifa_prefixlen > max_prefixlen))
                        {
                          preflen = max_prefixlen;
                        }
                      else
                        {
                          preflen = ifam->ifa_prefixlen;
                        }

                      for (i = 0; i < (preflen / 8); i++)
                        {
                          *cp++ = 0xff;
                        }
                      c = 0xff;
                      c <<= (8 - (preflen % 8));
                      *cp = c;
                    }
                }
            }
        }
    }

  NS_ASSERT (ifa_data_ptr <= (char *) &ifas[newlink + newaddr] + ifa_data_size);

  if (newaddr_idx > 0)
    {
      for (i = 0; i < newlink; ++i)
        {
          if (map_newlink_data[i] == -1)
            {
              /* We have fewer links then we anticipated.  Adjust the
                 forward pointer to the first address entry.  */
              ifas[i - 1].ifa.ifa_next = &ifas[newlink].ifa;
            }
        }

      if (i == 0 && newlink > 0)
        {
          /* No valid link, but we allocated memory.  We have to
             populate the first entry.  */
          memmove (ifas, &ifas[newlink], sizeof (struct ifaddrs_storage));
        }
    }

  *ifap = &ifas[0].ifa;

exit_free:
  __netlink_free_handle (&nh);
  dce_close (nh.fd);

  return 0;
}
