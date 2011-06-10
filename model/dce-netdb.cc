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

NS_LOG_COMPONENT_DEFINE ("SimuNetDb");

using namespace ns3;

struct hostent *dce_gethostbyname(const char *name)
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
struct hostent *dce_gethostbyname2(const char *name, int af)
{
  NS_ASSERT (af == AF_INET);
  return dce_gethostbyname (name);
}
int dce_getaddrinfo(const char *node, const char *service,
		     const struct addrinfo *hints,
		     struct addrinfo **res)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << ((NULL==node)?"":node) << ((NULL==service)?"":service) << hints << res);
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
void dce_freeaddrinfo(struct addrinfo *res)
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
const char *dce_gai_strerror(int errcode)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << errcode);
  NS_ASSERT (Current () != 0);
  return ::gai_strerror (errcode);
}
int dce_getnameinfo (const struct sockaddr *sa, socklen_t salen, char *host,
                       socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
  NS_LOG_FUNCTION ( Current () );

  if ( ( 0 == sa ) || ( 0 == salen ) )
    {
      Current () -> err = EINVAL;
      return EAI_SYSTEM;
    }

  switch ( sa->sa_family )
  {
    case AF_INET:
      {
        if ( salen < sizeof( struct sockaddr_in ) )
          {
            Current () -> err = EINVAL;
            return EAI_SYSTEM;
          }
        const struct sockaddr_in *inAddr = (const struct sockaddr_in *) sa;

        if  ( 0 != serv )
          {
            int r = snprintf(serv, servlen, "%d",  htons(inAddr -> sin_port) );

            if ( r > servlen )
              {
                return EAI_OVERFLOW;
              }
            if ( r < 0 )
              {
                Current () -> err = errno;
                return EAI_SYSTEM;
              }
          }
        if  ( 0 != host )
          {
            Ipv4Address ipv4 = Ipv4Address ( htonl ( inAddr -> sin_addr.s_addr ) );
            std::ostringstream oss;
            ipv4.Print (oss);

            int r = snprintf(host, hostlen, "%s", oss.str().c_str());

            if ( r > servlen )
              {
                return EAI_OVERFLOW;
              }
            if ( r < 0 )
              {
                Current () -> err = errno;
                return EAI_SYSTEM;
              }
          }
        return 0;
      }
      break;

    default:
      return EAI_FAMILY;
  }

  return 0; // XXX : tricheur
}
