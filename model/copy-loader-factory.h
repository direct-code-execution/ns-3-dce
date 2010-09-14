#ifndef COPY_LOADER_FACTORY_H
#define COPY_LOADER_FACTORY_H

#include "loader-factory.h"

#include <string>
#include <list>
#include <stdint.h>

namespace ns3 {

/**
 * The goal of the elf loader is two-fold:
 *   1) it must virtualize the global data of a program when it is 
 *     loaded in memory (as well as the global data of each of the libraries
 *     the main binary depends on)
 *   2) it must allow you to replace the system libc with a new libc
 *     which forwards all socket calls to per-node simulation sockets.
 *
 * In ELF systems, all per-binary global data is accessed as relative offsets
 * from the base address of the code so, all you need to do to virtualize
 * the data global data area (that is, achieve 1) of a binary is to load 
 * it multiple times in memory and, each time, at a different base address. 
 *
 * To replace the system libc, you need to make the ELF loader look for a
 * libc.so.6 in a new location.
 *
 * Achieving both of the above could be done by re-implementing a new ELF 
 * loader or by using a modified version of the glibc ELF loader. The latter
 * is much harder than it seems and the former would require quite a bit of
 * platform-specific code so, this class implements another option which is
 * to trick the normal glibc ELF loader into doing just what we need, that is:
 *   - allow us to load the same binary multiple times in memory
 *   - allow us to replace the libc binary.
 *
 * By default, when you request to load the same binary twice with dlopen,
 * the loader is very smart and figures out that the requested file is the same
 * that is, it has the same inode. So to trick the loader into loading the
 * file twice (or more), we have to load the same file with a different inode
 * and the only way to do this is to physically copy the file on the hard disk
 * before loading it.
 *
 * To ensure that we can precisely control which libraries are (re)loaded in 
 * memory and that we can thus replace the system libc with a simulation libc,
 * we also modify the copied elf files by changing the set of libraries they 
 * depend upon. To do this, we have to change the values referenced by the
 * DT_NEEDED entries of the SHT_DYNAMIC section. Unfortunately, we can't just
 * add a new string table section, copy the data from the old string table 
 * section, and add new entries for the new filenames because doing this
 * would require a lot of ajustement to the elf data structures. So, instead,
 * we edit in-place the content of the string table pointed to by DT_STRTAB and,
 * to do this successfully, we need to make sure that the new filenames have
 * the exact same length in bytes as the old filenames: if they are not the
 * same length, we would have to adjust the elf data structures quite a bit.
 *
 * So, we choose the name of each new file based on a unique 3-letter prefix
 * which replaces the first 3 letters of the original file. The 3-letter prefix
 * is an ascii representation of a 3-unit base 60 number which potentially allows
 * us to load up to 60^3 elf binaries with unique filenames.
 *
 * Note that the biggest problem with this loader is that it's not possible to
 * make it force unloading a library without running its fini functions
 * which brings a host of problems when ns3::DceManager needs to terminate
 * a process.
 */
class CopyLoaderFactory : public LoaderFactory
{
public:
  static TypeId GetTypeId (void);

  CopyLoaderFactory ();
  virtual ~CopyLoaderFactory ();
  virtual Loader *Create (int argc, char **argv, char **envp);

private:
  static uint32_t AllocateUid (void);
};

} // namespace ns3

#endif /* COPY_LOADER_FACTORY_H */
