#define _GNU_SOURCE 1
#include "system-wrappers.h"
#include "trampoline-manager.h"
#include "dce-fcntl.h"
#include "dce-unistd.h"
#include "sys/dce-stat.h"
#include "ns3/assert.h"
#include <sys/syscall.h>
#include <dlfcn.h>
#include <link.h>
#include <elf.h>
#include <stdlib.h>
#include <string>


namespace ns3 {

enum State {
  ENABLED,
  NOOP,
  DISABLED
};
static enum State g_state = DISABLED;

extern "C" int system_open(const char *pathname, int flags, mode_t mode)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      {
	long int status = syscall (__NR_open, (unsigned long)pathname, flags, mode);
	return status;
      }
    case ENABLED:
      {
	g_state = DISABLED;
	ssize_t retval = dce_open (pathname, flags, mode);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;  
}
extern "C" int system_close (int fd)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      {
	long int status = syscall (SYS_close, (unsigned long)fd);
	return status;
      }
    case ENABLED:
      {
	g_state = DISABLED;
	ssize_t retval = dce_close (fd);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}
extern "C" ssize_t system_write(int fd, const void *buf, size_t count)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      {
	long int status = syscall (SYS_write, fd, (unsigned long)buf, count);
	return status;
      }
    case ENABLED:
      {
	g_state = DISABLED;
	ssize_t retval = dce_write (fd, buf, count);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}
extern "C" ssize_t system_read(int fd, void *buf, size_t count)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      {
	long int status = syscall (SYS_read, fd, (unsigned long)buf, count);
	return status;
      }
    case ENABLED:
      {
	g_state = DISABLED;
	ssize_t retval = dce_read (fd, buf, count);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}
extern "C" ssize_t system_fcntl (int fd, int cmd, long arg)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      {
	long int status = syscall (SYS_fcntl, fd, cmd, arg);
	return status;
      }
    case ENABLED:
      {
	g_state = DISABLED;
	ssize_t retval = dce_fcntl (fd, cmd, arg);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}

extern "C" off_t system_lseek(int fd, off_t offset, int whence)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      {
	long int status = syscall (SYS_lseek, fd, offset, whence);
	return status;
      }
    case ENABLED:
      {
	g_state = DISABLED;
	off_t retval = dce_lseek (fd, offset, whence);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}
#if defined (__i386__)
extern "C" off64_t system_llseek(int fd, off64_t offset, int whence)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      {
	off64_t result;
	off_t offset_high = (offset >> 32) & 0xffffffff;
	off_t offset_low = offset & 0xffffffff;
	int status = syscall (SYS__llseek, fd, offset_high, offset_low, &result, whence);
	if (status == 0)
	  {
	    return result;
	  }
	return status;
      }
    case ENABLED:
      {
	g_state = DISABLED;
	off64_t retval = dce_lseek64 (fd, offset, whence);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}


extern "C" int system_fxstat64(int ver, int fd, struct stat64 *buf)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      if (ver == 0)
	{
	  long int status = syscall (SYS_fstat64, fd, buf);
	  return status;
	}
      else
	{
	  // the structure expected by the kernel for fstat
	  struct kernel_stat 
	  {
	    unsigned long long st_dev;
	    unsigned char __pad0[4];
	    unsigned long __st_ino;
	    unsigned int st_mode;
	    unsigned int st_nlink;
	    unsigned long st_uid;
	    unsigned long st_gid;
	    unsigned long long st_rdev;
	    unsigned char __pad3[4];
	    unsigned long long st_size;
	    unsigned long st_blksize;
	    unsigned long long st_blocks;
	    unsigned long st_atim;
	    unsigned long st_atim_nsec;
	    unsigned long st_mtim;
	    unsigned int st_mtim_nsec;
	    unsigned long st_ctim;
	    unsigned long st_ctim_nsec;
	    unsigned long long st_ino;
	  } kbuf;
	  long int status = syscall (SYS_fstat64, fd, &kbuf);
	  buf->st_dev = kbuf.st_dev;
	  buf->st_ino = kbuf.st_ino;
	  buf->st_mode = kbuf.st_mode;
	  buf->st_nlink = kbuf.st_nlink;
	  buf->st_uid = kbuf.st_uid;
	  buf->st_gid = kbuf.st_gid;
	  buf->st_rdev = kbuf.st_rdev;
	  buf->st_size = kbuf.st_size;
	  buf->st_blksize = kbuf.st_blksize;
	  buf->st_blocks = kbuf.st_blocks;
	  buf->st_atime = kbuf.st_atim;
	  buf->st_mtime = kbuf.st_mtim;
	  buf->st_ctime = kbuf.st_ctim;
	  return status;
	}
      break;
    case ENABLED:
      {
	g_state = DISABLED;
	off_t retval = dce_fxstat64 (ver, fd, buf);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}
extern "C" int system_fxstat(int ver, int fd, struct stat *buf)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      if (ver == 0)
	{
	  long int status = syscall (SYS_fstat, fd, buf);
	  return status;
	}
      else
	{
	  // the structure expected by the kernel for fstat
	  struct kernel_stat 
	  {
	    unsigned long st_dev;
	    unsigned long st_ino;
	    unsigned short st_mode;
	    unsigned short st_nlink;
	    unsigned short st_uid;
	    unsigned short st_gid;
	    unsigned long st_rdev;
	    unsigned long st_size;
	    unsigned long st_blksize;
	    unsigned long st_blocks;
	    unsigned long st_atim;
	    unsigned long st_atim_nsec;
	    unsigned long st_mtim;
	    unsigned long st_mtim_nsec;
	    unsigned long st_ctim;
	    unsigned long st_ctim_nsec;
	    unsigned long __unused4;
	    unsigned long __unused5;
	  } kbuf;
	  long int status = syscall (SYS_fstat, fd, &kbuf);
	  buf->st_dev = kbuf.st_dev;
	  buf->st_ino = kbuf.st_ino;
	  buf->st_mode = kbuf.st_mode;
	  buf->st_nlink = kbuf.st_nlink;
	  buf->st_uid = kbuf.st_uid;
	  buf->st_gid = kbuf.st_gid;
	  buf->st_rdev = kbuf.st_rdev;
	  buf->st_size = kbuf.st_size;
	  buf->st_blksize = kbuf.st_blksize;
	  buf->st_blocks = kbuf.st_blocks;
	  buf->st_atime = kbuf.st_atim;
	  buf->st_mtime = kbuf.st_mtim;
	  buf->st_ctime = kbuf.st_ctim;
	  return status;
	}
      break;
    case ENABLED:
      {
	g_state = DISABLED;
	off_t retval = dce_fxstat (ver, fd, buf);
	g_state = ENABLED;
	return retval;
      }
    }
  // quiet compiler
  return 0;
}

