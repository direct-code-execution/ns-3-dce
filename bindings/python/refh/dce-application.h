#ifndef DCE_APPLICATION_H
#define DCE_APPLICATION_H

#include <string>
#include <stdint.h>
#include <vector>
#include <unistd.h>

typedef __UID_T_TYPE uid_t;

namespace ns3 {

class Application {};

/**
 * \brief a wrapper around a posix synchronous application.
 *
 * The sole purpose of this class is to allow users to manipulate
 * these posix synchronous applications in their scripts as if they
 * were normal ns-3 applications and to start them at a specific
 * time using the normal ApplicationContainer::Start method.
 *
 * To implement the Stop method, we send a SIGKILL signal to the
 * target application. Note that doing this will make it impossible
 * to deal correctly with the application's memory management. i.e.,
 * once you call Stop on such an application, valgrind will most
 * likely start reporting a lot of lost memory. Crashes might ensue.
 */
class DceApplication : public Application
{
public:
  //static TypeId GetTypeId (void);

  DceApplication ();
  virtual ~DceApplication ();

  void SetBinary (std::string filename);
  void SetStackSize (uint32_t stackSize);
  void SetArguments (std::vector<std::string> args);
  void SetEnvironment (std::vector<std::pair<std::string,std::string> > envs);
  void SetStdinFile (std::string filename);
  //void SetFinishedCallback (Callback<void,uint16_t,int> cb);
  void SetUid (uid_t i);
  void SetEuid (uid_t i);
  void SetGid (uid_t i);
  void SetEgid (uid_t i);

  uint16_t GetPid();
};

} // namespace ns3

#endif /* DCE_APPLICATION_H */
