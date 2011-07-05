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

int dce_uname(struct utsname *buf)
{
  Thread *current = Current ();

  if (0 == buf)
    {
      current->err = EFAULT;
      return -1;
    }

  DceManager *manager = current->process->manager;
  NS_LOG_FUNCTION (current);
  NS_ASSERT (current != 0);
  NS_ASSERT (manager != 0);

  Ptr<DceNodeContext> nodeContext = manager->GetObject<DceNodeContext> ();

  if ( 0 == nodeContext )
    {
      nodeContext = CreateObject<DceNodeContext> ();
      manager->AggregateObject( nodeContext );
    }

  NS_ASSERT ( 0 != nodeContext );

  return nodeContext->UName (buf);

/*
TEMPOFUR
  */
}
