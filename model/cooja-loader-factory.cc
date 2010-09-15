#define _GNU_SOURCE 1
#include "cooja-loader-factory.h"
#include "elf-cache.h"
#include "elf-dependencies.h"
#include "ns3/log.h"
#include <string.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <list>
#include <errno.h>

namespace {
struct Template
{
  void *buffer;
  uint32_t id;
  uint32_t refcount;
};
}

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CoojaLoaderFactory");
NS_OBJECT_ENSURE_REGISTERED (CoojaLoaderFactory);

struct SharedCoojaNamespace
{
  SharedCoojaNamespace ();
  ~SharedCoojaNamespace ();
  ElfCache cache;
  std::list<struct Template *> templates;
};

class CoojaLoader : public Loader
{
public:
  CoojaLoader ();
private:
  struct Module
  {
    void *handle;
    uint32_t refcount;
    std::list<struct Module *> deps;
    void *dataPrivateStart;
    long load_base;
    ElfCache::ElfCachedFile cached;
  };

  virtual ~CoojaLoader ();
  virtual void NotifyStartExecute (void);
  virtual void NotifyEndExecute (void);
  virtual void UnloadAll (void);
  virtual void *Load (std::string filename, int flag);
  virtual void Unload (void *module);
  virtual void *Lookup (void *module, std::string symbol);

  static struct SharedCoojaNamespace *Peek (void);
  struct CoojaLoader::Module *SearchModule (uint32_t id);
  struct CoojaLoader::Module *LoadModule (std::string filename, int flag);
  struct Template *SearchTemplate (uint32_t id);
  void UnrefTemplate (uint32_t id);

  std::list<struct Module *> m_modules;
};

SharedCoojaNamespace::SharedCoojaNamespace ()
  : cache ("elf-cache", 0)
{}

SharedCoojaNamespace::~SharedCoojaNamespace ()
{
  for (std::list<struct Template *>::iterator i = templates.begin ();
       i != templates.end (); ++i)
    {
      struct Template *tmpl = *i;
      free (tmpl->buffer);
      delete tmpl;
    }
  templates.clear ();
}

struct SharedCoojaNamespace *
CoojaLoader::Peek (void)
{
  static SharedCoojaNamespace ns;
  return &ns;
}

void 
CoojaLoader::NotifyStartExecute (void)
{
  // restore the loader private data sections
  for (std::list<struct Module *>::const_iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      const struct Module *module = *i;
      memcpy ((void*)(module->load_base + module->cached.data_p_vaddr),
	      module->dataPrivateStart,
	      module->cached.data_p_memsz);
    }
}
void 
CoojaLoader::NotifyEndExecute (void)
{
  // save the loader private data sections
  for (std::list<struct Module *>::const_iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      const struct Module *module = *i;
      memcpy (module->dataPrivateStart, 
	      (void *)(module->load_base + module->cached.data_p_vaddr),
	      module->cached.data_p_memsz);
    }
}

void *
CoojaLoader::Load (std::string filename, int flag)
{
  struct Module *module = LoadModule (filename, flag);
  
  // acquire ref for client
  module->refcount++;
  return module->handle;
}

struct CoojaLoader::Module *
CoojaLoader::SearchModule (uint32_t id)
{
  for (std::list<struct Module *>::iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      struct Module *module = *i;
      if (module->cached.id == id)
	{
	  // already in, ignore.
	  return module;
	}
    }
  return 0;
}

struct Template *
CoojaLoader::SearchTemplate (uint32_t id)
{
  struct SharedCoojaNamespace *ns = Peek ();  
  for (std::list<struct Template *>::iterator i = ns->templates.begin ();
       i != ns->templates.end (); ++i)
    {
      if ((*i)->id == id)
	{
	  return *i;
	}
    }
  return 0;
}

#define ROUND_DOWN(addr, align) \
  (((long)addr) - (((long)(addr)) % (align)))

