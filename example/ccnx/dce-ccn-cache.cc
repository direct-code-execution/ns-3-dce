#include <fstream>
#include <vector>
#include <math.h>
#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/log.h"
#include "misc-tools.h"
#include "ns3/ccn-client-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DceCcnCache");

std::vector<int> GetNodes;
int middleNode = 0;
bool onlySomeNodes = true;
int ccnxVersion = 4;

std::string GetAddress (int level, int index)
{
  std::ostringstream oss;

  oss << "10." << (level + 1) << ".1." << (index + 1);

  return oss.str ();
}

void SetPosition (Ptr<Node> n, int radius, int angle, int level)
{
  float x = level * radius;
  float y = 0;

  if (angle != 0)
    {
      x += (float) radius * cos ( (M_PI * (float) angle) / 180.0);
      y = (float) radius * sin ( (M_PI * (float) angle) / 180.0);
    }
  setPos (n, x, y, 0);

  CcnClientHelper dce;

  dce.SetStackSize (1 << 20);

  // Launch ccn daemon on node 0
  dce.SetBinary ("ccnd");
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  // dce.AddEnvironment ("CCND_DEBUG", "-1"); // FULL TRACES
  dce.AddEnvironment ("CCND_DEBUG", "31");
  dce.AddEnvironment ("CCN_LOCAL_PORT", "9695");
  dce.AddEnvironment ("CCND_AUTOREG", "");
  dce.AddEnvironment ("CCND_LISTEN_ON", "");
  dce.AddEnvironment ("CCND_MTU", "");
  dce.AddEnvironment ("CCND_LOCAL_SOCKNAME", "");
  dce.AddEnvironment ("CCND_DATA_PAUSE_MICROSEC", "");
  dce.AddEnvironment ("CCND_KEYSTORE_DIRECTORY", "");

  ApplicationContainer daemon = dce.Install (n);
  daemon.Start (Seconds (0)) ;

  // Stop ccnd before simu end.
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.SetBinary ("ccndsmoketest");
  dce.SetStdinFile ("");
  dce.AddArgument ("kill");
  ApplicationContainer apps = dce.Install (n);
  apps.Start (Seconds (299.0));

}

void LinkNodes (Ptr<Node> left, int leftNum,  Ptr<Node> right, int level, int idx, bool doGet)
{
  static int r = 1;
  bool useTcp = 0;
  NS_LOG_FUNCTION (idx);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (left, right);

  Ipv4AddressHelper address;
  std::ostringstream oss;

  oss << "10." << (r++) << ".1.0";

  std::string network = oss.str ();

  std::string base = "0.0.0.1";

  address.SetBase (network.c_str (), "255.255.255.252", base.c_str () );

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  NS_LOG_FUNCTION (interfaces.GetAddress (1) << " <-- " << interfaces.GetAddress (0));

  CcnClientHelper dce;
  ApplicationContainer config;

  dce.SetStackSize (1 << 20);

  dce.SetBinary ("ccndc");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddEnvironment ("HOME", "/root");
  dce.AddArgument ("-v");
  dce.AddArgument ("add");
  dce.AddArgument ("/");
  dce.AddArgument (useTcp ? "tcp" : "udp");
  dce.AddArgument (Ipv4AddressToString (interfaces.GetAddress (1)) );

  config = dce.Install (left);
  config.Start (Seconds (0.1));

  if (doGet)
    {
      GetNodes.push_back (leftNum);
    }
}

void LinkCenterNodes (NodeContainer nodes, int leftIdx, int rightIdx, int leftLevel, int radius)
{
  NS_LOG_FUNCTION (leftIdx << rightIdx << leftLevel);
  Ptr<Node> leftNode = nodes.Get (leftIdx);
  Ptr<Node> rightNode = nodes.Get (rightIdx);

  if (1 == leftLevel)
    {
      SetPosition (rightNode, radius, 0, leftLevel);
    }
  SetPosition (leftNode, radius, 0, leftLevel + 1);

  LinkNodes (leftNode, leftIdx, rightNode, leftLevel, 1, false);


}

void LinkCircle (NodeContainer nodes,
                 int level, int centerIdx, int power, int radius, int i)
{
  int angle = 90 - (i * 90 / power);
  NS_LOG_FUNCTION (level << centerIdx << power << i << angle) ;

  NS_LOG_FUNCTION (   (centerIdx + 1 +  i) << (centerIdx + 1 +  i + 1) );

  SetPosition (nodes.Get (centerIdx + 1 +  i), radius,  angle, level);
  SetPosition (nodes.Get (centerIdx + 1 +  i + 1), radius, -angle, level);

  LinkNodes (nodes.Get (centerIdx + 1 +  i), centerIdx + 1 +  i,
             nodes.Get (centerIdx), level, (i * 4) + 3, true);

  LinkNodes (nodes.Get (centerIdx + 1 +  i + 1), centerIdx + 1 +  i + 1,
             nodes.Get (centerIdx), level, (i * 4) + 5, true);

}

