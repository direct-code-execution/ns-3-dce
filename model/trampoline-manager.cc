#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include "ns3/fatal-error.h"
#include "trampoline-manager.h"

namespace {

static void
Copy (unsigned long to, uint8_t *from, uint32_t size)
{
  unsigned long page_start = to / 4096 * 4096;
  int status = mprotect ((void*)page_start, 4096, PROT_WRITE);
  if (status != 0)
    {
      NS_FATAL_ERROR ("Could not setup trampoline at=" << to);
    }
  memcpy ((void*)to, from, size);
  status = mprotect ((void *)page_start, 4096, PROT_READ | PROT_EXEC);
  if (status != 0)
    {
      NS_FATAL_ERROR ("Could not setup trampoline at=" << to);
    }
}

}

#if defined (__i386__)

namespace ns3 {

class TrampolineManagerImpl
{
public:
  TrampolineManagerImpl ();
  ~TrampolineManagerImpl ();
  bool Insert (unsigned long from, unsigned long to);
};

TrampolineManagerImpl::TrampolineManagerImpl ()
{}
TrampolineManagerImpl::~TrampolineManagerImpl ()
{}
bool 
TrampolineManagerImpl::Insert (unsigned long from, unsigned long to)
{
  signed long delta = to;
  delta -= from + 5;
  unsigned long delta_unsigned = delta;
  unsigned char buffer[5];
  buffer[0] = 0xe9;
  buffer[1] = (delta_unsigned >> 0) & 0xff;
  buffer[2] = (delta_unsigned >> 8) & 0xff;
  buffer[3] = (delta_unsigned >> 16) & 0xff;
  buffer[4] = (delta_unsigned >> 24) & 0xff;
  Copy (from, buffer, 5);
  return true;
}

} // namespace ns3

#elif defined (__x86_64__)

#define _GNU_SOURCE 1 // for dl_iterate_phdr
#include "trampoline-manager.h"
#include "ns3/fatal-error.h"
#include "ns3/assert.h"
#include <link.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

extern "C" struct dl_phdr_info;

namespace ns3 {

class TrampolineManagerImpl
{
public:
  TrampolineManagerImpl ();
  ~TrampolineManagerImpl ();
  bool Insert (unsigned long from, unsigned long to);
 private:
  struct Trampoline
  {
    unsigned long from;
    unsigned long to;
  };

