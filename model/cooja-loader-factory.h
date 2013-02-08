#ifndef COOJA_LOADER_FACTORY_H
#define COOJA_LOADER_FACTORY_H

#include "loader-factory.h"

namespace ns3 {

class CoojaLoaderFactory : public LoaderFactory
{
public:
  static TypeId GetTypeId (void);
  CoojaLoaderFactory ();
  virtual ~CoojaLoaderFactory ();
  virtual Loader * Create (int argc, char **argv, char **envp);
};

} // namespace ns3

#endif /* COOJA_LOADER_FACTORY_H */
