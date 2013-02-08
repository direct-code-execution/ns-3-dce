#include "dce-pwd.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DcePwd");



struct passwd * dce_getpwnam (const char *name)
{
  return 0;
}

struct passwd * dce_getpwuid (uid_t uid)
{
  return 0;
}

void dce_endpwent (void)
{

}
