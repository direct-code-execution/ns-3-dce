#define _GNU_SOURCE 1
#include "cooja-loader-factory.h"
#include "elf-cache.h"
#include "elf-dependencies.h"
#include "ns3/log.h"
#ifdef DCE_MPI
#include "ns3/mpi-interface.h"
#endif
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
struct SharedModule
{
  void *handle;
  void *template_buffer;
  void *data_buffer;
  void *current_buffer;
  uint32_t buffer_size;
  uint32_t id;
  uint32_t refcount;
  std::list<struct SharedModule *> deps;
};
}

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DceCoojaLoaderFactory");
NS_OBJECT_ENSURE_REGISTERED (CoojaLoaderFactory);

struct SharedModules
{
  SharedModules ();
  ~SharedModules ();
  ElfCache cache;
  std::list<struct SharedModule *> modules;
};

class CoojaLoader : public Loader
{
public:
  CoojaLoader ();
private:
  struct Module
  {
    struct SharedModule *module;
    std::list<struct Module *> deps;
    uint32_t refcount;
    void *buffer;
  };

  virtual ~CoojaLoader ();
  virtual void NotifyStartExecute (void);
  virtual void NotifyEndExecute (void);
  virtual Loader * Clone (void);
  virtual void UnloadAll (void);
  virtual void * Load (std::string filename, int flag, bool failsafe = false);
  virtual void Unload (void *module);
  virtual void * Lookup (void *module, std::string symbol);

  static struct SharedModules * Peek (void);
  struct CoojaLoader::Module * SearchModule (uint32_t id);
  struct SharedModule * SearchSharedModule (uint32_t id);
  struct CoojaLoader::Module * LoadModule (std::string filename, int flag,
                                           bool failsafe = false);
  void UnrefSharedModule (SharedModule *search);

  std::list<struct Module *> m_modules;
};

SharedModules::SharedModules ()
#ifdef DCE_MPI
  : cache ("elf-cache", MpiInterface::GetSystemId ())
#else
  : cache ("elf-cache", 0)
#endif
{
}

SharedModules::~SharedModules ()
{
  for (std::list<struct SharedModule *>::iterator i = modules.begin ();
       i != modules.end (); ++i)
    {
      struct SharedModule *module = *i;
      NS_LOG_DEBUG ("delete shared module " << module);
      free (module->template_buffer);
      dlclose (module->handle);
      delete module;
    }
  modules.clear ();
}

struct SharedModules *
CoojaLoader::Peek (void)
{
  static SharedModules modules;
  return &modules;
}

void
CoojaLoader::NotifyStartExecute (void)
{
  for (std::list<struct Module *>::const_iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      const struct Module *module = *i;
      if (module->buffer == module->module->current_buffer)
        {
          continue;
        }
      if (module->module->current_buffer != 0)
        {
          // save the previous one
          memcpy (module->module->current_buffer,
                  module->module->data_buffer,
                  module->module->buffer_size);
        }
      // restore our own
      memcpy (module->module->data_buffer,
              module->buffer,
              module->module->buffer_size);
      // remember what we did
      module->module->current_buffer = module->buffer;
    }
}
void
CoojaLoader::NotifyEndExecute (void)
{
}

Loader *
CoojaLoader::Clone (void)
{
  CoojaLoader *clone = new CoojaLoader ();
  for (std::list<struct Module *>::const_iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      struct Module *module = *i;
      struct Module *clonedModule = new Module ();
      clonedModule->module = module->module;
      clonedModule->module->refcount++;
      clonedModule->refcount = module->refcount;
      clonedModule->buffer = malloc (module->module->buffer_size);
      memcpy (clonedModule->buffer,
              module->module->data_buffer,
              clonedModule->module->buffer_size);
      // setup deps.
      for (std::list<struct Module *>::iterator j = module->deps.begin ();
           j != module->deps.end (); ++j)
        {
          struct Module *dep = *j;
          struct Module *cloneDep = clone->SearchModule (dep->module->id);
          cloneDep->refcount++;
          clonedModule->deps.push_back (cloneDep);
        }
      NS_LOG_DEBUG ("add " << clonedModule->module->id);
      clone->m_modules.push_back (clonedModule);
    }
  return clone;
}

void *
CoojaLoader::Load (std::string filename, int flag, bool failsafe)
{
  struct Module *module = LoadModule (filename, flag, failsafe);
  if (!module)
    {
      return NULL;
    }

  // acquire ref for client
  module->refcount++;
  return module->module->handle;
}

struct CoojaLoader::Module *
CoojaLoader::SearchModule (uint32_t id)
{
  for (std::list<struct Module *>::iterator i = m_modules.begin (); i != m_modules.end (); ++i)
    {
      struct Module *module = *i;
      if (module->module->id == id)
        {
          // already in, ignore.
          return module;
        }
    }
  return 0;
}

struct SharedModule *
CoojaLoader::SearchSharedModule (uint32_t id)
{
  struct SharedModules *ns = Peek ();
  for (std::list<struct SharedModule *>::iterator i = ns->modules.begin ();
       i != ns->modules.end (); ++i)
    {
      if ((*i)->id == id)
        {
          return *i;
        }
    }
  return 0;
}

#define ROUND_DOWN(addr, align) \
  (((unsigned long)addr) - (((unsigned long)(addr)) % (align)))

