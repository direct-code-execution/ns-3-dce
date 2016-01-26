#include "dce-umask.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <errno.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceUmask");

mode_t dce_umask (mode_t mask)
{
  Thread *current = Current ();
  NS_LOG_FUNCTION (current);
  NS_ASSERT (current != 0);

  mode_t omask;
  mask &= 0777;
  omask = current->process->uMask;
  current->process->uMask = mask;

  return omask;
}
