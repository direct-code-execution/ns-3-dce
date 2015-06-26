// I do the version files for DCE

#define _GNU_SOURCE
#include <link.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>
#include <link.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

struct libinfo
{
  char *lib2find; // Name with .so and without path and without version numbers
  char *libfound; // Full path of the founded library
};

static int
callback (struct dl_phdr_info *info, size_t size, void *data)
{
  const char *fullName = info->dlpi_name;
  char *crsr = (char *)fullName;
  char *last = NULL;
  struct libinfo *ln = (struct libinfo*) data;

  // First seek for last '/'
  while (*crsr)
    {
      if ('/' == *crsr)
        {
          last = crsr;
        }
      *crsr++;
    }
  if (NULL == last)
    {
      return 0;
    }
  crsr = last;
  *crsr++;
  if ( strncmp (crsr, ln->lib2find, strlen (ln->lib2find) ))
    {
      return 0;
    }
  ln->libfound = strdup (fullName);
  return 1;
}

// Seek the full path of a library
// libName: basename of the lib without path and without version number for example libc.so
// Return an allocated string if found else NULL.
char * seek4Lib (char *libName)
{
  struct libinfo data;

  if (NULL == libName)
    {
      return NULL;
    }
  data.lib2find = libName;
  data.libfound = NULL;

  dl_iterate_phdr (callback, (void *) &data);

  return data.libfound;
}

int createversion (const char *input, const char *prefix,  const char *output)
{
  if (NULL == input)
    {
      return 1;
    }
  const char *filename = input;
  int fd = open (filename, O_RDONLY);
  struct stat buf;
  fstat (fd, &buf);
  unsigned long file = (unsigned long)mmap (0, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file == (unsigned long)MAP_FAILED)
    {
      return 1;
    }
  ElfW (Ehdr) * header = (ElfW (Ehdr) *)file;
  ElfW (Shdr) * sh = (ElfW (Shdr)*)(file + header->e_shoff);
  ElfW (Sym) * symtab = 0;
  unsigned long n_symtab = 0;
  ElfW (Half) * versym = 0;
  ElfW (Verdef) * verdef = 0;
  char *strtab = 0;
  int i;

  FILE *p = fopen (prefix, "r");

  FILE *f = fopen (output,"w");

  while (!feof (p))
    {
      char buf[1024];

      int l = fread (buf, 1, sizeof(buf), p);

      if (l > 0)
        {
          l = fwrite (buf, 1, l,f);
        }
    }
  fclose (p);

  if (NULL == f)
    {
      return 3;
    }

  for (i = 0; i < header->e_shnum; i++)
    {
      if (sh[i].sh_type == SHT_DYNSYM)
        {
          symtab = (ElfW (Sym)*)(file + sh[i].sh_offset);
          n_symtab = sh[i].sh_size / sh[i].sh_entsize;
        }
      else if (sh[i].sh_type == SHT_STRTAB && strtab == 0)
        {
          // XXX: should check the section name.
          strtab = (char*)(file + sh[i].sh_offset);
        }
      else if (sh[i].sh_type == SHT_GNU_versym)
        {
          versym = (ElfW (Half)*)(file + sh[i].sh_offset);
        }
      else if (sh[i].sh_type == SHT_GNU_verdef)
        {
          verdef = (ElfW (Verdef)*)(file + sh[i].sh_offset);
        }
    }
  if (strtab == 0 || verdef == 0
      || symtab == 0 || n_symtab == 0 || versym == 0)
    {
      return 3;
    }
  ElfW (Verdef) * cur, *prev;
  int local_passthru_printed = 0;
  for (prev = 0, cur = verdef;
       cur != prev;
       prev = cur, cur = (ElfW (Verdef)*)(((unsigned long)cur) + cur->vd_next))
    {
      assert (cur->vd_version == 1);
      assert (cur->vd_cnt == 2 || cur->vd_cnt == 1);
      ElfW (Verdaux) * first = (ElfW (Verdaux)*)(((unsigned long)cur) + cur->vd_aux);
      if (cur->vd_flags & VER_FLG_BASE)
        {
          continue;
        }
      fprintf (f,"%s {\n", strtab + first->vda_name);
      int has_one_symbol = 0;
      for (i = 0; i < n_symtab; i++)
        {
          if (symtab[i].st_name == 0 || symtab[i].st_value == 0)
            {
              continue;
            }
          ElfW (Half) ver = versym[i];
          if (cur->vd_ndx == ver)
            {
              if (!has_one_symbol)
                {
                  has_one_symbol = 1;
                  fprintf (f,"global:\n");
                }
              fprintf (f,"\t%s;\n", strtab + symtab[i].st_name);
            }
        }
      if (cur->vd_cnt == 1)
        {
          if (!local_passthru_printed)
            {
              local_passthru_printed = 1;
              fprintf (f,"local:*;\n};\n");
            }
          else
            {
              fprintf (f,"};\n");
            }
        }
      else
        {
          ElfW (Verdaux) * parent = (ElfW (Verdaux)*)(((unsigned long)first) + first->vda_next);
          fprintf (f,"} %s;\n", strtab + parent->vda_name);
        }
    }
  fclose (f);
  return 0;
}

int
main (int argc, char *argv[])
{
  int i = 0;
  createversion (seek4Lib ("libc.so"), argv[++i], "model/libc.version");
  createversion (seek4Lib ("libpthread.so"), argv[++i], "model/libpthread.version");
  createversion (seek4Lib ("librt.so"), argv[++i], "model/librt.version");
  createversion (seek4Lib ("libm.so"), argv[++i], "model/libm.version");
  createversion (seek4Lib ("libdl.so"), argv[++i], "model/libdl.version");

  return 0;
}
