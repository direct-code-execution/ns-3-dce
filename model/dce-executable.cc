#include "dce-executable.h"
#include "process.h"
#include "ns3/log.h"
#include "ns3/fatal-error.h"
#include <dlfcn.h>
#include <link.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("DceExecutable");

namespace ns3 {

// the room DCE_RESERVE_EXECUTABLE_TLS reserved, which ends at the thread pointer
static size_t g_reservedTls = 0;

static uint8_t *
GetThreadPointer (void)
{
  uint8_t *tp;
  asm volatile ("mov %%fs:0, %0" : "=r" (tp));
  return tp;
}

void
DceReserveExecutableTls (void *reserve, size_t size)
{
  // The executable's TLS block ends at the thread pointer: the room is
  // usable only if nothing comes after it in the block.
  if ((uint8_t *) reserve + size != GetThreadPointer ())
    {
      NS_LOG_UNCOND ("DCE_RESERVE_EXECUTABLE_TLS: the simulator's executable has other thread local "
                     "variables after the reserved room: programs loaded from an executable "
                     "cannot use their thread local variables");
      return;
    }
  g_reservedTls = size;
}

void *
DceExecutableMain (void *handle)
{
#if defined(__x86_64__)
  struct link_map *map = 0;
  if (dlinfo (handle, RTLD_DI_LINKMAP, &map) != 0 || map == 0)
    {
      return 0;
    }
  // the first PT_LOAD of a PIE maps the ELF header at the load address
  const ElfW (Ehdr) *header = (const ElfW (Ehdr) *) map->l_addr;
  if (memcmp (header->e_ident, ELFMAG, SELFMAG) != 0 || header->e_entry == 0)
    {
      return 0;
    }
  const uint8_t *start = (const uint8_t *)(map->l_addr + header->e_entry);
  for (int i = 0; i < 64; i++)
    {
      const uint8_t *p = start + i;
      int32_t disp;
      if (p[0] == 0xff && p[1] == 0x15)
        {
          // call *__libc_start_main@GOTPCREL(%rip): too far
          break;
        }
      if (p[0] == 0x48 && p[1] == 0x8d && p[2] == 0x3d)
        {
          // lea main(%rip), %rdi
          memcpy (&disp, p + 3, 4);
          return (void *)(p + 7 + disp);
        }
      if (p[0] == 0x48 && p[1] == 0x8b && p[2] == 0x3d)
        {
          // mov main@GOTPCREL(%rip), %rdi
          memcpy (&disp, p + 3, 4);
          return *(void **)(p + 7 + disp);
        }
    }
#endif
  return 0;
}

bool
DceExecutableTlsImage (std::string filename, std::vector<uint8_t> &image)
{
  image.clear ();
  int fd = ::open (filename.c_str (), O_RDONLY);
  if (fd == -1)
    {
      return false;
    }
  ElfW (Ehdr) header;
  std::vector<ElfW (Phdr)> phdrs;
  bool ok = ::pread (fd, &header, sizeof (header), 0) == sizeof (header)
    && memcmp (header.e_ident, ELFMAG, SELFMAG) == 0
    && header.e_ident[EI_CLASS] == ELFCLASS64
    && header.e_phentsize == sizeof (ElfW (Phdr));
  if (ok)
    {
      phdrs.resize (header.e_phnum);
      size_t size = header.e_phnum * sizeof (ElfW (Phdr));
      ok = ::pread (fd, phdrs.data (), size, header.e_phoff) == (ssize_t) size;
    }
  const ElfW (Phdr) *tls = 0;
  bool executable = false;
  for (size_t i = 0; ok && i < phdrs.size (); i++)
    {
      if (phdrs[i].p_type == PT_INTERP)
        {
          executable = true;
        }
      else if (phdrs[i].p_type == PT_TLS && phdrs[i].p_memsz != 0)
        {
          tls = &phdrs[i];
        }
    }
  if (ok && executable && tls != 0)
    {
      size_t align = tls->p_align ? tls->p_align : 1;
      image.resize ((tls->p_memsz + align - 1) / align * align, 0);
      ok = ::pread (fd, image.data (), tls->p_filesz, tls->p_offset) == (ssize_t) tls->p_filesz;
      NS_LOG_DEBUG (filename << ": executable with " << image.size () << " bytes of local-exec TLS");
    }
  ::close (fd);
  if (!ok)
    {
      image.clear ();
    }
  return !image.empty ();
}

void
DceExecutableTlsRelocate (void *handle, std::vector<uint8_t> &image)
{
  struct link_map *map = 0;
  if (image.empty () || dlinfo (handle, RTLD_DI_LINKMAP, &map) != 0 || map == 0)
    {
      return;
    }
  const ElfW (Ehdr) *header = (const ElfW (Ehdr) *) map->l_addr;
  const ElfW (Phdr) *phdr = (const ElfW (Phdr) *)(map->l_addr + header->e_phoff);
  for (int i = 0; i < header->e_phnum; i++)
    {
      if (phdr[i].p_type != PT_TLS)
        {
          continue;
        }
      const uint8_t *relocated = (const uint8_t *)(map->l_addr + phdr[i].p_vaddr);
      uint8_t *block = GetThreadPointer () - image.size ();
      for (size_t offset = 0; offset + sizeof (uint64_t) <= phdr[i].p_filesz; offset += sizeof (uint64_t))
        {
          uint64_t before, after, now;
          memcpy (&before, &image[offset], sizeof (before));
          memcpy (&after, relocated + offset, sizeof (after));
          memcpy (&now, block + offset, sizeof (now));
          if (before != after && now == before)
            {
              memcpy (block + offset, &after, sizeof (after));
            }
        }
      memcpy (image.data (), relocated, phdr[i].p_filesz);
    }
}

// the address of the program's own definition of a symbol, 0 if another
// module defines it
static void *
OwnSymbol (void *handle, struct link_map *map, const char *name)
{
  void *symbol = dlsym (handle, name);
  Dl_info info;
  struct link_map *owner = 0;
  if (symbol == 0 || dladdr1 (symbol, &info, (void **) &owner, RTLD_DL_LINKMAP) == 0 || owner != map)
    {
      return 0;
    }
  return symbol;
}

ProgramAllocator *
DceExecutableAllocator (void *handle)
{
  struct link_map *map = 0;
  if (dlinfo (handle, RTLD_DI_LINKMAP, &map) != 0 || map == 0)
    {
      return 0;
    }
  ProgramAllocator allocator;
  allocator.malloc = (void * (*)(size_t)) OwnSymbol (handle, map, "malloc");
  allocator.free = (void (*)(void *)) OwnSymbol (handle, map, "free");
  allocator.calloc = (void * (*)(size_t, size_t)) OwnSymbol (handle, map, "calloc");
  allocator.realloc = (void * (*)(void *, size_t)) OwnSymbol (handle, map, "realloc");
  allocator.posix_memalign = (int (*)(void **, size_t, size_t)) OwnSymbol (handle, map, "posix_memalign");
  allocator.malloc_usable_size = (size_t (*)(void *)) OwnSymbol (handle, map, "malloc_usable_size");
  if (allocator.malloc == 0 || allocator.free == 0 || allocator.calloc == 0 || allocator.realloc == 0)
    {
      return 0;
    }
  NS_LOG_DEBUG ("the program has its own allocator");
  return new ProgramAllocator (allocator);
}

void
DceExecutableTlsInit (const std::vector<uint8_t> &image)
{
  if (image.size () > g_reservedTls)
    {
      NS_FATAL_ERROR ("A program loaded from an executable keeps " << image.size () << " bytes of thread "
                      "local variables below the thread pointer, where the simulator's executable has "
                      << g_reservedTls << ": reserve the room with DCE_RESERVE_EXECUTABLE_TLS");
    }
  memcpy (GetThreadPointer () - image.size (), image.data (), image.size ());
}

} // namespace ns3
