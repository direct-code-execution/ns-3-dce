#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/dce-module.h"

static std::string g_testError;

extern "C" void dce_manager_test_store_test_error (const char *s)
{
  g_testError = s;
}

using namespace ns3;
namespace ns3 {

class DceManagerTestCase : public TestCase
{
public:
  DceManagerTestCase (std::string filename, Time maxDuration, std::string stdinFilename, bool useNet);
private:
  virtual void DoRun (void);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_filename;
  std::string m_stdinFilename;
  Time m_maxDuration;
  bool m_useKernel;
  bool m_useNet;
};

DceManagerTestCase::DceManagerTestCase (std::string filename, Time maxDuration, std::string stdin, bool useNet)
  : TestCase ("Check that process \"" + filename + "\" completes correctly."),
    m_filename (filename), m_stdinFilename( stdin), m_maxDuration ( maxDuration ), m_useKernel (1), m_useNet (useNet)
{

}
void
DceManagerTestCase::Finished (int *pstatus, uint16_t pid, int status)
{
  *pstatus = status;
}
void
DceManagerTestCase::DoRun (void)
{
  NodeContainer nodes;
  nodes.Create (1);
  DceApplicationHelper dce;
  ApplicationContainer apps;
  DceManagerHelper dceManager;

  if (m_useNet) {
      if (m_useKernel) {
          dceManager.SetNetworkStack("ns3::LinuxSocketFdFactory", "Library", StringValue ("libnet-next-2.6.so"));
          dceManager.Install (nodes);

          dce.SetBinary ("./ip");
          dce.SetStackSize (1<<16);
          dce.ResetArguments();
          dce.ParseArguments("-f inet addr add local 127.0.0.1/8 scope host dev lo");
          apps = dce.Install (nodes.Get (0));

          apps.Start (Seconds (2.0));
          dce.ResetArguments();
          dce.ParseArguments("link set lo up");
          apps = dce.Install (nodes.Get (0));
          apps.Start (Seconds (3.0));
          dce.ResetArguments();
          dce.ParseArguments("route list table all");
          apps = dce.Install (nodes.Get (0));
          apps.Start (Seconds (3.1));

          dce.ResetArguments();
          dce.ParseArguments("addr show dev lo");
          apps = dce.Install (nodes.Get (0));
          apps.Start (Seconds (3.0));

          //dceManager.SetTaskManagerAttribute( "FiberManagerType", StringValue ( "UcontextFiberManager" ) );
      } else
        {
          dceManager.Install (nodes);

          InternetStackHelper stack;
          stack.Install (nodes);
        }
  }
  else
    {
      dceManager.Install (nodes);
    }
  int status = -1;

  dce.SetBinary (m_filename);
  dce.SetStackSize (1<<20);
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.SetStdinFile (m_stdinFilename);
  dce.SetFinishedCallback ( MakeBoundCallback (&DceManagerTestCase::Finished, &status ) );
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (10.0));

  if (m_maxDuration.IsStrictlyPositive()) {
      Simulator::Stop ( m_maxDuration );
  }
  Simulator::Run ();
  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (status, 0, "Process did not return successfully: " << g_testError);
}

static class DceManagerTestSuite : public TestSuite
{
public:
  DceManagerTestSuite ();
private:
} g_processTests;
//


DceManagerTestSuite::DceManagerTestSuite ()
  : TestSuite ("process-manager", UNIT)
{
  typedef struct {
    const char *name;
    int duration;
    const char *stdinfile;
    bool useNet;
  } testPair;

  const testPair tests[] = { /*
      { "test-empty", 0, "" },
      {  "test-sleep", 0, "" },
      {  "test-pthread", 0, "" , false},
      {  "test-mutex", 0, "" },
      {  "test-once", 0, "" },
      {  "test-pthread-key", 0, "" },
      {  "test-sem", 0, "" },
      {  "test-malloc", 0, "" },
      {  "test-malloc-2", 0, "" },
      {  "test-fd-simple", 0, "" },
      {  "test-strerror", 0, "" },
      {  "test-stdio", 0, "/etc/passwd" },
      {  "test-string", 0, "" },
      {  "test-netdb", 0, "" },
      {  "test-env", 0, "" },
      {  "test-cond", 0, "" },
      {  "test-timer-fd", 0, "" },
      {  "test-stdlib", 0, "" },
      {  "test-select", 3600, "" },
      {  "test-nanosleep", 0, "" },
      {  "test-random", 0, "" },
      {  "test-fork", 0, "" },
      {  "test-local-socket", 0, "" },
      {  "test-poll", 320, "" }, */
      {  "test-tcp-socket", 320, "", true },
/*      {  "test-exec", 1000, "" }, */
  };
  for (unsigned int i = 0; i < sizeof(tests)/sizeof(testPair);i++)
    {
      AddTestCase (new DceManagerTestCase (tests[i].name ,  Seconds (tests[i].duration) , tests[i].stdinfile, tests[i].useNet ) );
    }
}

} // namespace ns3
