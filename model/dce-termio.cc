#include "dce-termio.h"
#include "utils.h"
#include "process.h"
#include "dce-manager.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <vector>
#include <errno.h>

NS_LOG_COMPONENT_DEFINE ("SimuTermio");

using namespace ns3;


int dce_tcgetattr (int fd, struct termios *termios_p)
{
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  current->err = ENOTTY;
  return -1;
}
int dce_tcsetattr (int fd, int optional_actions, const struct termios *termios_p)
{
  NS_ASSERT (Current () != 0);
  Thread *current = Current ();

  current->err = ENOTTY;
  return -1;
}
