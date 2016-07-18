#ifndef ELF_DEPENDENCIES_H
#define ELF_DEPENDENCIES_H

#include <string>
#include <vector>
#include <list>

namespace ns3 {

/**
 * List dependancies of a library or program
 */
class ElfDependencies
{
public:
  struct Dependency
  {
    std::string required;
    std::string found;
  };
  typedef std::vector<struct Dependency>::const_iterator Iterator;

  ElfDependencies (std::string filename, bool failsafe = false);

  Iterator Begin (void) const;
  Iterator End (void) const;

private:
  std::list<std::string> Split (std::string input, std::string sep) const;
  /**
   * Takes into account both PATH and LD_LIBRARY_PATH
   */
  std::list<std::string> GetSearchDirectories (void) const;
  bool Exists (std::string filename) const;
  /**
   *
   */
  bool SearchFile (std::string filename, std::string *dirname) const;
  /*
   * Lists dependancies of the file
   * Current implementation parses output of "ldd"
   */
  std::vector<struct Dependency> GatherDependencies (std::string fullname) const;
  std::vector<struct Dependency> NewGather (std::string sName, std::string fullname) const;

  std::vector<struct Dependency> m_deps;
};

} // namespace ns3


#endif /* ELF_DEPENDENCIES_H */
