#ifndef DLM_LOADER_FACTORY_H
#define DLM_LOADER_FACTORY_H

#include "loader-factory.h"

namespace ns3 {

class DlmLoaderFactory : public LoaderFactory
{
public:
  static TypeId GetTypeId (void);
  DlmLoaderFactory ();
  virtual ~DlmLoaderFactory ();
  virtual Loader * Create (int argc, char **argv, char **envp);
};

} // namespace ns3

#endif /* DLM_LOADER_FACTORY_H */
