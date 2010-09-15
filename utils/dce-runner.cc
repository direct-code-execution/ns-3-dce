#include <dlfcn.h>
#include "ns3/config.h"
#include "ns3/string.h"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <link.h>


using namespace ns3;

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
lookup_symbol (unsigned int symbol_section_type, void *h, const char *symbol)
{
  struct link_map *map;
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
lookup_symbol (void *h, const char *symbol)
{
  /**
   * We have to perform the symbol lookup ourselves because dlsym
   * does not work for us. i.e., dlsym searches only for symbols
   * exported by the dynamic symbol section of a binary while what
   * we need here is to find symbols which are not exported
   * from the binary. So, we have to search for the SHT_SYMTAB
   * section and parse it until we find our target symbols.
   */
  void *address = lookup_symbol (SHT_SYMTAB, h, symbol);
  if (address == 0)
    {
      address = lookup_symbol (SHT_DYNSYM, h, symbol);
    }
  return address;
}


int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::DceManagerHelper::LoaderFactory", StringValue ("ns3::DlmLoaderFactory[]"));
  argc--;
  argv++;
  void *h = dlopen (argv[0], RTLD_LAZY | RTLD_GLOBAL);
  if (h == 0)
    {
      std::cerr << "unable to open " << argv[0] << " " << dlerror () << std::endl;
      return 1;
    }
  void *symbol = lookup_symbol (h, "main");
  if (symbol == 0)
    {
      std::cerr << "unable to lookup main" << std::endl;
      return 1;
    }
  int (*main_symbol) (int, char **) = (int (*) (int, char **)) symbol;
  int retval = main_symbol (argc, argv);
  dlclose (h);
  return retval;
}