  static int IteratePhdr (struct dl_phdr_info *info,
			  size_t size, void *data);
  static unsigned long RoundUp (unsigned long value, unsigned long round);
  void SetupStage0 (unsigned long from, unsigned long to);
  void SetupStage1 (unsigned long from, unsigned long to);
  void AddPatch (unsigned long at, unsigned long length);

  
  // array of to addresses
  unsigned long *m_to;
  // array of from addresses
  unsigned long *m_from;
  int8_t m_nTrampolines;
  struct Patch
  {
    unsigned long at;
    uint8_t *originalData;
    unsigned long originalDataLength;
  };
  std::vector<struct Patch> m_patches;
};

struct TrampolineData
{
  unsigned long to;
  unsigned long from;
  unsigned long temporary;
};


TrampolineManagerImpl::TrampolineManagerImpl ()
  : m_to (0),
    m_from (0),
    m_nTrampolines (0)
{}
TrampolineManagerImpl::~TrampolineManagerImpl ()
{
  for (uint32_t i = 0; i < m_patches.size (); i++)
    {
      struct Patch patch = m_patches[i];
      Copy (patch.at, patch.originalData, patch.originalDataLength);
      delete [] patch.originalData;
    }
  m_patches.clear ();
  delete [] m_to;
  delete [] m_from;
}
void 
TrampolineManagerImpl::AddPatch (unsigned long at, unsigned long length)
{
  struct Patch patch;
  patch.at = at;
  patch.originalData = new uint8_t [length] ();
  uint8_t *buffer = (uint8_t *)at;
  memcpy (patch.originalData, buffer, length);
  patch.originalDataLength = length;
  m_patches.push_back (patch);
}
unsigned long
TrampolineManagerImpl::RoundUp (unsigned long value, unsigned long round)
{
  if ((value % round) == 0)
    {
      return value;
    }
  unsigned long n = value / round;
  return (n + 1) * round;
}
void
TrampolineManagerImpl::SetupStage0 (unsigned long from, unsigned long to)
{
  AddPatch (from, 9);
  signed long delta = to;
  delta -= from + (2+2+5);
  unsigned long udelta = delta;
  unsigned char buffer[9];
  // push trampoline index
  buffer[0] = 0x6a;
  buffer[1] = m_nTrampolines;
  // pop %r11
  buffer[2] = 0x41;
  buffer[3] = 0x5b;
  // jmp delta
  buffer[4] = 0xe9;
  buffer[5] = (udelta >> 0) & 0xff;
  buffer[6] = (udelta >> 8) & 0xff;
  buffer[7] = (udelta >> 16) & 0xff;
  buffer[8] = (udelta >> 24) & 0xff;
  Copy (from, buffer, 2+2+5);
}
void
TrampolineManagerImpl::SetupStage1 (unsigned long from, unsigned long to)
{
  AddPatch (from, 14);
  unsigned char buffer[14];
  // mov to,%rax
  buffer[0] = 0x48;
  buffer[1] = 0xa1;
  buffer[2] = (to >> 0) & 0xff;
  buffer[3] = (to >> 8) & 0xff;
  buffer[4] = (to >> 16) & 0xff;
  buffer[5] = (to >> 24) & 0xff;
  buffer[6] = (to >> 32) & 0xff;
  buffer[7] = (to >> 40) & 0xff;
  buffer[8] = (to >> 48) & 0xff;
  buffer[9] = (to >> 54) & 0xff;
  //jmpq *(%rax,%r11,8)
  buffer[10] = 0x42;
  buffer[11] = 0xff;
  buffer[12] = 0x24;
  buffer[13] = 0xd8;
  Copy (from, buffer, 10+4);
}
int
TrampolineManagerImpl::IteratePhdr (struct dl_phdr_info *info,
				    size_t size, void *data)
{
  struct TrampolineData *trampoline = (struct TrampolineData *) data;
  unsigned long load_base = info->dlpi_addr;
  const ElfW(Phdr) *phdr, *loadro = 0, *loadrw = 0;
  for (int i = 0; i < info->dlpi_phnum; i++)
    {
      phdr = &info->dlpi_phdr[i];
      if (phdr->p_type == PT_LOAD)
	{
	  if (phdr->p_flags & PF_W)
	    {
	      loadrw = phdr;
	    }
	  else
	    {
	      loadro = phdr;
	    }
	}
    }
  NS_ASSERT (loadro != 0 && loadrw != 0);
  unsigned long start, end;
  start = loadro->p_vaddr + load_base;
  end = loadro->p_vaddr + loadro->p_memsz + load_base;
  if (trampoline->from < start || trampoline->from >= end)
    {
      // not this library
      return 0;
    }
  end = RoundUp (end, 16);
  NS_ASSERT (RoundUp (end, loadro->p_align) - end > 8);
  trampoline->temporary = end;
  return 1;
}
bool 
TrampolineManagerImpl::Insert (unsigned long from, unsigned long to)
{
  struct TrampolineData data;
  data.from = from;
  data.to = to;
  int status = dl_iterate_phdr (&TrampolineManagerImpl::IteratePhdr, &data);
  if (status == 0)
    {
      return false;
    }
  SetupStage0 (from, data.temporary);
  SetupStage1 (data.temporary, (unsigned long)&m_to);

  unsigned long *pto = new unsigned long [m_nTrampolines+1] ();
  memcpy (pto, m_to, sizeof(struct Trampoline*)*m_nTrampolines);
  pto[m_nTrampolines] = to;
  delete [] m_to;
  m_to = pto;

  unsigned long *pfrom = new unsigned long [m_nTrampolines+1] ();
  memcpy (pfrom, m_from, sizeof(struct Trampoline*)*m_nTrampolines);
  pfrom[m_nTrampolines] = from;
  delete [] m_from;
  m_from = pfrom;

  NS_ASSERT (m_nTrampolines <= 0x7f);
  m_nTrampolines++;
  return true;
}

} // namespace ns3

#else
#error "Will not work on this architecture: need an implementation of class TrampolineManagerImpl."
#endif

namespace ns3 {

TrampolineManager *
TrampolineManager::Instance (void)
{
  static TrampolineManager instance (new TrampolineManagerImpl ());
  return &instance;
}

TrampolineManager::TrampolineManager (TrampolineManagerImpl *impl)
  : m_impl (impl)
{}

TrampolineManager::~TrampolineManager ()
{
  delete m_impl;
}

bool 
TrampolineManager::Insert (unsigned long from, unsigned long to)
{
  return m_impl->Insert (from, to);
}

} // namespace ns3
