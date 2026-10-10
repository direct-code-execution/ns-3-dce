#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include "test-macros.h"

// Running out of file descriptors: socket() keeps failing with EMFILE, and
// does not leak the sockets it could not return.
int
main (int argc, char *argv[])
{
  std::vector<int> fds;
  while (true)
    {
      int fd = socket (AF_INET, SOCK_DGRAM, 0);
      if (fd < 0)
        {
          TEST_ASSERT_EQUAL (errno, EMFILE);
          break;
        }
      fds.push_back (fd);
    }
  for (int i = 0; i < 100; i++)
    {
      TEST_ASSERT_EQUAL (socket (AF_INET, SOCK_DGRAM, 0), -1);
      TEST_ASSERT_EQUAL (errno, EMFILE);
    }
  for (size_t i = 0; i < fds.size (); i++)
    {
      TEST_ASSERT_EQUAL (close (fds[i]), 0);
    }
  int fd = socket (AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT (fd >= 0);
  TEST_ASSERT_EQUAL (close (fd), 0);
  return 0;
}
