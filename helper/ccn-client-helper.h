#ifndef CCN_HELPER_H
#define CCN_HELPER_H
#include "dce-application-helper.h"
#include <fstream>
#include <vector>


namespace ns3 {

class CcnClientHelper : public DceApplicationHelper
{
public:
  CcnClientHelper ();
  virtual ApplicationContainer Install (NodeContainer c);
  virtual ApplicationContainer InstallInNode (Ptr<Node> node);

  void AddFile (std::string from, std::string to);

  /**
   * Reset environmental variables for the main binary for this application.
   */
  void ResetEnvironment (void);


private:
  std::string GetKeystoreDir (void);
  void CreateKeystore ();
  void CopyFile (std::string from, std::string to);
  std::string GetKeystoreTemplate (void);
  std::vector<std::pair <std::string, std::string> > m_files;

  void CopyRealFileToVirtual (int nodeId, std::string from, std::string to);


};

}
#endif // CCN_HELPER_H
