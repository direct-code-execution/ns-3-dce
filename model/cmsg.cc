#include "cmsg.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <sys/socket.h>
#include <string.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DceCmsg");

Cmsg::Cmsg (struct msghdr *msg)
  : m_msg (msg),
    m_current ((uint8_t *)msg->msg_control),
    m_len (msg->msg_controllen)
{
  NS_LOG_FUNCTION(msg);
  NS_ASSERT(msg);
}

bool
Cmsg::Add (int level, int type, int len, const uint8_t *buffer)
{
  NS_LOG_DEBUG(level << type << len << buffer);
  int cmsglen = CMSG_SPACE (len);
  if (m_len < cmsglen)
    {
      NS_LOG_WARN("Can't append message: not enough space");
      m_msg->msg_flags |= MSG_CTRUNC;
      return false;
    }
  struct cmsghdr msghdr;
  msghdr.cmsg_len = cmsglen;
  msghdr.cmsg_level = level;
  msghdr.cmsg_type = type;
  memcpy (m_current, &msghdr, sizeof (struct cmsghdr));
  memcpy (CMSG_DATA ((struct cmsghdr*)m_current), buffer, cmsglen - sizeof (struct cmsghdr));
  m_current += cmsglen;
  m_len -= cmsglen;
  return true;
}


int
Cmsg::GetNext (int *level, int *type, int *len, uint8_t **buffer)
{
  struct cmsghdr *cm = NULL;

  if (m_len < (int)CMSG_LEN (0))
    {
      return -1;
    }

  cm = (struct cmsghdr *)m_current;
  if (cm->cmsg_len == 0 || (int)cm->cmsg_len > m_len)
    {
      return -(1);
    }

  *level = cm->cmsg_level;
  *type = cm->cmsg_type;
  *len = cm->cmsg_len - CMSG_LEN (0);
  *buffer = CMSG_DATA (cm);

  m_current += CMSG_ALIGN (cm->cmsg_len);
  m_len -= CMSG_ALIGN (cm->cmsg_len);

  return 0;
}

void
Cmsg::Finish (void)
{
  NS_LOG_DEBUG("Updating msg_controllen from [" << m_msg->msg_controllen << "] to " << (m_msg->msg_controllen - m_len) << "\n");
  m_msg->msg_controllen -= m_len;
}


void
Cmsg::Print (std::ostream &os) const
{
    os << "#ElementsInVector=" << m_msg->msg_iovlen << ".";
}

std::ostream& operator<< (std::ostream& os, const Cmsg & address)
{
    address.Print(os);
    return os;
}

} // namespace ns3
