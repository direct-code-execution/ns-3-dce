//#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "dce-manager.h"
#include "task-manager.h"
#include "rr-task-scheduler.h"
#include "cooja-loader-factory.h"
#include "ns3/socket-fd-factory.h"
#include "ns3/internet-stack-helper.h"
#include "testdce.h"
static std::string g_testError;

extern "C" void dce_manager_test_store_test_error (const char *s)
{
  g_testError = s;
}

using namespace ns3;
namespace ns3dce {

class DceManagerTestCase : public TestCase
{
public:
  DceManagerTestCase (std::string filename, Time maxDuration, std::string stdinFilename);
private:
  virtual void DoRun (void);
  Ptr<DceManager> CreateManager (int *pstatus);
  void StartApplication (Ptr<DceManager> manager, int *pstatus);
  void CreateAndAggregateObjectFromTypeId (Ptr<Node> node, const std::string typeId);
  void SetupSimpleStack (Ptr<Node> node);
  static void Finished (int *pstatus, uint16_t pid, int status);

  std::string m_filename;
  std::string m_stdinFilename;
  ObjectFactory m_networkStackFactory;
  ObjectFactory m_tcpFactory;
  const Ipv4RoutingHelper *m_routing;
  Time m_maxDuration;
  InternetStackHelper m_internet;

};

DceManagerTestCase::DceManagerTestCase (std::string filename, Time maxDuration, std::string stdin)
  : TestCase ("Check that process \"" + filename + "\" completes correctly."),
    m_filename (filename), m_stdinFilename( stdin), m_maxDuration ( maxDuration )
{

}
void
DceManagerTestCase::StartApplication (Ptr<DceManager> manager, int *pstatus)
{
  std::vector<std::string> noargs;
  std::vector<std::pair<std::string,std::string> > noenv;
  
  uint16_t pid = manager->Start (m_filename, m_stdinFilename, 1<<20, noargs, noenv);
  manager->SetFinishedCallback (pid, MakeBoundCallback (&DceManagerTestCase::Finished, pstatus));
}
void
DceManagerTestCase::CreateAndAggregateObjectFromTypeId (Ptr<Node> node, const std::string typeId)
{
  ObjectFactory factory;
  factory.SetTypeId (typeId);
  Ptr<Object> protocol = factory.Create <Object> ();
  node->AggregateObject (protocol);
}
void
DceManagerTestCase::SetupSimpleStack (Ptr<Node> node)
{
  m_internet.SetIpv6StackInstall(false);
  m_internet.SetTcp("ns3::TcpL4Protocol");
  m_internet.Install (node);

  m_networkStackFactory.SetTypeId ("ns3::Ns3SocketFdFactory");

  Ptr<SocketFdFactory> networkStack = m_networkStackFactory.Create<SocketFdFactory> ();
  NS_ASSERT( 0 != networkStack );

  node->AggregateObject (networkStack);

  m_internet.EnablePcapIpv4All("IPV4_DCE_TEST");

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

  SetupSimpleStack (a);

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
void
DceManagerTestCase::DoRun (void)
{
  int status = - 1;
  Ptr<DceManager> a = CreateManager (&status);

  if (m_maxDuration.IsStrictlyPositive()) {
      Simulator::Stop ( m_maxDuration );
  }
  Simulator::Run ();
  Simulator::Destroy ();
  NS_TEST_ASSERT_MSG_EQ (status, 0, "Process did not return successfully: " << g_testError);
  //  return status != 0;
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
  typedef struct {
    const char *name;
    int duration;
    const char *stdinfile;
  } testPair;

  const testPair tests[] = {
      { "test-empty", 0, "" },
      {  "test-sleep", 0, "" },
      {  "test-pthread", 0, "" },
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
      {  "test-poll", 320, "" },
      {  "test-tcp-socket", 320, "" },
  };
  for (unsigned int i = 0; i < sizeof(tests)/sizeof(testPair);i++)
    {
      AddTestCase (new DceManagerTestCase (tests[i].name ,  Seconds (tests[i].duration) , tests[i].stdinfile) );
    }
}

} // namespace ns3
