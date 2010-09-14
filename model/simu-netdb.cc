#include "simu-netdb.h"
#include "utils.h"
#include "simu-stdlib.h"
#include "simu-string.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("SimuNetDb");

using namespace ns3;

struct hostent *simu_gethostbyname(const char *name)
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
struct hostent *simu_gethostbyname2(const char *name, int af)
{
  NS_ASSERT (af == AF_INET);
  return simu_gethostbyname (name);
}
int simu_getaddrinfo(const char *node, const char *service,
		     const struct addrinfo *hints,
		     struct addrinfo **res)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << node << service << hints << res);
  NS_ASSERT (Current () != 0);
  struct addrinfo *tmp = 0;
  int status = ::getaddrinfo (node, service, hints, &tmp);
  // copy outgoing data structure so that the memory is allocated from the calling process memory pool.
  struct addrinfo *cur, *prev, *head;
  head = 0;
  prev = 0;
  for (cur = tmp; cur != 0; cur = cur->ai_next)
    {
      struct addrinfo *copy = (struct addrinfo*)simu_malloc (sizeof(struct addrinfo));
      memcpy (copy, cur, sizeof (struct addrinfo));
      copy->ai_addr = (struct sockaddr*)simu_malloc (cur->ai_addrlen);
      if (cur->ai_canonname != 0)
	{
	  copy->ai_canonname = simu_strdup (cur->ai_canonname);
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
      memset (*res, 0, sizeof (struct addrinfo));
    }
  ::freeaddrinfo (tmp);
  return status;
}
void simu_freeaddrinfo(struct addrinfo *res)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << res);
  NS_ASSERT (Current () != 0);
  struct addrinfo *cur, *next;
  for (cur = res; cur != 0; cur = next)
    {
      next = cur->ai_next;
      simu_free (cur->ai_addr);
      if (cur->ai_canonname != 0)
	{
	  simu_free (cur->ai_canonname);
	}
      simu_free (cur);
    }
}
const char *simu_gai_strerror(int errcode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << errcode);
  NS_ASSERT (Current () != 0);
  return ::gai_strerror (errcode);
}
