#include "memory-usage.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <sstream>
#include <string>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MemoryUsage");

static long AsLong (std::string str)
{
  std::istringstream iss;
  iss.str (str);
  long l;
  iss >> l;
  return l;
}

long GetCurrentMemoryUsage (void)
{
  int pipefd[2];
  int retval = ::pipe (pipefd);
  if (retval == -1)
    {
      NS_LOG_ERROR ("Cannot create pipe: " << ::strerror (errno));
      return 0;
    }
  pid_t pid = ::fork ();
  if (pid == 0)
    {
      // child.
      NS_LOG_DEBUG ("Child");
      ::close (pipefd[0]);
      int tmp;
      tmp = dup2 (pipefd[1], 1);
      if (tmp == -1)
	{
	  NS_LOG_ERROR ("Cannot redirect stdout");
	  ::exit (0);
	}
      retval = ::execl ("/usr/bin/smem", "smem", "-c", "pid uss", (char *)NULL);
      if (retval == -1)
	{
	  NS_LOG_ERROR ("Cannot execl smem: " << ::strerror (errno));
	}
      ::close (pipefd[1]);
      ::_exit (EXIT_SUCCESS);
    }
  else
    {
      // parent.
      NS_LOG_DEBUG ("Parent");
      ::close (pipefd[1]);
      std::string output;
      uint8_t c;
      ssize_t bytesRead = ::read (pipefd[0], &c, 1);
      while (bytesRead == 1)
	{
	  output.push_back (c);
	  bytesRead = ::read (pipefd[0], &c, 1);
	}
      waitpid (pid, 0, 0);
      ::close (pipefd[0]);

      pid = getpid ();
      std::istringstream iss;
      iss.str (output);
      while (!iss.eof ())
	{
	  std::string line;
	  std::getline (iss, line);
	  std::string::size_type cur = 0, next = 0;
	  cur = line.find_first_not_of (" \t", cur);
	  next = line.find_first_of (" \t", cur);
	  if (cur == std::string::npos || next == std::string::npos)
	    {
	      break;
	    }
	  std::string line_pid = line.substr(cur, next-cur);
	  cur = next+1;
	  cur = line.find_first_not_of (" \t", cur);
	  next = line.find_first_of (" \t", cur);
	  if (cur == std::string::npos || next == std::string::npos)
	    {
	      break;
	    }
	  std::string line_uss = line.substr(cur, next-cur);
	  if (AsLong (line_pid) == pid)
	    {
	      return AsLong (line_uss);
	    }
	}
    }
  return 0;
}

} // namespace ns3
