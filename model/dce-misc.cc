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
  NS_ASSERT ( 0 != nodeContext );

  return nodeContext->UName (buf);
}
