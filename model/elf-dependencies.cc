#include "elf-dependencies.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ElfDependencies");

ElfDependencies::ElfDependencies (std::string filename)
{
  std::string fullname;
  bool found;
  found = SearchFile (filename, &fullname);
  NS_ASSERT (found);
  std::vector<struct Dependency> deps = GatherDependencies (fullname);
  std::reverse (deps.begin (), deps.end ());
  struct Dependency dependency;
  dependency.required = filename;
  dependency.found = fullname;
  deps.push_back (dependency);
  m_deps = deps;
}

std::vector<struct ElfDependencies::Dependency>
ElfDependencies::GatherDependencies (std::string fullname) const
{
  NS_LOG_FUNCTION (this << fullname);
  /* We gather the dependencies for the input file using the 'ldd' program.
   * To do this, we do a standard fork+exec and forward the output of ldd
   * to the parent through a pipe which is then parsed in the parent.
   */
  std::vector<struct Dependency> dependencies;

  int pipefd[2];
  int retval = ::pipe (pipefd);
  if (retval == -1)
    {
      NS_LOG_ERROR ("Cannot create pipe: " << ::strerror (errno));
      return dependencies;
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
      retval = ::execl ("/usr/bin/ldd", "ldd", fullname.c_str (), (char *)NULL);
      if (retval == -1)
        {
          NS_LOG_ERROR ("Cannot execl ldd: " << ::strerror (errno));
        }
      ::close (pipefd[1]);
      ::_exit (EXIT_SUCCESS);
    }
  else
    {
      // parent.
      NS_LOG_DEBUG ("Parent");
      ::close (pipefd[1]);
      std::string lddOutput;
      uint8_t c;
      ssize_t bytesRead = ::read (pipefd[0], &c, 1);
      while (bytesRead == 1)
        {
          lddOutput.push_back (c);
          bytesRead = ::read (pipefd[0], &c, 1);
        }
      waitpid (pid, 0, 0);
      ::close (pipefd[0]);
      std::string::size_type cur = 0;
      while (true)
        {
          NS_LOG_DEBUG ("line=" << lddOutput);
          std::string::size_type dep_start = lddOutput.find_first_not_of (" \t", cur);
          std::string::size_type dep_end = lddOutput.find (" ", dep_start);
          std::string::size_type full_start = lddOutput.find_first_of (">", dep_end);
          full_start = lddOutput.find_first_not_of (" \t", full_start + 1);
          std::string::size_type full_end = lddOutput.find_first_of (" \n", full_start);
          NS_LOG_DEBUG ("dep_start=" << dep_start << " dep_end=" << dep_end << " full_start=" << full_start << " full_end=" << full_end);
          if (dep_start != std::string::npos &&
              full_start != std::string::npos &&
              full_start > dep_start)
            {
              std::string depname = lddOutput.substr (dep_start, dep_end - dep_start);
              std::string fulldepname = lddOutput.substr (full_start, full_end - (full_start));
              NS_LOG_DEBUG (depname << "->" << fulldepname);
              if (depname == "linux-gate.so.1" ||
                  depname == "ld-linux.so.2" ||
                  depname == "ld-linux-x86-64.so.2" ||
                  depname == "/lib/ld-linux.so.2" ||
                  depname == "/lib64/ld-linux-x86-64.so.2" ||
                  depname == "/usr/lib/debug/ld-linux-x86-64.so.2" ||
                  depname == "linux-vdso.so.1")
                {
                  goto next;
                }
              else
                {
                  struct Dependency dependency;
                  dependency.required = depname;
                  dependency.found = fulldepname;
                  dependencies.push_back (dependency);
                }
            }
          else
            {
              break;
            }
next:
          cur = lddOutput.find_first_of ("\n", full_start);
          if (cur != std::string::npos)
            {
              cur++;
            }
          else
            {
              break;
            }
        }

    }

  return dependencies;
}


std::list<std::string>
ElfDependencies::Split (std::string input, std::string sep) const
{
  NS_LOG_FUNCTION (this << input << sep);
  std::list<std::string> retval;
  std::string::size_type cur = 0, next;
  while (true)
    {
      next = input.find (sep, cur);
      if (next == cur)
        {
          cur++;
          continue;
        }
      else if (next == std::string::npos)
        {
          if (input.size () != cur)
            {
              retval.push_back (input.substr (cur, input.size () - cur));
            }
          break;
        }
      retval.push_back (input.substr (cur, next - cur));
      cur = next + 1;
    }
  return retval;
}

std::list<std::string>
ElfDependencies::GetSearchDirectories (void) const
{
  NS_LOG_FUNCTION (this);
  std::list<std::string> directories;
  char *ldLibraryPath = getenv ("LD_LIBRARY_PATH");
  if (ldLibraryPath != 0)
    {
      std::list<std::string> tmp = Split (ldLibraryPath, ":");
      directories.insert (directories.end (), 
                          tmp.begin (),
                          tmp.end ());
    }
  char *path = getenv ("PATH");
  if (path != 0)
    {
      std::list<std::string> tmp = Split (path, ":");
      directories.insert (directories.end (), 
                          tmp.begin (),
                          tmp.end ());
    }
  directories.push_back ("/lib");
  directories.push_back ("/usr/lib");
  directories.push_back (".");
  return directories;
}

bool
ElfDependencies::Exists (std::string filename) const
{
  //NS_LOG_FUNCTION (this << filename);
  struct stat st;
  int retval = ::stat (filename.c_str (), &st);
  return retval == 0;
}

bool
ElfDependencies::SearchFile (std::string filename, std::string *fullname) const
{
  NS_LOG_FUNCTION (this << filename);
  if (Exists (filename))
    {
      *fullname = filename;
      NS_LOG_DEBUG ("Found: " << filename << " as " << *fullname);
      return true;
    }
  std::list<std::string> dirs = GetSearchDirectories ();
  for (std::list<std::string>::const_iterator i = dirs.begin (); i != dirs.end (); i++)
    {
      if (Exists (*i + "/" + filename))
        {
          *fullname = *i + "/" + filename;
          NS_LOG_DEBUG ("Found: " << filename << " as " << *fullname);
          return true;
        }
    }
  return false;
}


ElfDependencies::Iterator 
ElfDependencies::Begin (void) const
{
  return m_deps.begin ();
}

ElfDependencies::Iterator 
ElfDependencies::End (void) const
{
  return m_deps.end ();
}


} // namespace ns3
