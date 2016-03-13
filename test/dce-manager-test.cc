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
                      bool useNet, std::string stack, bool skip);
private:
  virtual void DoRun (void);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_filename;
  std::string m_stdinFilename;
  Time m_maxDuration;
  std::string m_netstack;
  bool m_useNet;
  bool m_skip;
};

DceManagerTestCase::DceManagerTestCase (std::string filename, Time maxDuration,
                                        std::string stdin, bool useNet, std::string stack,
                                        bool skip)
  : TestCase (std::string ("") + (skip ? "(SKIP) " : "") +
              filename +
              " (" + stack +")"),
    m_filename (filename),
    m_stdinFilename (stdin),
    m_maxDuration (maxDuration),
    m_netstack (stack),
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

  dceManager.SetAttribute ("UnameStringRelease", StringValue ("3"));
  dceManager.SetAttribute ("UnameStringVersion", StringValue ("25"));

  if (m_useNet)
    {
      if (m_netstack == "linux")
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
      else if (m_netstack == "ns3")
        {
          dceManager.Install (nodes);

          InternetStackHelper stack;
          Ipv4DceRoutingHelper ipv4RoutingHelper;
          stack.SetRoutingHelper (ipv4RoutingHelper);
          stack.Install (nodes);
        }
      else if (m_netstack == "freebsd")
        {
          dceManager.SetNetworkStack ("ns3::FreeBSDSocketFdFactory", "Library", StringValue ("libfreebsd.so"));
          dceManager.Install (nodes);

          dce.SetBinary ("freebsd-iproute");
          dce.SetStackSize (1 << 16);
          dce.ResetArguments ();
          dce.ParseArguments ("lo0 127.0.0.1 255.0.0.0");
          apps = dce.Install (nodes.Get (0));
          apps.Start (Seconds (2.0));
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

#define NS3_STACK      (1 << 0)
#define LINUX_STACK    (1 << 1)
#define FREEBSD_STACK  (1 << 2)

DceManagerTestSuite::DceManagerTestSuite ()
  : TestSuite ("dce-process-manager", UNIT)
{
  typedef struct
  {
    const char *name;
    int duration;
    const char *stdinfile;
    bool useNet;
    bool skipUctx;
    uint32_t stackMask;
  } testPair;

  const testPair tests[] = {
    { "test-empty", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-sleep", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-pthread", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-mutex", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-once", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-pthread-key", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-sem", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-malloc", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-malloc-2", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-fd-simple", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-strerror", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-stdio", 0, "/etc/passwd",false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-string", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-netdb", 3600, "", true, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-env", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-cond", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-timer-fd", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-stdlib", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-fork", 0, "", false, true, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-select", 3600, "", true, false, LINUX_STACK},
    {  "test-nanosleep", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-random", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-local-socket", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-poll", 3200, "", true, false, NS3_STACK|LINUX_STACK},
    {  "test-tcp-socket", 320, "", true, false, LINUX_STACK},
    {  "test-exec", 0, "", false, true, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-raw-socket", 320, "", true, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-iperf", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-name", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-pipe", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-dirent", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-socket", 30, "", true, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-bug-multi-select", 30, "", true, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-tsearch", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-clock-gettime", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
    {  "test-gcc-builtin-apply", 0, "", false, false, NS3_STACK|LINUX_STACK|FREEBSD_STACK},
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

  // ns-3 stack
  for (unsigned int i = 0; i < sizeof(tests) / sizeof(testPair); i++)
    {

      AddTestCase (new DceManagerTestCase (tests[i].name,  Seconds (tests[i].duration),
                                           tests[i].stdinfile,
                                           tests[i].useNet,
                                           "ns3",
                                           (tests[i].stackMask & NS3_STACK) ?
                                           (isUctxFiber ? tests[i].skipUctx : false) : true
                                           ),
                   TestCase::QUICK);
    }

  // linux stack
  TypeId tid;
  bool kern_linux = TypeId::LookupByNameFailSafe ("ns3::LinuxSocketFdFactory", &tid);
  std::string filePath = SearchExecFile ("DCE_PATH", "liblinux.so", 0);
  if (kern_linux && (filePath.length () > 0))
    {
      for (unsigned int i = 0; i < sizeof(tests) / sizeof(testPair); i++)
        {
          AddTestCase (new DceManagerTestCase (tests[i].name,  Seconds (tests[i].duration),
                                               tests[i].stdinfile,
                                               tests[i].useNet,
                                               "linux",
                                               (tests[i].stackMask & LINUX_STACK) ?
                                               (isUctxFiber ? tests[i].skipUctx : false) : true
                                               ),
                       TestCase::QUICK);
        }
    }

  // FreeBSD
  bool kern_freebsd = TypeId::LookupByNameFailSafe ("ns3::FreeBSDSocketFdFactory", &tid);
  filePath = SearchExecFile ("DCE_PATH", "libfreebsd.so", 0);
  if (kern_freebsd && (filePath.length () > 0))
    {
      for (unsigned int i = 0; i < sizeof(tests) / sizeof(testPair); i++)
        {
          AddTestCase (new DceManagerTestCase (tests[i].name,  Seconds (tests[i].duration),
                                               tests[i].stdinfile,
                                               tests[i].useNet,
                                               "freebsd",
                                               (tests[i].stackMask & FREEBSD_STACK) ?
                                               (isUctxFiber ? tests[i].skipUctx : false) : true
                                               ),
                       TestCase::QUICK);
        }
    }
}

} // namespace ns3
