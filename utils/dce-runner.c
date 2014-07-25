#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <link.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#ifndef __RTLD_OPENEXEC
// internal glibc flag that we also use with the same semantics.
#define __RTLD_OPENEXEC 0x20000000
#endif

static void *lookup_entry_point (void *h)
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

  void *entry = (void *)(map->l_addr + header.e_entry);
  close (fd);
  return entry;
}



int main (int argc, char *argv[])
{
  void *h = dlmopen (LM_ID_NEWLM, argv[1], RTLD_NOW | RTLD_GLOBAL | __RTLD_OPENEXEC);
  if (h == 0)
    {
      fprintf (stderr, "unable to open %s %s\n", argv[1], dlerror ());
      return 1;
    }
  Lmid_t lmid;
  int status = dlinfo (h, RTLD_DI_LMID, &lmid);
  if (status != 0)
    {
      fprintf (stderr, "unable to get lmid\n");
      return 1;
    }
  void *entry = lookup_entry_point (h);
  argv[-1] = (char*)((long)argc + 1);
  int i = 0;
  for (i = 0; i < argc-1; i++)
    {
      argv[i] = argv[i+1];
    }
  argv[argc-1] = "--ns3::DceManagerHelper::LoaderFactory=ns3::DlmLoaderFactory[]";
  argv[argc] = "--ns3::TaskManager::FiberManagerType=UcontextFiberManager";
#if defined (__x86_64__)
  __asm__ ("\txor %%rbp,%%rbp\n"
	   "\txor %%rdx,%%rdx\n"
	   "\tmov %0,%%rsp\n"
	   "\tjmp *%1\n" 
	   : 
	   : "r"(&argv[-1]), "r" (entry)
	   : "%rdx"
	   );
#elif defined (__i386__)
  __asm__ ("\txor %%ebp,%%ebp\n"
	   "\txor %%edx,%%edx\n"
	   "\tmov %0,%%esp\n"
	   "\tjmp *%1\n" 
	   : 
	   : "r"(&argv[-1]), "r" (entry)
	   : "%edx"
	   );
#else
#error "unsupported arch"
#endif
  // should never reach this point.
  exit (-1);
  return 0;
}
