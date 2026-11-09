#include <net/if.h>
#include "test-macros.h"

// Interface indexes start at 1 (lo): with the ns-3 stack, NetDevice index
// + 1, as in netlink messages.
int
main (int argc, char *argv[])
{
  TEST_ASSERT_EQUAL (if_nametoindex ("lo"), 1);
  TEST_ASSERT_EQUAL (if_nametoindex ("nosuchdevice"), 0);
  return 0;
}
