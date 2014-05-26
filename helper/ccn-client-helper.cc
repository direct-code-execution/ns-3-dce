#include "ccn-client-helper.h"
#include "dce-application.h"
#include "ns3/log.h"
#include "utils.h"
#include <fstream>
#include <stdlib.h>
#include <unistd.h>

NS_LOG_COMPONENT_DEFINE ("CcnClientHelper");

namespace ns3 {

CcnClientHelper::CcnClientHelper ()
{
}

ApplicationContainer
CcnClientHelper::Install (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  ApplicationContainer apps;
  for (NodeContainer::Iterator j = c.Begin (); j != c.End (); ++j)
    {
      int nodeId = (*j)->GetId ();
      CreateKeystore ();
      std::stringstream oss;

      oss << "files-" << nodeId << "/root/.ccnx/";
      UtilsEnsureAllDirectoriesExist (oss.str ());
      oss << ".ccnx_keystore";

      CopyFile (GetKeystoreTemplate (), oss.str ());

      oss.str ("");
      oss.clear ();

      oss << "files-" << nodeId;
      UtilsEnsureDirectoryExists (oss.str ());

      oss << "/var/";
      UtilsEnsureDirectoryExists (oss.str ());

      oss << "tmp";
      UtilsEnsureDirectoryExists (oss.str ());

      for (std::vector <std::pair <std::string, std::string> >::iterator i = m_files.begin ();
           i != m_files.end (); ++i)
        {
          CopyRealFileToVirtual (nodeId, (*i).first, (*i).second);
        }
    }
  return DceApplicationHelper::Install (c);
}

ApplicationContainer 
CcnClientHelper::InstallInNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  ApplicationContainer apps;
      int nodeId = node->GetId ();
      CreateKeystore ();
      std::stringstream oss;

      oss << "files-" << nodeId << "/root/.ccnx/";
      UtilsEnsureAllDirectoriesExist (oss.str ());
      oss << ".ccnx_keystore";

      CopyFile (GetKeystoreTemplate (), oss.str ());

      oss.str ("");
      oss.clear ();

      oss << "files-" << nodeId;
      UtilsEnsureDirectoryExists (oss.str ());

      oss << "/var/";
      UtilsEnsureDirectoryExists (oss.str ());

      oss << "tmp";
      UtilsEnsureDirectoryExists (oss.str ());

      for (std::vector <std::pair <std::string, std::string> >::iterator i = m_files.begin ();
           i != m_files.end (); ++i)
        {
          CopyRealFileToVirtual (nodeId, (*i).first, (*i).second);
        }
  return DceApplicationHelper::InstallInNode (node);
}

std::string
CcnClientHelper::GetKeystoreDir (void)
{
  std::stringstream oss;
  oss << "/tmp/.dck" << ::getpid () << "/";
  UtilsEnsureAllDirectoriesExist (oss.str ());
  return oss.str ();
}

std::string
CcnClientHelper::GetKeystoreTemplate (void)
{
  std::stringstream oss;

  oss << GetKeystoreDir () << ".ccnx_keystore";

  return oss.str ();
}

void
CcnClientHelper::CopyFile (std::string from, std::string to)
{
  std::ifstream f1 (from.c_str (), std::fstream::binary);
  std::ofstream f2 (to.c_str (), std::fstream::trunc | std::fstream::binary);
  f2 << f1.rdbuf ();
  f2.close ();
  f1.close ();
}

void
CcnClientHelper::CreateKeystore ()
{
  std::stringstream os;
  struct stat st;

  os << GetKeystoreDir () << ".ccnx_keystore";

  if (0 == stat (os.str ().c_str (), &st))
    {
      return;
    }

  std::stringstream oss;

  oss << GetKeystoreDir () << "openssl.cnf";

  std::ofstream conf;

  conf.open (oss.str ().c_str ());

  conf << "# This is not really relevant because we're not sending cert requests anywhere," << std::endl;
  conf << "# but openssl req can refuse to go on if it has no config file." << std::endl;
  conf << "[ req ]" << std::endl;
  conf << "distinguished_name      = req_distinguished_name" << std::endl;
  conf << "[ req_distinguished_name ]" << std::endl;
  conf << "countryName                     = Country Name (2 letter code)" << std::endl;
  conf << "countryName_default             = AU" << std::endl;
  conf << "countryName_min                 = 2" << std::endl;
  conf << "countryName_max                 = 2" << std::endl;

  conf.close ();

  std::stringstream oss2;

  oss2 << "openssl req -config " << oss.str () <<
  " -newkey rsa:1024 -x509 -keyout " <<  GetKeystoreDir () << "private_key.pem -out "
       << GetKeystoreDir () << "certout.pem -subj /CN=foo -nodes 2>/dev/null";

  int ret = ::system (oss2.str ().c_str ());

  std::stringstream oss3;

  oss3 << "openssl pkcs12 -export -name ccnxuser -out " <<  GetKeystoreDir ()
       << ".ccnx_keystore -in " <<  GetKeystoreDir () << "certout.pem -inkey "
       <<  GetKeystoreDir () <<  "private_key.pem -password pass:'Th1s1sn0t8g00dp8ssw0rd.'";

  //NS_LOG_FUNCTION (oss3.str ());

  ret =  ::system (oss3.str ().c_str ());

  std::stringstream oss4;

  oss.str ("");
  oss.clear ();

  oss << GetKeystoreDir () <<  "certout.pem";
  unlink (oss.str ().c_str ());

  oss.str ("");
  oss.clear ();

  oss << GetKeystoreDir () <<  "openssl.cnf";
  unlink (oss.str ().c_str ());

  oss.str ("");
  oss.clear ();

  oss << GetKeystoreDir () <<  "private_key.pem";
  unlink (oss.str ().c_str ());


}
void
CcnClientHelper::AddFile (std::string from, std::string to)
{
  m_files.push_back (std::make_pair (from, to));
}
void
CcnClientHelper::CopyRealFileToVirtual (int nodeId, std::string from, std::string to)
{
  std::stringstream oss;

  oss << "files-" << nodeId << to;

  std::string vto = oss.str ();

  size_t pos = vto.find_last_of ("/");
  if (pos != std::string::npos)
    {
      std::string dir = vto.substr (0,pos);
      UtilsEnsureAllDirectoriesExist (vto);
    }
  CopyFile (from, vto);
}

/**
 * Reset environmental variables for the main binary for this application.
 */
void 
CcnClientHelper::ResetEnvironment (void)
{
  m_files.clear();
  DceApplicationHelper::ResetEnvironment();
}

}
