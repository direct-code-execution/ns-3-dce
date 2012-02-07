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
  void AddFile (std::string from, std::string to);

private:
  std::string GetKeystoreDir (void);
  void CreateKeystore ();
  void CopyFile (std::string from, std::string to);
  std::string GetKeystoreTemplate (void);
  std::vector< std::pair <std::string, std::string > > m_files;

  void CopyRealFileToVirtual (int nodeId, std::string from, std::string to);

};
}
#endif // CCN_HELPER_H