#elif defined (__x86_64__)

extern "C" int system_fxstat(int ver, int fd, struct stat *buf)
{
  switch (g_state)
    {
    case NOOP:
      return -1;
    case DISABLED:
      if (ver == 0)
	{
	  long int status = syscall (SYS_fstat, fd, buf);
	  return status;
	}
      else
	{
	  // the structure expected by the kernel for fstat
	  struct kernel_stat
	  {
	    unsigned long   st_dev;
	    unsigned long   st_ino;
	    unsigned long   st_nlink;
	    unsigned int    st_mode;
	    unsigned int    st_uid;
	    unsigned int    st_gid;
	    unsigned int    __pad0;
	    unsigned long   st_rdev;
	    long            st_size;
	    long            st_blksize;
	    long            st_blocks;
	    unsigned long   st_atim;
	    unsigned long   st_atim_nsec;
	    unsigned long   st_mtim;
	    unsigned long   st_mtim_nsec;
	    unsigned long   st_ctim;
	    unsigned long   st_ctim_nsec;
	    long            __unused[3];
	  } kbuf;
	  long int status = syscall (SYS_fstat, fd, &kbuf);
	  buf->st_dev = kbuf.st_dev;
	  buf->st_ino = kbuf.st_ino;
	  buf->st_mode = kbuf.st_mode;
	  buf->st_nlink = kbuf.st_nlink;
	  buf->st_uid = kbuf.st_uid;
	  buf->st_gid = kbuf.st_gid;
	  buf->st_rdev = kbuf.st_rdev;
	  buf->st_size = kbuf.st_size;
	  buf->st_blksize = kbuf.st_blksize;
	  buf->st_blocks = kbuf.st_blocks;
	  buf->st_atime = kbuf.st_atim;
	  buf->st_mtime = kbuf.st_mtim;
	  buf->st_ctime = kbuf.st_ctim;
	  return status;
	}
      break;
    case ENABLED:
      {
	g_state = DISABLED;
	off_t retval = dce_fxstat (ver, fd, buf);
	g_state = ENABLED;
	return retval;
      }
    } 
  // quiet compiler
  return 0;
}

#else
#error "Need a definition for this platform"
#endif


static ElfW(Shdr)
read_section (int fd, ElfW(Ehdr) header, int i)
{
  // helper functions for function below.
  off_t offset;
  offset = lseek (fd, header.e_shoff + i * header.e_shentsize, SEEK_SET);
  NS_ASSERT (offset != -1);
  ElfW(Shdr) section;
  ssize_t bytes_read;
  bytes_read = read (fd, &section, sizeof (section));
  NS_ASSERT (bytes_read == sizeof(section));
  return section;
}

