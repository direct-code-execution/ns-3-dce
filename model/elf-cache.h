#ifndef ELF_CACHE_H
#define ELF_CACHE_H

#include <string>
#include <stdint.h>
#include <elf.h>
#include <link.h>
#include <vector>

namespace ns3 {

class ElfCache
{
public:
  ElfCache (std::string directory, uint32_t uid);

  struct ElfCachedFile
  {
    std::string cachedFilename;
    std::string basename;
    long data_p_vaddr;
    long data_p_memsz;
    uint32_t id;
    std::vector<uint32_t> deps;
  };
  struct ElfCachedFile Add (std::string filename);

private:
  struct FileInfo
  {
    long p_vaddr;
    long p_memsz;
    std::vector<uint32_t> deps;
  };
  struct Overriden
  {
    std::string from;
    std::string to;
  };
  std::string GetBasename (std::string filename) const;
  void CopyFile (std::string source, std::string destination) const;
  void WriteString (char *str, uint32_t uid) const;
  uint8_t NumberToChar (uint8_t c) const;
  static uint32_t AllocateId (void);
  struct FileInfo EditBuffer (uint8_t *map, uint32_t selfId) const;
  struct FileInfo EditFile (std::string filename, uint32_t selfId) const;
  uint32_t GetDepId (std::string depname) const;
  std::string EnsureCacheDirectory (void) const;
  unsigned long GetBaseAddress (ElfW (Phdr) * phdr, long phnum) const;
  long GetDtStrTab (ElfW (Dyn) * dyn, long baseAddress) const;


  std::string m_directory;
  uint32_t m_uid;
  std::vector<struct ElfCachedFile> m_files;
  std::vector<struct Overriden> m_overriden;
};

} // namespace ns3

#endif /* ELF_CACHE_H */
