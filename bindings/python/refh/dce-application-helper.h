#ifndef DCE_HELPER_H
#define DCE_HELPER_H

#include <string>
#include <stdint.h>
#include <vector>

namespace ns3 {

typedef __UID_T_TYPE uid_t;

class ApplicationContainer {};
class NodeContainer;
class Node;
template <class T>
class Ptr;

/**
 * \brief Configuration helper for the application executed with DCE.
 *
 */
class DceApplicationHelper
{
public:
  /**
   * Construct a DceApplicationHelper
   */
  DceApplicationHelper ();
  
  virtual ~DceApplicationHelper () {};


  /**
   * \param filename the name of executable file to run
   */
  void SetBinary (std::string filename);

  /**
   * \param stackSize stack size for this application.
   */
  void SetStackSize (uint32_t stackSize);

  /**
   * \param filename specify the filename to use as a stdin
   */
  void SetStdinFile (std::string filename);

  /**
   * \param arg add an argument for the main executable
   */
  void AddArgument (std::string arg);
  void AddArguments (std::string a0, std::string a1);
  void AddArguments (std::string a0, std::string a1, std::string a2);
  void AddArguments (std::string a0, std::string a1, std::string a2, std::string a3);
  void AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
                     std::string a4);
  void AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
                     std::string a4, std::string a5);
  void AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
                     std::string a4, std::string a5, std::string a6);
  void AddArguments (std::string a0, std::string a1, std::string a2, std::string a3,
                     std::string a4, std::string a5, std::string a6, std::string a7);
  /**
   * \param args add a string of arguments for the main executable. can specify with space-separated arguments, e.g., "-d -f test.conf".
   */
  void ParseArguments (std::string args);

  /**
   * Reset arguments for the main binary for this application.
   */
  void ResetArguments (void);

  /**
   * Add environmental variable for this appilcation.
   * \param name the variable name to the environment. e.g., "HOME"
   * \param value the variable value to the environment. e.g., "/home/furbani"
   */
  void AddEnvironment (std::string name, std::string value);

  /**
   * Reset environmental variables for the main binary for this application.
   */
  void ResetEnvironment (void);

  /**
   * Install the configured application into node.
   *
   * \param node The node on which to install.
   */
  virtual ApplicationContainer InstallInNode (Ptr<Node> node);

  /**
   * Install the configured application into node.
   *
   * \param c NodeContainer that run this application.
   */
  virtual ApplicationContainer Install (NodeContainer c);

  /**
   * Register callback function invoked at the end of application.
   *
   * \param cb callback function to be called.
   */
  //void SetFinishedCallback (Callback<void,uint16_t,int> cb);

  /**
   * Configure 'uid' of the process.
   * \param i the value of uid.
   */
  void SetUid (uid_t i);

  /**
   * Configure effective user ID of this process.
   * \param i the value of euid.
   */
  void SetEuid (uid_t i);

  /**
   * Configure group ID of this process.
   * \param i the value of gid.
   */
  void SetGid (uid_t i);

  /**
   * Configure effective group ID of this process.
   * \param i the value of egid.
   */
  void SetEgid (uid_t i);

};

} // namespace ns3

#endif /* DCE_HELPER_H */
