#ifndef DCE_APPLICATION_H
#define DCE_APPLICATION_H

#include "ns3/application.h"
#include "ns3/traced-callback.h"

namespace ns3 {

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
  static TypeId GetTypeId (void);

  DceApplication ();
  virtual ~DceApplication ();

  void SetBinary (std::string filename);
  void SetStackSize (uint32_t stackSize);
  void SetArguments (std::vector<std::string> args);
  void SetEnvironment (std::vector<std::pair<std::string,std::string> > envs);
  void SetStdinFile (std::string filename);
  void SetFinishedCallback (Callback<void,uint16_t,int> cb);
  void SetUid (uid_t i);
  void SetEuid (uid_t i);
  void SetGid (uid_t i);
  void SetEgid (uid_t i);

  uint16_t GetPid();

private:
  // inherited from Application base class.
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  virtual void DoDispose (void);

  std::string m_filename;
  uint32_t m_stackSize;
  std::vector<std::string> m_args;
  std::vector<std::pair<std::string,std::string> > m_envs;
  uint16_t m_pid;
  TracedCallback<uint16_t> m_dceStarted;
  std::string m_stdinFilename;
  Callback<void,uint16_t,int> m_finishedCallback;
  uid_t m_uid;
  uid_t m_euid;
  uid_t m_gid;
  uid_t m_egid;
};

} // namespace ns3

#endif /* DCE_APPLICATION_H */
