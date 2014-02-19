# -*- coding: utf-8 -*-
from ns.dce import *
import ns.applications
import ns.core
import ns.internet
import ns.network
import ns.point_to_point

def printInfo(o):
	import os
	print " **************************************"
	print " ** PYTHONPATH: " + str(os.environ['PYTHONPATH'])
	print " **************************************"
	print " ** "+str(o)
	attrs = dir(o)
	print '\n   - '.join("%s" % item for item in attrs )
	print " **************************************"


print "Basic Test Python Bindings"

# ---------------------------------------------------------------
# ns-3 configuration
# ---------------------------------------------------------------
print " - Configure network"
# Logs
#ns.core.LogComponentEnable("Node", ns.core.LOG_LEVEL_INFO)
#ns.core.LogComponentEnable("NodeList", ns.core.LOG_LEVEL_INFO)
#ns.core.LogComponentEnable("Dce", ns.core.LOG_LEVEL_INFO)
#ns.core.LogComponentEnable("DceManager", ns.core.LOG_LEVEL_ALL)
#ns.core.LogComponentEnable("DceApplication", ns.core.LOG_LEVEL_INFO)
#ns.core.LogComponentEnable("DceApplicationHelper", ns.core.LOG_LEVEL_INFO)

# Nodes
nodes = ns.network.NodeContainer()
nodes.Create(2)

# Connection
pointToPoint = ns.point_to_point.PointToPointHelper()
pointToPoint.SetDeviceAttribute("DataRate", ns.core.StringValue("5Mbps"))
pointToPoint.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

devices = pointToPoint.Install(nodes)

stack = ns.internet.InternetStackHelper()
stack.Install(nodes)

address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address("10.1.1.0"), ns.network.Ipv4Mask("255.255.255.0"))

interfaces = address.Assign (devices);

#echoServer = ns.applications.UdpEchoServerHelper(9)

#serverApps = echoServer.Install(nodes.Get(1))
#serverApps.Start(ns.core.Seconds(1.0))
#serverApps.Stop(ns.core.Seconds(10.0))

#echoClient = ns.applications.UdpEchoClientHelper(interfaces.GetAddress(1), 9)
#echoClient.SetAttribute("MaxPackets", ns.core.UintegerValue(1))
#echoClient.SetAttribute("Interval", ns.core.TimeValue(ns.core.Seconds (1.0)))
#echoClient.SetAttribute("PacketSize", ns.core.UintegerValue(1024))

#clientApps = echoClient.Install(nodes.Get(0))
#clientApps.Start(ns.core.Seconds(2.0))
#clientApps.Stop(ns.core.Seconds(10.0))

print "******** Server: "+str(interfaces.GetAddress(0))
print "******** Client: "+str(interfaces.GetAddress(1))

# ---------------------------------------------------------------
# DCE configuration
# ---------------------------------------------------------------
print " - Configure DCE"
dceManager = ns.dce.DceManagerHelper()
dceManager.Install (nodes);
dce = ns.dce.DceApplicationHelper()
#printInfo(dce)

# Server
dce.SetBinary ("udp-perf")
dce.SetStackSize (1<<20)
dce.AddArgument ("--duration=10")
dce.AddArgument ("--nodes=2")
apps = dce.InstallInNode (nodes.Get(0))
apps.Start ( ns.core.Seconds (1.0))
srvAddress = interfaces.GetAddress(0)

# Client
dce.SetBinary ("udp-perf")
dce.ResetArguments()
dce.AddArgument ("--client")
dce.AddArgument ("--nodes=2")
dce.AddArgument ("--host="+str(srvAddress))
#dce.AddArgument ("--bandwidth=5Mbps")
dce.AddArgument ("--duration=10")
apps = dce.InstallInNode (nodes.Get(1))
apps.Start ( ns.core.Seconds (2.0))

print "------------------------------------------------------------------------------>"
print " - Run Simulation"
ns.core.Simulator.Stop (ns.core.Seconds(220.0))
ns.core.Simulator.Run ()
ns.core.Simulator.Destroy ()
print "<------------------------------------------------------------------------------"
print " - Done"