int less = 1;
int other = 49;
int more = 50;

void InstallGetters (NodeContainer nodes)
{
  int getNumber = 0;
  CcnClientHelper dce;

  for (int turn = 0; turn < 2; turn++)
    {
      for (uint32_t n = 0; n < GetNodes.size () ; n++)
        {
          int nodeNum = GetNodes [n];

          if (((onlySomeNodes &&  (nodeNum == 13)) || (nodeNum == 39))||(!onlySomeNodes))
            {

              getNumber++;

              dce.SetStackSize (1 << 20);
              dce.ResetArguments ();
              dce.ResetEnvironment ();
              dce.AddEnvironment ("HOME", "/root");
              dce.SetBinary ((ccnxVersion == 4) ? "ccnget" : "ccnpeek");
              dce.SetStdinFile ("");
              dce.AddArgument ("-c");
              dce.AddArgument ("/A");

              ApplicationContainer get = dce.Install (nodes.Get (nodeNum));

              float startTime = 0.2;

              if (turn > 0)
                {
                  startTime = 0.4;
                }
              else
                {
                  if (nodeNum >= middleNode)
                    {
                      startTime = 0.3;
                    }
                }

              get.Start (Seconds (startTime));
            }
        }
    }
}

void
CreateReadme ()
{
  std::ofstream osf ("/tmp/README", std::fstream::trunc);

  osf << "The wanted data is here :)" ;

  osf.close ();
}

// ===========================================================================
//
//
// ===========================================================================
int
main (int argc, char *argv[])
{
  std::string animFile = "CcnCache.xml";
  int width = 6;
  int radius = 2000;
  bool nn = false;

  CommandLine cmd;
  cmd.AddValue ("width", "Topology width min 2, default 2.", width);
  cmd.AddValue ("radius", "Radius default 2000.", radius);
  cmd.AddValue ("NN", "Toggle, if true return the number of nodes for this parameters.", nn);
  cmd.AddValue ("some", "Toggle, if true only some nodes do ccnget ", onlySomeNodes);
  cmd.AddValue ("cv", "Ccnx version 4 for 0.4.x variantes and 5 for 0.5.x variantes, default: 4",
                ccnxVersion);
  cmd.Parse (argc, argv);

  NS_ASSERT_MSG (radius >= 2, "Radius must be greater or equal to 2.");
  NS_ASSERT_MSG ( (4 == ccnxVersion) || (5 == ccnxVersion),
                  "Ccnx version must be equal to 4 or 5");

  int NodeNumber = 0;
  int power = 0;
  for (int level = 0; level < width ; level++)
    {
      NodeNumber++;
      if (level == 1)
        {
          power = 2;
        }
      else
        {
          power = power * 2;
        }
      NodeNumber += power;
    }
  if (nn)
    {
      printf ("%d\n",NodeNumber);
      exit (0);
    }
  NodeContainer nodes;
  nodes.Create (NodeNumber);

  InternetStackHelper stack;
  stack.Install (nodes);

  DceManagerHelper dceManager;
  dceManager.Install (nodes);
//  dceManager.SetAttribute ("MinimizeOpenFiles", BooleanValue (1) );

  int NodeCursor = 0;
  int PreviousCenter = -1;

  power = 0;
  // Create Topology .
  for (int l = 0; l < width; l++)
    {
      int Center = NodeCursor++;

      if ( (l - 1) == (width / 2))
        {
          middleNode = NodeCursor;
        }

      if (l > 0)
        {
          // Link Center With Previous Center
          LinkCenterNodes (nodes, Center,  PreviousCenter, l, radius);
        }
      if (l == 1)
        {
          power = 2;
        }
      else
        {
          power = power * 2;
        }

      for (int n = 0; n < power ; n += 2)
        {
          LinkCircle (nodes, l + 1, Center, power, radius, n);
        }
      NodeCursor += power;

      PreviousCenter = Center;

    }
  //DceApplicationHelper dce;
  CreateReadme ();

  CcnClientHelper dce;

  dce.SetStackSize (1 << 20);
  dce.ResetArguments ();
  dce.ResetEnvironment ();

  dce.SetBinary ((ccnxVersion == 4) ? "ccnput" : "ccnpoke");
  dce.SetStdinFile ("/tmp/README");
  dce.AddFile ("/tmp/README", "/tmp/README");

  dce.AddArgument ("-x");
  dce.AddArgument ("300");
  dce.AddArgument ("/A");
  dce.AddEnvironment ("HOME", "/root");

  ApplicationContainer put = dce.Install (nodes.Get (0));
  put.Start (Seconds (0.15));

  InstallGetters (nodes);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Create the animation object and configure for specified output
  AnimationInterface anim (animFile);

  Simulator::Stop (Seconds (300.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

