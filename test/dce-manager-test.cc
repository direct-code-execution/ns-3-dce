#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/dce-module.h"
#include <sys/stat.h>
#include <sys/types.h>

static std::string g_testError;

extern "C" void dce_manager_test_store_test_error (const char *s)
{
  g_testError = s;
}
extern "C" bool useKernel (void);

using namespace ns3;
namespace ns3 {

class DceManagerTestCase : public TestCase
{
public:
  DceManagerTestCase (std::string filename, Time maxDuration, std::string stdinFilename, bool useNet, bool useK);
private:
  virtual void DoRun (void);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_filename;
  std::string m_stdinFilename;
  Time m_maxDuration;
  bool m_useKernel;
  bool m_useNet;
};

DceManagerTestCase::DceManagerTestCase (std::string filename, Time maxDuration, std::string stdin, bool useNet, bool useK)
  : TestCase ("Check that process \"" + filename + "\" completes correctly."),
    m_filename (filename), m_stdinFilename ( stdin), m_maxDuration ( maxDuration ), m_useKernel (useK), m_useNet (useNet)
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
          dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("libnet-next-2.6.so"));
          dceManager.Install (nodes);

          dce.SetBinary ("./ip");
          dce.SetStackSize (1<<16);
          dce.ResetArguments ();
          dce.ParseArguments ("-f inet addr add local 127.0.0.1/8 scope host dev lo");
          apps = dce.Install (nodes.Get (0));

          apps.Start (Seconds (2.0));
          dce.ResetArguments ();
          dce.ParseArguments ("link set lo up");
          apps = dce.Install (nodes.Get (0));
          apps.Start (Seconds (3.0));
          dce.ResetArguments ();
          dce.ParseArguments ("route list table all");
          apps = dce.Install (nodes.Get (0));
          apps.Start (Seconds (3.1));

          dce.ResetArguments ();
          dce.ParseArguments ("addr show dev lo");
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

  dceManager.SetAttribute ("MinimizeOpenFiles", BooleanValue (1) );

  dce.SetBinary (m_filename);
  dce.SetStackSize (1<<20);
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetStdinFile (m_stdinFilename);
  dce.SetFinishedCallback ( MakeBoundCallback (&DceManagerTestCase::Finished, &status ) );
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (10.0));

  if (m_maxDuration.IsStrictlyPositive ()) {
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

  const testPair tests[] = {
    { "test-empty", 0, "", false},
    {  "test-sleep", 0, "", false },
    {  "test-pthread", 0, "", false},
    {  "test-mutex", 0, "", false },
    {  "test-once", 0, "", false},
    {  "test-pthread-key", 0, "", false},
    {  "test-sem", 0, "", false},
    {  "test-malloc", 0, "", false},
    {  "test-malloc-2", 0, "", false},
    {  "test-fd-simple", 0, "", false},
    {  "test-strerror", 0, "", false},
    {  "test-stdio", 0, "/etc/passwd",false},
    {  "test-string", 0, "", false },
    {  "test-netdb", 3600, "", true },
    {  "test-env", 0, "", false },
    {  "test-cond", 0, "", false},
    {  "test-timer-fd", 0, "", false},
    {  "test-stdlib", 0, "", false},
    {  "test-select", 3600, "", true },
    {  "test-nanosleep", 0, "", false},
    {  "test-random", 0, "", false },
      {  "test-fork", 0, "", false },
    {  "test-local-socket", 0, "", false },
    {  "test-poll", 3200, "", true },
    {  "test-tcp-socket", 320, "", true },
      {  "test-exec", 0, "" , false},
    {  "test-raw-socket", 320, "", true },
  };

  // Prepare directories and files for test-stdio
  mkdir ("files-0", S_IRWXU);
  mkdir ("files-0/tmp", S_IRWXU);
  mkdir ("files-0/etc", S_IRWXU);
  FILE *to = fopen ("files-0/etc/passwd","w");

  for (int i=0; i<1024*10; i++)
    {
      char c = ( i % ( 126 - 32 ) ) + 32;

      fwrite (&c, 1, 1, to);
    }
  fclose (to);
  //

  for (unsigned int i = 0; i < sizeof(tests)/sizeof(testPair); i++)
    {
      AddTestCase (new DceManagerTestCase (tests[i].name,  Seconds (tests[i].duration), tests[i].stdinfile, tests[i].useNet, useKernel () ) );
    }
}

} // namespace ns3
