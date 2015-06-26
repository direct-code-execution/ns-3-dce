#ifndef LOADER_FACTORY_H
#define LOADER_FACTORY_H

#include "ns3/object.h"
#include "ns3/simple-ref-count.h"

namespace ns3 {

class Loader
{
public:
  virtual ~Loader () = 0;
  virtual void NotifyStartExecute (void);
  virtual void NotifyEndExecute (void);
  virtual Loader * Clone (void) = 0;
  virtual void UnloadAll (void) = 0;
  virtual void * Load (std::string filename, int flag, bool failsafe = false) = 0;
  virtual void Unload (void *module) = 0;
  virtual void * Lookup (void *module, std::string symbol) = 0;
};

class LoaderFactory : public Object
{
public:
  static TypeId GetTypeId (void);
  virtual ~LoaderFactory () = 0;
  virtual Loader * Create (int argc, char **argv, char **envp) = 0;
};

} // namespace ns3

#endif /* LOADER_FACTORY_H */
