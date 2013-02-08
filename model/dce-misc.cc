#include "dce-misc.h"
#include "process.h"
#include "ns3/log.h"
#include "utils.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/node-list.h"
#include "dce-node-context.h"
#include "dce-manager.h"
#include "errno.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceMisc");

int dce_uname (struct utsname *buf)
{
  Ptr<DceNodeContext> nodeContext = DceNodeContext::GetNodeContext ();
  NS_ASSERT (0 != nodeContext);

  return nodeContext->UName (buf);
}

int dce_gethostname (char *name, size_t len)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current << UtilsGetNodeId ());
  NS_ASSERT (current != 0);

  if (!name)
    {
      current->err = EFAULT;
      return 1;
    }
  if (len <= 0)
    {
      current->err = EINVAL;
      return 1;
    }

  struct utsname tmp;

  dce_uname (&tmp);

  size_t sl = strlen (tmp.nodename);

  memset (name, 0, len);
  memcpy (name, &tmp.nodename, std::min (sl, len));

  return 0;
}
