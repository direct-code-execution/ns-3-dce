#include "elf-cache.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/fatal-error.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <string.h>
#include <sys/mman.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ElfCache");

ElfCache::ElfCache (std::string directory, uint32_t uid)
  : m_directory (directory),
    m_uid (uid)
{
  struct Overriden overriden;
  overriden.from = "libc.so.6";
  overriden.to = "libc-ns3.so";
  m_overriden.push_back (overriden);
  overriden.from = "libpthread.so.0";
  overriden.to = "libpthread-ns3.so";
  m_overriden.push_back (overriden);
  overriden.from = "librt.so.1";
  overriden.to = "librt-ns3.so";
  m_overriden.push_back (overriden);
  overriden.from = "libm.so.6";
  overriden.to = "libm-ns3.so";
  m_overriden.push_back (overriden);
}

std::string
ElfCache::GetBasename (std::string filename) const
{
  std::string::size_type tmp = filename.find_last_of ("/");
  if (tmp == std::string::npos)
    {
      return filename;
    }
  return filename.substr (tmp + 1, filename.size () - (tmp + 1));
}

void
ElfCache::CopyFile (std::string source, std::string destination) const
{
  NS_LOG_FUNCTION (this << source << destination);
  int src = open (source.c_str (), O_RDONLY);
  int dst = open (destination.c_str (), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  uint8_t buffer[1024];
  ssize_t bytes_read = read (src, buffer, 1024);
  while (bytes_read > 0)
    {
      ssize_t bytes_written = 0;
      while (bytes_written != bytes_read)
        {
          ssize_t written = write (dst, buffer + bytes_written, bytes_read - bytes_written);
          bytes_written += written;
        }
      bytes_read = read (src, buffer, 1024);
    }
  close (src);
  close (dst);
  NS_LOG_DEBUG ("copied " << source << " to " << destination);
}

long
ElfCache::GetDtStrTab (ElfW(Dyn) *dyn, long baseAddress) const
{
  bool prelinked = false;
  long dt_strtab = 0;
  while (dyn->d_tag != DT_NULL)
    {
      if (dyn->d_tag == DT_STRTAB)
        {
          dt_strtab = dyn->d_un.d_val;
        }
      else if (dyn->d_tag == DT_GNU_PRELINKED)
        {
          prelinked = true;
        }
      dyn++;
    }
  if (prelinked)
    {
      dt_strtab -= baseAddress;
    }
  return dt_strtab;
}

unsigned long
ElfCache::GetBaseAddress (ElfW(Phdr) *phdr, long phnum) const
{
  unsigned long end = ~0; // the max value storable in a uint64_t
  unsigned long base = end;
  for (long i = 0; i < phnum; i++, phdr++)
    {
      if (phdr->p_type == PT_LOAD)
        {
          unsigned long ph_base = phdr->p_vaddr & ~(phdr->p_align - 1);
          if (ph_base < base)
            {
              base = ph_base;
            }
        }
    }
  if (base == end)
    {
      NS_LOG_ERROR ("Could not find base address.");
    }
  return base;
}


uint8_t
ElfCache::NumberToChar (uint8_t c) const
{
  NS_ASSERT (c <= 60);
  if (c < 10)
    {
      return c + 0x30;
    }
  else if (c < (10 + 25))
    {
      return c - 10 + 0x41;
    }
  else if (c < (10 + 25 + 25))
    {
      return c - (10 + 25) + 0x61;
    }
  return 0; // quiet compiler
}
void
ElfCache::WriteString (char *str, uint32_t uid) const
{
  // we use base 4 chars in base 60.
  if (uid >= 60 * 60 * 60 * 60)
    {
      NS_FATAL_ERROR ("Please, report a bug: not enough unique strings for loaded code.");
    }
  uint8_t a = uid % 60;
  uint8_t b = (uid / 60) % 60;
  uint8_t c = (uid / 3600) % 60;
  uint8_t d = (uid / 216000) % 60;
  str[0] = NumberToChar (d);
  str[1] = NumberToChar (c);
  str[2] = NumberToChar (b);
  str[3] = NumberToChar (a);
  NS_LOG_DEBUG ("wrote " << str);
}

struct ElfCache::FileInfo
ElfCache::EditBuffer (uint8_t *map, uint32_t selfId) const
{
  ElfW (Ehdr) *header = (ElfW (Ehdr) *)map;
  ElfW (Phdr) * phdr = (ElfW (Phdr) *)(map + header->e_phoff);
  ElfW (Dyn) * dyn = 0;
  long base_address = GetBaseAddress (phdr, header->e_phnum);

  // find DYNAMIC and fill DataSection
  struct FileInfo fileInfo;
  ElfW (Phdr) * pt_load_rw = 0;
  ElfW (Phdr) * pt_gnu_relro = 0;
  for (uint32_t i = 0; i < header->e_phnum; i++, phdr++)
    {
      switch (phdr->p_type)
        {
        case PT_LOAD:
          if (phdr->p_flags & PF_W)
            {
              // data section !
              pt_load_rw = phdr;
            }
          break;
        case PT_DYNAMIC:
          // now, seek DT_NEEDED
          dyn = (ElfW (Dyn) *)(map + phdr->p_offset);
          break;
        case PT_GNU_RELRO:
          pt_gnu_relro = phdr;
          break;
        }
    }
  NS_ASSERT (pt_load_rw != 0);
  fileInfo.p_vaddr = pt_load_rw->p_vaddr;
  fileInfo.p_memsz = pt_load_rw->p_memsz;
  if (pt_gnu_relro != 0)
    {
      NS_ASSERT (pt_gnu_relro->p_vaddr == pt_load_rw->p_vaddr);
      fileInfo.p_vaddr += pt_gnu_relro->p_memsz;
      fileInfo.p_memsz -= pt_gnu_relro->p_memsz;
    }

  // first, Patch the DT_NEEDED, and, DT_SONAME entries
  // and save the DT_INIT entry
  long dt_strtab = GetDtStrTab (dyn, base_address);
  long dt_init = 0;
  ElfW (Dyn) * cur = dyn;
  while (cur->d_tag != DT_NULL)
    {
      if (cur->d_tag == DT_NEEDED)
        {
          char *needed = (char *)(map + dt_strtab + cur->d_un.d_val);
          if (std::string (needed) != "ld-linux-x86-64.so.2"
              && std::string (needed) != "ld-linux.so.2")
            {
              uint32_t id = GetDepId (needed);
              fileInfo.deps.push_back (id);
              WriteString (needed, id);
            }
        }
      else if (cur->d_tag == DT_SONAME)
        {
          char *soname = (char *)(map + dt_strtab + cur->d_un.d_val);
          WriteString (soname, selfId);
        }
      else if (cur->d_tag == DT_INIT)
        {
          dt_init = cur->d_un.d_val;
        }
      cur++;
    }
  // then, eliminate DT_FINI, DT_FINI_ARRAY and DT_FINI_ARRAYSZ
  cur = dyn;
  while (cur->d_tag != DT_NULL)
    {
      if (cur->d_tag == DT_FINI)
        {
          cur->d_tag = DT_INIT;
          cur->d_un.d_val = dt_init;
        }
      else if (cur->d_tag == DT_FINI_ARRAYSZ)
        {
          cur->d_un.d_val = 0;
        }
      cur++;
    }
  return fileInfo;
}

struct ElfCache::FileInfo
ElfCache::EditFile (std::string filename, uint32_t selfId) const
{
  NS_LOG_FUNCTION (this << filename);
  int fd = ::open (filename.c_str (), O_RDWR);
  NS_ASSERT_MSG (fd != -1, "unable to open file=" << filename << " error=" << strerror (errno));
  struct stat st;
  int retval = ::fstat (fd, &st);
  NS_ASSERT_MSG (retval == 0, "unable to fstat file=" << filename << " error=" << strerror (errno));
  uint64_t size = st.st_size;
  uint8_t *buffer = (uint8_t *) ::mmap (0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  NS_ASSERT_MSG (buffer != MAP_FAILED, "unable to mmap file=" << filename << " error=" << strerror (errno));
  close (fd);

  struct FileInfo fileInfo = EditBuffer (buffer, selfId);

  // write back changes to hard disk
  retval = ::msync (buffer, size, MS_SYNC);
  NS_ASSERT_MSG (retval == 0, "msync failed " << strerror (errno));

  retval = ::munmap (buffer, size);
  NS_ASSERT_MSG (retval == 0, "munmap failed " << strerror (errno));

  return fileInfo;
}

uint32_t
ElfCache::AllocateId (void)
{
  static uint32_t id = 0;
  id++;
  return id;
}

uint32_t
ElfCache::GetDepId (std::string depname) const
{
  for (std::vector<struct Overriden>::const_iterator i = m_overriden.begin (); i != m_overriden.end (); ++i)
    {
      struct Overriden overriden = *i;
      if (overriden.from == depname)
        {
          depname = overriden.to;
        }
      NS_LOG_DEBUG ("from: " << overriden.from << ", to: " << overriden.to);
    }
  for (std::vector<struct ElfCachedFile>::const_iterator i = m_files.begin (); i != m_files.end (); ++i)
    {
      NS_LOG_DEBUG ("cache: " << i->basename);
      if (depname == i->basename)
        {
          return i->id;
        }
    }
  NS_ASSERT_MSG (false, "did not find " << depname);
  return 0; // quiet compiler
}

std::string
ElfCache::EnsureCacheDirectory (void) const
{
  int retval = ::mkdir (m_directory.c_str (), S_IRWXU);
  if (retval == 0)
    {
      NS_LOG_DEBUG ("Created elf loader cache directory.");
    }
  std::ostringstream oss;
  oss << m_directory << "/" << m_uid;
  retval = ::mkdir (oss.str ().c_str (), S_IRWXU);
  if (retval == 0)
    {
      NS_LOG_DEBUG ("Created elf loader cache directory.");
    }
  return oss.str ();
}

struct ElfCache::ElfCachedFile
ElfCache::Add (std::string filename)
{
  NS_LOG_FUNCTION (this << filename);
  std::string basename = GetBasename (filename);
  // check if we have an override rule for this file
  for (std::vector<struct Overriden>::const_iterator i = m_overriden.begin (); i != m_overriden.end (); ++i)
    {
      struct Overriden overriden = *i;
      if (overriden.from == basename)
        {
          // check if the overriden file is already in-store.
          for (std::vector<struct ElfCachedFile>::const_iterator i = m_files.begin (); i != m_files.end (); ++i)
            {
              if (i->basename == overriden.to)
                {
                  return *i;
                }
            }
          NS_ASSERT (false);
        }
    }

  // check if the file is already in-store.
  for (std::vector<struct ElfCachedFile>::const_iterator i = m_files.begin (); i != m_files.end (); ++i)
    {
      if (i->basename == basename)
        {
          return *i;
        }
    }

  std::string directory = EnsureCacheDirectory ();
  std::string fileCopy = directory + "/" + basename;
  CopyFile (filename, fileCopy);

  uint32_t selfId = AllocateId ();

  struct FileInfo fileInfo = EditFile (fileCopy, selfId);

  struct ElfCachedFile cached;
  cached.cachedFilename = fileCopy;
  cached.basename = basename;
  cached.data_p_vaddr = fileInfo.p_vaddr;
  cached.data_p_memsz = fileInfo.p_memsz;
  cached.id = selfId;
  cached.deps = fileInfo.deps;

  m_files.push_back (cached);
  return cached;
}


} // namespace ns3