struct CoojaLoader::Module *
CoojaLoader::LoadModule (std::string filename, int flag, bool failsafe)
{
  NS_LOG_FUNCTION (this << filename << flag);
  struct SharedModules *modules = Peek ();
  ElfDependencies deps = ElfDependencies (filename, failsafe);
  struct Module *module = 0;
  for (ElfDependencies::Iterator i = deps.Begin (); i != deps.End (); ++i)
    {
      if (i->found == "")
        {
          continue;
        }
      ElfCache::ElfCachedFile cached = modules->cache.Add (i->found);
      struct SharedModule *sharedModule = SearchSharedModule (cached.id);
      if (sharedModule == 0)
        {
          void *handle = dlopen (cached.cachedFilename.c_str (),
                                 RTLD_LAZY | RTLD_DEEPBIND | RTLD_LOCAL);
          NS_ASSERT_MSG (handle != 0, "Could not open " << cached.cachedFilename << " " << dlerror ());
          struct link_map *link_map;
          dlinfo (handle, RTLD_DI_LINKMAP, &link_map);

          sharedModule = new SharedModule ();
          NS_LOG_DEBUG ("create shared module=" << sharedModule <<
                        " file=" << cached.cachedFilename <<
                        " id=" << cached.id);
          sharedModule->refcount = 0;
          sharedModule->id = cached.id;
          sharedModule->handle = handle;
          sharedModule->buffer_size = cached.data_p_memsz;
          sharedModule->template_buffer = malloc (cached.data_p_memsz);
          sharedModule->data_buffer = (void *)(link_map->l_addr + cached.data_p_vaddr);
          memcpy (sharedModule->template_buffer,
                  sharedModule->data_buffer,
                  sharedModule->buffer_size);
          sharedModule->current_buffer = 0;
          for (std::vector<uint32_t>::const_iterator j = cached.deps.begin ();
               j != cached.deps.end (); ++j)
            {
              struct SharedModule *dep = SearchSharedModule (*j);
              dep->refcount++;
              sharedModule->deps.push_back (dep);
            }
          modules->modules.push_back (sharedModule);
        }
      module = SearchModule (sharedModule->id);
      if (module == 0)
        {
          module = new Module ();
          NS_LOG_DEBUG ("Create module for " << sharedModule->handle <<
                        " " << cached.cachedFilename);
          module->module = sharedModule;
          sharedModule->refcount++;
          module->refcount = 0;
          module->buffer = malloc (sharedModule->buffer_size);
          if (sharedModule->current_buffer != 0)
            {
              // save the previous one
              memcpy (module->module->current_buffer,
                      module->module->data_buffer,
                      module->module->buffer_size);
            }
          // make sure we re-initialize the data section with the template
          memcpy (sharedModule->data_buffer,
                  sharedModule->template_buffer,
                  sharedModule->buffer_size);
          // record current buffer to ensure that it is saved later
          sharedModule->current_buffer = module->buffer;
          // setup deps.
          for (std::vector<uint32_t>::const_iterator j = cached.deps.begin ();
               j != cached.deps.end (); ++j)
            {
              struct Module *dep = SearchModule (*j);
              dep->refcount++;
              module->deps.push_back (dep);
            }
          NS_LOG_DEBUG ("add " << module);
          m_modules.push_back (module);
        }
    }
  return module;
}
void
CoojaLoader::UnrefSharedModule (SharedModule *search)
{
  NS_LOG_FUNCTION (this << search << search->refcount);
  struct SharedModules *ns = Peek ();
  for (std::list<struct SharedModule *>::iterator i = ns->modules.begin ();
       i != ns->modules.end (); ++i)
    {
      struct SharedModule *module = *i;
      if (module == search)
        {
          module->refcount--;
          if (module->refcount == 0)
            {
              NS_LOG_DEBUG ("delete shared module " << module);
              for (std::list<struct SharedModule *>::iterator j = module->deps.begin ();
                   j != module->deps.end (); ++j)
                {
                  struct SharedModule *dep = *j;
                  UnrefSharedModule (dep);
                }
              dlclose (module->handle);
              free (module->template_buffer);
              delete module;
              ns->modules.erase (i);
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
      NS_LOG_DEBUG ("Delete module " << module);
      if (module->module->current_buffer == module->buffer)
        {
          module->module->current_buffer = 0;
        }
      UnrefSharedModule (module->module);
      free (module->buffer);
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
      if (module->module->handle == handle)
        {
          module->refcount--;
          if (module->refcount == 0)
            {
              m_modules.erase (i);
              for (std::list<struct Module *>::iterator j = module->deps.begin ();
                   j != module->deps.end (); ++j)
                {
                  struct Module *dep = *j;
                  Unload (dep->module->handle);
                }
              // close only after unloading the deps.
              NS_LOG_DEBUG ("Delete module for " << module->module->handle);
              if (module->module->current_buffer == module->buffer)
                {
                  module->module->current_buffer = 0;
                }
              UnrefSharedModule (module->module);
              free (module->buffer);
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
  if (!p)
    {
      NS_LOG_WARN (dlerror());
    }
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
{
}
CoojaLoaderFactory::~CoojaLoaderFactory ()
{
}
Loader *
CoojaLoaderFactory::Create (int argc, char **argv, char **envp)
{
  CoojaLoader *loader = new CoojaLoader ();
  return loader;
}

} // namespace ns3
