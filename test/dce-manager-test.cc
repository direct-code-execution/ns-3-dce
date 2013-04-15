#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/dce-module.h"
#include "ns3/ipv4-dce-routing-helper.h"
#include <sys/stat.h>
#include <sys/types.h>
//#include <mcheck.h>

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
  DceManagerTestCase (std::string filename, Time maxDuration, std::string stdinFilename,
                      bool useNet, bool useK, bool skip);
private:
  virtual void DoRun (void);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_filename;
  std::string m_stdinFilename;
  Time m_maxDuration;
  bool m_useKernel;
  bool m_useNet;
  bool m_skip;
};

DceManagerTestCase::DceManagerTestCase (std::string filename, Time maxDuration,
                                        std::string stdin, bool useNet, bool useK,
                                        bool skip)
  : TestCase (std::string ("") + (skip ? "(SKIP) " : "") +
              "Check that process \"" + filename +
              (useK ? " (kernel)" : " (ns3)") +
              "\" completes correctly."),
    m_filename (filename),
    m_stdinFilename (stdin),
    m_maxDuration (maxDuration),
    m_useKernel (useK),
    m_useNet (useNet),
    m_skip (skip)
{
//  mtrace ();
}
void
DceManagerTestCase::Finished (int *pstatus, uint16_t pid, int status)
{
  *pstatus = status;
}
void
DceManagerTestCase::DoRun (void)
{
  if (m_skip)
    {
      return;
    }

  NodeContainer nodes;
  nodes.Create (1);
  DceApplicationHelper dce;
  ApplicationContainer apps;
  DceManagerHelper dceManager;

  if (m_useNet)
    {
      if (m_useKernel)
        {
          dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
          dceManager.Install (nodes);

          dce.SetBinary ("ip");
          dce.SetStackSize (1 << 16);
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

        }
      else
        {
          dceManager.Install (nodes);

          InternetStackHelper stack;
          Ipv4DceRoutingHelper ipv4RoutingHelper;
          stack.SetRoutingHelper (ipv4RoutingHelper);
          stack.Install (nodes);
        }
    }
  else
    {
      dceManager.Install (nodes);
    }
  int status = -1;

  dceManager.SetAttribute ("MinimizeOpenFiles", BooleanValue (1));

  dce.SetBinary (m_filename);
  dce.SetStackSize (1 << 20);
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetStdinFile (m_stdinFilename);
  dce.SetFinishedCallback (MakeBoundCallback (&DceManagerTestCase::Finished, &status));
  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (10.0));

  if (m_maxDuration.IsStrictlyPositive ())
    {
      Simulator::Stop (m_maxDuration);
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
  typedef struct
  {
    const char *name;
    int duration;
    const char *stdinfile;
    bool useNet;
    bool skipUctx;
  } testPair;

  const testPair tests[] = {
    { "test-empty", 0, "", false, false},
    {  "test-sleep", 0, "", false, false},
    {  "test-pthread", 0, "", false, false},
    {  "test-mutex", 0, "", false, false},
    {  "test-once", 0, "", false, false},
    {  "test-pthread-key", 0, "", false, false},
    {  "test-sem", 0, "", false, false},
    {  "test-malloc", 0, "", false, false},
    {  "test-malloc-2", 0, "", false, false},
    {  "test-fd-simple", 0, "", false, false},
    {  "test-strerror", 0, "", false, false},
    {  "test-stdio", 0, "/etc/passwd",false, false},
    {  "test-string", 0, "", false, false},
    {  "test-netdb", 3600, "", true, false},
    {  "test-env", 0, "", false, false},
    {  "test-cond", 0, "", false, false},
    {  "test-timer-fd", 0, "", false, false},
    {  "test-stdlib", 0, "", false, false},
    {  "test-fork", 0, "", false, true},
    {  "test-select", 3600, "", true, false},
    {  "test-nanosleep", 0, "", false, false},
    {  "test-random", 0, "", false, false},
    {  "test-local-socket", 0, "", false, false},
    {  "test-poll", 3200, "", true, false},
    {  "test-tcp-socket", 320, "", true, false},
    {  "test-exec", 0, "", false, true},
    {  "test-raw-socket", 320, "", true, false},
    {  "test-iperf", 0, "", false, false},
    {  "test-name", 0, "", false, false},
    {  "test-pipe", 0, "", false, false},
    {  "test-dirent", 0, "", false, false},
    {  "test-socket", 30, "", true, false},
    {  "test-bug-multi-select", 30, "", true, false},
    {  "test-tsearch", 0, "", false, false},
    {  "test-clock-gettime", 0, "", false, false},
    {  "test-gcc-builtin-apply", 0, "", false, false},
    // XXX: not completely tested      {  "test-signal", 30, "" , false},
  };

  // Prepare directories and files for test-stdio
  mkdir ("files-0", S_IRWXU);
  mkdir ("files-0/tmp", S_IRWXU);
  mkdir ("files-0/etc", S_IRWXU);
  FILE *to = fopen ("files-0/etc/passwd","w");

  for (int i = 0; i < 1024 * 10; i++)
    {
      char c = (i % (126 - 32)) + 32;

      fwrite (&c, 1, 1, to);
    }
  fclose (to);
  //

  char *envVar = getenv ("NS_ATTRIBUTE_DEFAULT");
  bool isUctxFiber = false;
  if (envVar != 0)
    {
      std::string env = std::string (envVar);
      std::string::size_type val = 0;
      val = env.find ("UcontextFiberManager", 0);
      if (val != std::string::npos)
        {
          isUctxFiber = true;
        }
    }

  for (unsigned int i = 0; i < sizeof(tests) / sizeof(testPair); i++)
    {

      AddTestCase (new DceManagerTestCase (tests[i].name,  Seconds (tests[i].duration),
                                           tests[i].stdinfile,
                                           tests[i].useNet,
                                           false,
                                           isUctxFiber ? tests[i].skipUctx : false
                                           ));
    }
  TypeId tid;
  bool kern = TypeId::LookupByNameFailSafe ("ns3::LinuxSocketFdFactory", &tid);
  if (kern)
    {
      for (unsigned int i = 0; i < sizeof(tests) / sizeof(testPair); i++)
        {
          AddTestCase (new DceManagerTestCase (tests[i].name,  Seconds (tests[i].duration),
                                               tests[i].stdinfile,
                                               tests[i].useNet,
                                               true,
                                               isUctxFiber ? tests[i].skipUctx : false
                                               ));
        }
    }
}

} // namespace ns3
