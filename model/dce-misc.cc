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
#include "sys/dce-utsname.h"
#include "dce-unistd.h"
#include "dirent.h"
#include "process.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceMisc");

void
CleanupPthreadKeys (void)
{
  ns3::Thread *current = Current ();
  // From this function, we perform process cleanup which _requires_
  // a user context. here delete the keys of each thread which might
  // require calling a key destructor in the process.
  for (std::list<ThreadKeyValue>::iterator j = current->keyValues.begin ();
       j != current->keyValues.end (); ++j)
    {
      NS_LOG_DEBUG ("destroy key " << j->key << " " << j->destructor << " " << j->value);
      if (j->destructor != 0 && j->value != 0)
        {
          void *v = j->value;
          // according to the posix spec, we must
          // set the value to zero before invoking the
          // destructor.
          j->value = 0;
          j->destructor (v);
        }
    }
  current->keyValues.clear ();
}


int dce_uname (struct utsname *buf)
{
  Ptr<DceNodeContext> nodeContext = DceNodeContext::GetNodeContext ();
  NS_ASSERT (0 != nodeContext);

  return nodeContext->UName (buf);
}

int dce_gethostname (char *name, size_t len)
{
  ns3::Thread *current = Current ();
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
