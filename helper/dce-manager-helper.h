#ifndef DCE_MANAGER_HELPER_H
#define DCE_MANAGER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/attribute.h"
#include "ns3/object-base.h"
#include "ns3/node-container.h"
#include <string>

namespace ns3 {

class DceManagerHelper : public ObjectBase
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  DceManagerHelper ();
  void SetScheduler (std::string type, 
		     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
		     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue ());
  void SetTaskManagerAttribute (std::string n0, const AttributeValue &v0);
  void SetLoader (std::string type);
  void SetNetworkStack (std::string type,
			std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue ());
  void SetAttribute (std::string n1, const AttributeValue &v1);
  void Install (NodeContainer nodes);
private:
  ObjectFactory m_loaderFactory;
  ObjectFactory m_schedulerFactory;
  ObjectFactory m_taskManagerFactory;
  ObjectFactory m_managerFactory;
  ObjectFactory m_networkStackFactory;
};

} // namespace ns3

#endif /* DCE_MANAGER_HELPER_H */