struct CoojaLoader::Module *
CoojaLoader::LoadModule (std::string filename, int flag)
{
  NS_LOG_FUNCTION (this << filename << flag);
  struct SharedCoojaNamespace *ns = Peek ();
  ElfDependencies deps = ElfDependencies (filename);
  struct Module *module = 0;
  for (ElfDependencies::Iterator i = deps.Begin (); i != deps.End (); ++i)
    {
      ElfCache::ElfCachedFile cached = ns->cache.Add (i->found);
      module = SearchModule (cached.id);
      if (module == 0)
	{
	  void *handle = dlopen (cached.cachedFilename.c_str (), RTLD_LAZY | RTLD_DEEPBIND | RTLD_LOCAL);
	  NS_ASSERT_MSG (handle != 0, "Could not open " << cached.cachedFilename << " " << dlerror ());
	  struct link_map *link_map;
	  dlinfo (handle, RTLD_DI_LINKMAP, &link_map);

	  module = new Module ();
	  module->cached = cached;
	  module->handle = handle;
	  module->refcount = 0; // will be incremented later in ::Load or as a dep below.
	  module->dataPrivateStart = malloc (cached.data_p_memsz);
	  struct Template *tmpl = SearchTemplate (cached.id);
	  if (tmpl == 0)
	    {
	      // save the template
	      NS_LOG_DEBUG ("create template " << cached.id);
	      tmpl = new Template ();
	      tmpl->buffer = malloc (cached.data_p_memsz);
	      tmpl->id = cached.id;
	      tmpl->refcount = 1;
	      ns->templates.push_back (tmpl);
	      // The libc loader maps the rw PT_LOAD segment as ro. 
	      // Why ? I don't know but changing its protection here 
	      // is sufficient to make this work.
	      int pagesize = sysconf(_SC_PAGE_SIZE);
	      NS_ASSERT_MSG (pagesize != -1, "Unable to obtain page size " << strerror (errno));
	      int retval = mprotect ((void *)ROUND_DOWN(link_map->l_addr + cached.data_p_vaddr, pagesize), 
				     cached.data_p_memsz, 
				     PROT_READ | PROT_WRITE | PROT_EXEC);
	      NS_ASSERT_MSG (retval == 0, "mprotect failed " << strerror (errno));
	      memcpy (tmpl->buffer, (void *)(link_map->l_addr + cached.data_p_vaddr),
		      cached.data_p_memsz);
	    }
	  else
	    {
	      // restore the current state from the template state
	      NS_LOG_DEBUG ("reuse template " << cached.id);
	      tmpl->refcount++;
	      // now, we can safely copy the data without triggering a segfault.
	      memcpy ((void *)(link_map->l_addr + cached.data_p_vaddr), 
		      tmpl->buffer, cached.data_p_memsz);
	    }
	  module->load_base = link_map->l_addr;
	  for (std::vector<uint32_t>::const_iterator j = cached.deps.begin (); j != cached.deps.end (); ++j)
	    {
	      struct Module *dep = SearchModule (*j);
	      dep->refcount++;
	      module->deps.push_back (dep);
	    }
	  m_modules.push_back (module);
	}
    }
  return module;
}
void
CoojaLoader::UnrefTemplate (uint32_t id)
{
  NS_LOG_FUNCTION (this << id);
  struct SharedCoojaNamespace *ns = Peek ();  
  for (std::list<struct Template *>::iterator i = ns->templates.begin ();
       i != ns->templates.end (); ++i)
    {
      struct Template *tmpl = *i;
      if (tmpl->id == id)
	{
	  tmpl->refcount--;
	  if (tmpl->refcount == 0)
	    {
	      NS_LOG_DEBUG ("delete template " << id);
	      free (tmpl->buffer);
	      delete tmpl;
	      ns->templates.erase (i);
	    }
	  break;
	}
    }
}
void 
CoojaLoader::UnloadAll (void)
{
  NS_LOG_FUNCTION (this);
  for (std::list<struct Module *>::const_iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      struct Module *module = *i;
      dlclose (module->handle);
      UnrefTemplate (module->cached.id);
      delete module;
    }
  m_modules.clear ();
}
void 
CoojaLoader::Unload (void *handle)
{
  NS_LOG_FUNCTION (this << handle);
  for (std::list<struct Module *>::iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      struct Module *module = *i;
      if (module->handle == handle)
	{
	  module->refcount--;
	  if (module->refcount == 0)
	    {
	      m_modules.erase (i);
	      for (std::list<struct Module *>::iterator j = module->deps.begin (); 
		   j != module->deps.end (); ++j)
		{
		  struct Module *dep = *j;
		  Unload (dep->handle);
		}
	      // close only after unloading the deps.
	      UnrefTemplate (module->cached.id);
	      dlclose (module->handle);
	      delete module;
	    }
	  break;
	}
    }
}
void *
CoojaLoader::Lookup (void *module, std::string symbol)
{
  NS_LOG_FUNCTION (this << module << symbol);
  void *p = dlsym (module, symbol.c_str ());
  return p;
}

CoojaLoader::CoojaLoader ()
{
  NS_LOG_FUNCTION (this);
}

CoojaLoader::~CoojaLoader ()
{
  NS_LOG_FUNCTION (this);
  UnloadAll ();
}


TypeId 
CoojaLoaderFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CoojaLoaderFactory")
    .SetParent<LoaderFactory> ()
    .AddConstructor<CoojaLoaderFactory> ()
    ;
  return tid;
}
CoojaLoaderFactory::CoojaLoaderFactory ()
{}
CoojaLoaderFactory::~CoojaLoaderFactory ()
{}
Loader *
CoojaLoaderFactory::Create (int argc, char **argv, char **envp)
{
  CoojaLoader *loader = new CoojaLoader ();
  return loader;
}

} // namespace ns3
