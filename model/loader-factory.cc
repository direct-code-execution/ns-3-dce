#include "loader-factory.h"

namespace ns3 {

Loader::~Loader ()
{
}

void Loader::NotifyStartExecute (void)
{
}
void Loader::NotifyEndExecute (void)
{
}

TypeId
LoaderFactory::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoaderFactory")
    .SetParent<Object> ();
  return tid;
}
LoaderFactory::~LoaderFactory ()
{
}

} // namespace ns3
