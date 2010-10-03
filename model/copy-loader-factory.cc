#include "copy-loader-factory.h"
#include "elf-cache.h"
#include "elf-dependencies.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("CopyLoaderFactory");

namespace ns3 {

class CopyLoader : public Loader
{
public:
  CopyLoader (uint32_t uid);
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

  virtual ~CopyLoader ();
  virtual Loader *Clone (void) {return 0; /* XXX */}
  virtual void UnloadAll (void);
  virtual void *Load (std::string filename, int flag);
  virtual void Unload (void *module);
  virtual void *Lookup (void *module, std::string symbol);

  struct CopyLoader::Module *SearchModule (uint32_t id);
  struct CopyLoader::Module *LoadModule (std::string filename, int flag);

  std::list<struct Module *> m_modules;
  ElfCache m_cache;
};

void *
CopyLoader::Load (std::string filename, int flag)
{
  struct Module *module = LoadModule (filename, flag);
  
  // acquire ref for client
  module->refcount++;
  return module->handle;
}

struct CopyLoader::Module *
CopyLoader::SearchModule (uint32_t id)
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

struct CopyLoader::Module *
CopyLoader::LoadModule (std::string filename, int flag)
{
  NS_LOG_FUNCTION (this << filename << flag);
  ElfDependencies deps = ElfDependencies (filename);
  struct Module *module = 0;
  for (ElfDependencies::Iterator i = deps.Begin (); i != deps.End (); ++i)
    {
      ElfCache::ElfCachedFile cached = m_cache.Add (i->found);
      module = SearchModule (cached.id);
      if (module == 0)
	{
	  void *handle = dlopen (cached.cachedFilename.c_str (), RTLD_LAZY | RTLD_DEEPBIND | RTLD_LOCAL);
	  module = new Module ();
	  module->cached = cached;
	  module->handle = handle;
	  module->refcount = 0; // will be incremented later in ::Load or as a dep below.
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
CopyLoader::UnloadAll (void)
{
  NS_LOG_FUNCTION (this);
  for (std::list<struct Module *>::const_iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      struct Module *module = *i;
      dlclose (module->handle);
      delete module;
    }
  m_modules.clear ();
}
void 
CopyLoader::Unload (void *handle)
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
	      dlclose (module->handle);
	      delete module;
	    }
	  break;
	}
    }
}
void *
CopyLoader::Lookup (void *module, std::string symbol)
{
  NS_LOG_FUNCTION (this << module << symbol);
  void *p = dlsym (module, symbol.c_str ());
  return p;
}

CopyLoader::CopyLoader (uint32_t uid)
  : m_cache ("elf-cache", uid)
{
  NS_LOG_FUNCTION (this);
}

CopyLoader::~CopyLoader ()
{
  NS_LOG_FUNCTION (this);
  UnloadAll ();
}


NS_OBJECT_ENSURE_REGISTERED(CopyLoaderFactory);

TypeId 
CopyLoaderFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CopyLoaderFactory")
    .SetParent<LoaderFactory> ()
    .AddConstructor<CopyLoaderFactory> ()
    ;
  return tid;
}
CopyLoaderFactory::CopyLoaderFactory ()
{}
CopyLoaderFactory::~CopyLoaderFactory ()
{}
Loader *
CopyLoaderFactory::Create (int argc, char **argv, char **envp)
{
  // note: we ignore argc and co in this implementation.
  uint32_t uid = AllocateUid ();
  CopyLoader *loader = new CopyLoader (uid);
  return loader;
}
uint32_t
CopyLoaderFactory::AllocateUid (void)
{
  static uint32_t uid = 0;
  uid++;
  return uid;
}

} // namespace ns3
