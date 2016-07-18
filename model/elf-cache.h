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
    long data_p_vaddr;          /**!< Virtual address of data */
    
    long data_p_memsz;
    uint32_t id;                /**!< some random but unique number */
    std::vector<uint32_t> deps;
  };
  
  /**
   * Adds program or library to the cache, fixing ELF values for DCE needs
   */
  struct ElfCachedFile Add (std::string filename);

private:
  struct FileInfo
  {
    long p_vaddr;
    long p_memsz;
    std::vector<uint32_t> deps;
  };

  /**
   * Dictionary that maps a standard library SONAME to its DCE SONAME
   * so that DT_NEEDED sections can later be rearranged accordingly
   */
  struct Overriden
  {
    std::string from; /**!< original DT_NEEDED/DT_SONAME */
    std::string to;   /**!< new DT_NEEDED/DT_SONAME */
  };
  std::string GetBasename (std::string filename) const;
  void CopyFile (std::string source, std::string destination) const;
  void WriteString (char *str, uint32_t uid) const;
  uint8_t NumberToChar (uint8_t c) const;
  static uint32_t AllocateId (void);
  
  /**
   * Filters DT_NEEDED 
   * @param selfId uuid
   */
  struct FileInfo EditBuffer (uint8_t *map, uint32_t selfId) const;

  /**
   * Loads file into memory, update DT_SONAME and DT_NEEDED with DCE (=per node) 
   * values 
   * 
   * @param selfId uuid
   */
  struct FileInfo EditFile (std::string filename, uint32_t selfId) const;

  /**
   * replace depname (usually
   */
  uint32_t GetDepId (std::string depname) const;
  
  /**
   * Create cache folder if necessary, along with node id
   */
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
