#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/dce-module.h"
#include "ns3/quagga-helper.h"
#include "ns3/point-to-point-grid.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("quagga");

void
procExit (std::string path, uint16_t pid, int retcode)
{
  std::cout << "Process exit path=" << path << " pid=" << pid << " retcode=" << retcode << std::endl;
}

// Parameters
uint32_t nNodes = 2;
uint32_t stopTime = 60;
std::string netStack = "ns3";


int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("nNodes", "Number of Router nodes", nNodes);
  cmd.AddValue ("stopTime", "Time to stop(seconds)", stopTime);
  cmd.AddValue ("netStack", "What network stack", netStack);
  cmd.Parse (argc, argv);

  PointToPointHelper p2p;
  InternetStackHelper stack;

  PointToPointGridHelper grid (nNodes, nNodes, p2p);
  grid.InstallStack (stack);

  Ipv4AddressHelper ipv4helper ("10.1.1.0", "255.255.255.0");
  grid.AssignIpv4Addresses (
			    Ipv4AddressHelper("10.1.0.0", "255.255.255.0"),
			    Ipv4AddressHelper("10.2.0.0", "255.255.255.0")
			    );

  Ptr<DceManagerHelper> dceManager = CreateObject<DceManagerHelper> ();
  dceManager->SetNetworkStack("ns3::Ns3SocketFdFactory");

  QuaggaHelper quagga;
  
  ApplicationContainer apps;

  for (int i=0; i<nNodes; i++)
    for (int j=0; j<nNodes; j++)
      {
	Ptr<Node> node = grid.GetNode (i,j);
	dceManager->Install (node);
	quagga.EnableOspf (node);
	quagga.EnableOspfDebug (node);
	quagga.EnableZebraDebug (node);
	apps.Add (quagga.Install (node));
      }

  apps.Stop (Seconds(150.0));

  Config::Connect ("/NodeList/*/$ns3::DceManager/Exit", MakeCallback (&procExit));

  Simulator::Stop (Seconds(160.0));
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_INFO ("End of experiment");

  return 0;
}
