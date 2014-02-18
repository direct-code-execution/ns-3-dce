#ifndef DCE_MANAGER_HELPER_H
#define DCE_MANAGER_HELPER_H

#include <string>
#include <vector>
#include <stdint.h>

//#include "ns3/object.h"

namespace ns3 {
class Node;
template <class T>
class Ptr;

/**
 * \brief Container of information of a DCE finished process
 *
 */
class ProcStatus
{
public:
	ProcStatus() {};

  ProcStatus (int n, int e, int p, int64_t ns, int64_t ne, long rs, long re, double nd, long rd, std::string cmd);

  /**
   * returns node ID information
   */
  int GetNode (void) const;
  /**
   * returns exit code
   */
  int GetExitCode (void) const;
  /**
   * returns Simulated pid
   */
  int GetPid (void) const;
  /**
   * returns Start Time in Simulated Time the unit is nanoseconds
   */
  int64_t GetSimulatedStartTime (void) const;
  /**
   * returns End Time in Simulated Time the unit is nanoseconds
   */
  int64_t GetSimulatedEndTime (void) const;
  /**
   * returns Real Start Time (time_t)
   */
  long GetRealStartTime (void) const;
  /**
   * returns Real End Time
   */
  long GetRealEndTime (void) const;
  /**
   * returns Simulated duration in seconds
   */
  double GetSimulatedDuration (void) const;
  /**
   * returns real duration in seconds
   */
  long GetRealDuration (void) const;
  /**
   * returns Command Line argv[]
   */
  std::string GetCmdLine (void) const;

};

class Object{};
class AttributeValue{};
class NodeContainer;


class EmptyAttributeValue: AttributeValue{
public:
	EmptyAttributeValue(){};
};


/**
 * \brief configure required instances for DCE-capable nodes
 *
 */
class DceManagerHelper : public Object
{
public:

  /**
   * Construct a DceManagerHelper
   */
  DceManagerHelper ();

  /**
   * \param type the name of the TaskScheduler to set
   * \param n0 the name of the attribute to set to the TaskScheduler
   * \param v0 the value of the attribute to set to the TaskScheduler
   * \param n1 the name of the attribute to set to the TaskScheduler
   * \param v1 the value of the attribute to set to the TaskScheduler
   *
   * Set these attributes on each ns3::TaskScheduler
   */
  void SetScheduler (std::string type,
                     std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                     std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue ());

  /**
   * \param type the name of the ProcessDelayModel to set
   * (ns3::RandomProcessDelayModel and ns3::TimeOfDayProcessDelayModel are available)
   * \param n0 the name of the attribute to set to the ProcessDelayModel
   * \param v0 the value of the attribute to set to the ProcessDelayModel
   * \param n1 the name of the attribute to set to the ProcessDelayModel
   * \param v1 the value of the attribute to set to the ProcessDelayModel
   *
   * Set these attributes on each ns3::ProcessDelayModel
   */
  void SetDelayModel (std::string type,
                      std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                      std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue ());

  /**
   * \param n0 the name of the attribute to set to the ns3::TaskManager
   * \param v0 the value of the attribute to set to the ns3::TaskManager
   *
   * Set these attributes on each ns3::TaskManager
   */
  void SetTaskManagerAttribute (std::string n0, const AttributeValue &v0);

  /**
   * \param type the name of loader set to the ns3::LoaderFactory
   (ns3::CoojaLoaderFactory[] and ns3::DlmLoaderFactory[] are available)
   *
   */
  void SetLoader (std::string type);

  /**
   * \param type the name of the ns3::SocketFdFactory to set
   * (ns3::Ns3SocketFdFactory and ns3::LinuxSocketFdFactory are available)
   * \param n0 the name of the attribute to set to the ns3::SocketFdFactory
   * \param v0 the value of the attribute to set to the ns3::SocketFdFactory
   *
   * Set these attributes on each ns3::SocketFdFactory
   */
  void SetNetworkStack (std::string type,
                        std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue ());

  /**
   * \param n1 the name of the attribute to set to the ns3::DceManager
   * \param v1 the value of the attribute to set to the ns3::DceManager
   *
   * Set these attributes on each ns3::DceManager
   */
  void SetAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \param nodes a set of nodes
   *
   * This method creates all of DCE related instances to run an applicaion
   * binary on nodes.
   */
  void Install (NodeContainer nodes);

  /**
   * \param p the name of path
   *
   * This method configures the path used by simulated methods
   * 'execvp' and 'execlp'
   */
  void SetVirtualPath (std::string p);

  /**
   * This method returns the path used by simulated methods
   * 'execvp' and 'execlp'
   */
  std::string GetVirtualPath () const;

  /**
   *
   * This method returns a Vector of process information
   * that are already finished.
   */
  static std::vector<ProcStatus> GetProcStatus (void);
};

} // namespace ns3

#endif /* DCE_MANAGER_HELPER_H */
