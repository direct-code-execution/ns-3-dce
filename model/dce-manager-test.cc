#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "dce-manager.h"
#include "task-manager.h"
#include "rr-task-scheduler.h"
#include "cooja-loader-factory.h"

static std::string g_testError;

extern "C" void dce_manager_test_store_test_error (const char *s)
{
  g_testError = s;
}


namespace ns3 {



class DceManagerTestCase : public TestCase
{
public:
  DceManagerTestCase (std::string filename);
private:
  virtual bool DoRun (void);
  Ptr<DceManager> CreateManager (int *pstatus);
  void StartApplication (Ptr<DceManager> manager, int *pstatus);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_filename;
};

DceManagerTestCase::DceManagerTestCase (std::string filename)
  : TestCase ("Check that process \"" + filename + "\" completes correctly."),
    m_filename (filename)
{}
void
DceManagerTestCase::StartApplication (Ptr<DceManager> manager, int *pstatus)
{
  std::vector<std::string> noargs;
  std::vector<std::pair<std::string,std::string> > noenv;
  
  uint16_t pid = manager->Start (m_filename, 1<<20, noargs, noenv);
  manager->SetFinishedCallback (pid, MakeBoundCallback (&DceManagerTestCase::Finished, pstatus));
}
Ptr<DceManager> 
DceManagerTestCase::CreateManager (int *pstatus)
{
  Ptr<Node> a = CreateObject<Node> ();
  Ptr<TaskManager> taskManager = CreateObject<TaskManager> ();
  Ptr<TaskScheduler> taskScheduler = CreateObject<RrTaskScheduler> ();
  Ptr<DceManager> aManager = CreateObject<DceManager> ();
  Ptr<LoaderFactory> loaderFactory = CreateObject<CoojaLoaderFactory> ();
  taskManager->SetScheduler (taskScheduler);
  a->AggregateObject (loaderFactory);
  a->AggregateObject (taskManager);
  a->AggregateObject (aManager);
  Simulator::ScheduleWithContext (a->GetId (), Seconds (0.0),
				  &DceManagerTestCase::StartApplication, this, 
				  aManager, pstatus);
  return aManager;
}
void
DceManagerTestCase::Finished (int *pstatus, uint16_t pid, int status)
{
  *pstatus = status;
}
bool
DceManagerTestCase::DoRun (void)
{
  int status = - 1;
  Ptr<DceManager> a = CreateManager (&status);
  Simulator::Run ();
  Simulator::Destroy ();
  NS_TEST_ASSERT_MSG_EQ (status, 0, "Process did not return successfully: " << g_testError);
  return status != 0;
}

static class DceManagerTestSuite : public TestSuite
{
public:
  DceManagerTestSuite ();
private:
} g_processTests;

DceManagerTestSuite::DceManagerTestSuite ()
  : TestSuite ("process-manager", UNIT)
{
  const char *tests [] = {
  "test-empty",
  "test-sleep",
  "test-pthread",
  "test-mutex",
  "test-once",
  "test-pthread-key",
  "test-sem",
  "test-malloc",
  "test-malloc-2",
  "test-fd-simple",
  "test-strerror",
  "test-stdio",
  "test-string",
  "test-netdb",
  "test-env",
  "test-cond",
  "test-timer-fd",
  "test-stdlib",
  "test-select",
  "test-nanosleep",
  "test-random",
  };
  for (unsigned int i = 0; i < sizeof(tests)/sizeof(char*);i++)
    {
      AddTestCase (new DceManagerTestCase (tests[i]));
    }
}

} // namespace ns3
