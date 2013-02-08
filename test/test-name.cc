#include <iostream>
#include <unistd.h>
#include <sys/utsname.h>
#include "test-macros.h"

int main (int argc, char *argv[])
{
  char gifbuffer [1024];
  struct utsname n;
  int ret = uname (&n);

  TEST_ASSERT_EQUAL (ret, 0);

  std::cout << "NodeName:" << n.nodename << std::endl;

  ret = gethostname (gifbuffer, sizeof (gifbuffer));

  TEST_ASSERT_EQUAL (ret, 0);

  std::cout << "Hostname:" << gifbuffer << std::endl;

  return 0;
}
