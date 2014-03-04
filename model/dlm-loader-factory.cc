#include "dlm-loader-factory.h"
#include "ns3/log.h"
#include "ns3/fatal-error.h"
#include <list>
#include <dlfcn.h>

NS_LOG_COMPONENT_DEFINE ("DlmLoaderFactory");

extern "C" {
// The function and structure declarations below are not exactly equal to the
// corresponding declarations in the elf loader, mostly for simplicity to
// avoid dragging in too many dependent declarations.

typedef Lmid_t (*DlLmidNew)(int, char **, char **);
typedef void (*DlLmidDelete)(Lmid_t);
typedef int (*DlLmidAddLibRemap)(Lmid_t lmid, const char *src, const char *dst);
typedef void (*Ns3ReportTestError)(const char *);

}


namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DlmLoaderFactory);

class DlmLoader : public Loader
{
public:
  DlmLoader (int argc, char **argv, char **envp);
  virtual ~DlmLoader ();
  virtual Loader * Clone (void)
  {
    return 0;                             /* XXX */
  }
  virtual void UnloadAll (void);
  virtual void * Load (std::string filename, int flag);
  virtual void Unload (void *module);
  virtual void * Lookup (void *module, std::string symbol);
private:
  Lmid_t m_lmid;
  std::list<void *> m_loaded;
};

DlmLoader::DlmLoader (int argc, char **argv, char **envp)
{
  NS_LOG_FUNCTION (this << argc);
  void *libvdl = dlopen ("libvdl.so", RTLD_LAZY | RTLD_LOCAL);
  DlLmidNew dlLmidNew = (DlLmidNew) dlsym (libvdl, "dl_lmid_new");
  dlclose (libvdl);
  if (dlLmidNew == 0)
    {
      NS_FATAL_ERROR ("Could not find our fancy elf loader");
      // Note: we do not link explicitely against our fancy elf loader
      // because it appears to break our stupid build system
      // so, we have to lookup the necessary magic functions at runtime.
    }

  // create a new context
  m_lmid = dlLmidNew (argc, argv, envp);

}
DlmLoader::~DlmLoader ()
{
  NS_LOG_FUNCTION (this);
  m_loaded.clear ();
  void *libvdl = dlopen ("libvdl.so", RTLD_LAZY | RTLD_LOCAL);
  DlLmidDelete dlLmidDelete = (DlLmidDelete) dlsym (libvdl, "dl_lmid_delete");
  dlclose (libvdl);
  if (dlLmidDelete == 0)
    {
      NS_FATAL_ERROR ("Could not find our fancy elf loader for lmid deletion");
      // Note: we do not link explicitely against our fancy elf loader
      // because it appears to break our stupid build system
      // so, we have to lookup the necessary magic functions at runtime.
      return;
    }
  dlLmidDelete (m_lmid);
}
void
DlmLoader::UnloadAll (void)
{
  NS_LOG_FUNCTION (this);
  for (std::list<void *>::const_iterator i = m_loaded.begin ();
       i != m_loaded.end (); ++i)
    {
      ::dlclose (*i);
    }
}
void *
DlmLoader::Load (std::string filename, int flag)
{
  NS_LOG_FUNCTION (this << filename << flag);
  void *module = dlmopen (m_lmid, filename.c_str (), flag);
  m_loaded.push_back (module);
  return module;
}
void
DlmLoader::Unload (void *module)
{
  NS_LOG_FUNCTION (this << module);
  ::dlclose (module);
  m_loaded.remove (module);
}
void *
DlmLoader::Lookup (void *module, std::string symbol)
{
  NS_LOG_FUNCTION (this << module << symbol);
  void *p = dlsym (module, symbol.c_str ());
  if (!p)
    {
      NS_LOG_WARN (dlerror());
    }
  return p;
}


TypeId
DlmLoaderFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DlmLoaderFactory")
    .SetParent<LoaderFactory> ()
    .AddConstructor<DlmLoaderFactory> ()
  ;
  return tid;
}

DlmLoaderFactory::DlmLoaderFactory ()
{
}
DlmLoaderFactory::~DlmLoaderFactory ()
{
}
Loader *
DlmLoaderFactory::Create (int argc, char **argv, char **envp)
{
  DlmLoader *loader = new DlmLoader (argc, argv, envp);
  return loader;
}


} // namespace ns3