static void *
lookup_symbol (unsigned int symbol_section_type, const char *library, const char *symbol)
{
  struct link_map *map;
  void *h = dlopen (library, RTLD_LAZY);
  int status = dlinfo (h, RTLD_DI_LINKMAP, &map);
  if (status == -1)
    {
      return 0;
    }
  const char *abs_filename = map->l_name;
  int fd = open (abs_filename, O_RDONLY);
  ElfW(Ehdr) header;
  ssize_t bytes_read = read (fd, &header, sizeof (header));
  if (bytes_read != sizeof (header))
    {
      close (fd);
      return 0;
    }
  unsigned long symtab_offset = 0;
  unsigned long symtab_size = 0;
  unsigned long symtab_strtab_index = 0;
  unsigned long symtab_entry_size = 0;
  for (int i = 0; i < header.e_shnum; i++)
    {
      ElfW(Shdr) section = read_section (fd, header, i);
      if (section.sh_type == symbol_section_type)
	{
	  symtab_offset = section.sh_offset;
	  symtab_size = section.sh_size;
	  symtab_strtab_index = section.sh_link;
	  symtab_entry_size = section.sh_entsize;
	  break;
	}
    }
  if (symtab_offset == 0)
    {
      return 0;
    }
  ElfW(Shdr) section = read_section (fd, header, symtab_strtab_index);
  unsigned long strtab_offset = section.sh_offset;
  unsigned long strtab_size = section.sh_size;

  char *strtab = (char *)malloc (strtab_size);
  lseek (fd, strtab_offset, SEEK_SET);
  read (fd, strtab, strtab_size);

  ElfW(Sym) *symtab = (ElfW(Sym)*)malloc (symtab_size);
  lseek (fd, symtab_offset, SEEK_SET);
  read (fd, symtab, symtab_size);

  unsigned long symbol_value = 0;
  for (unsigned int i = 0; i < symtab_size / symtab_entry_size; i++)
    {
      if (symtab[i].st_name == 0)
	{
	  continue;
	}
      char *str = &strtab[symtab[i].st_name];
      if (std::string (str) != symbol)
	{
	  continue;
	}
      // yay ! we have our symbol.
      symbol_value = symtab[i].st_value;
      break;
    }

  if (symbol_value == 0)
    {
      return 0;
    }

  // add base address of library.
  symbol_value += map->l_addr;

  free (strtab);
  free (symtab);
  close (fd);

  return (void*)symbol_value;
}

static void *
lookup_symbol (const char *library, const char *symbol)
{
  /**
   * We have to perform the symbol lookup ourselves because dlsym
   * does not work for us. i.e., dlsym searches only for symbols
   * exported by the dynamic symbol section of a binary while what
   * we need here is to find symbols which are not exported
   * from the binary. So, we have to search for the SHT_SYMTAB
   * section and parse it until we find our target symbols.
   */
  void *address = lookup_symbol (SHT_SYMTAB, library, symbol);
  if (address == 0)
    {
      address = lookup_symbol (SHT_DYNSYM, library, symbol);
    }
  return address;
}


static void initialize_wrappers (void)
{
  static bool initialized = false;
  if (initialized)
    {
      return;
    }
  initialized = true;
  struct Wrapper {
    const char *name;
    void * wrapper;
  } wrappers [] = {
    {"__open", (void *)&system_open},
    {"__open_nocancel", (void *)&system_open},
    {"__close", (void *)&system_close},
    {"__close_nocancel", (void *) &system_close},
    {"__write", (void *)&system_write},
    {"__read", (void *)&system_read},
    {"__fcntl", (void *)&system_fcntl},
    {"__lseek", (void *)&system_lseek},
    {"__fxstat", (void *)&system_fxstat},
#if defined (__i386__)
    {"__llseek", (void *)&system_llseek},
    {"__fxstat64", (void *)&system_fxstat64},
#endif
    // XXX: others ?
  };

  TrampolineManager *t = TrampolineManager::Instance ();
  for (int i = 0; i != sizeof(wrappers)/sizeof(struct Wrapper); i++)
    {
      void *symbol = lookup_symbol ("libc.so.6", wrappers[i].name);
      bool ok;
      ok = t->Insert ((unsigned long)symbol, (unsigned long)wrappers[i].wrapper);
      NS_ASSERT (ok);      
    }
}
void SystemWrappersEnable (void)
{
  initialize_wrappers ();
  g_state = ENABLED;
}
void SystemWrappersEnableNoop (void)
{
  initialize_wrappers ();
  g_state = NOOP;
}
void SystemWrappersDisable (void)
{
  g_state = DISABLED;
}


} // namespace ns3
