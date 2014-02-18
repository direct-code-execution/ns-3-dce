# -*- coding: utf-8 -*-

from ns.dce import *
import ns.applications
import ns.core
import ns.internet
import ns.network
import ns.point_to_point

print "Basic Test Python Bindings"

# ---------------------------------------------------------------
# ns-3 configuration
# ---------------------------------------------------------------
print " - Configure network"
# Logs
ns.core.LogComponentEnable("Node", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("NodeList", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("Dce", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("DceManager", ns.core.LOG_LEVEL_ALL)
ns.core.LogComponentEnable("DceApplication", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("DceApplicationHelper", ns.core.LOG_LEVEL_INFO)

# Nodes
nodes = ns.network.NodeContainer()
nodes.Create(1)


# ---------------------------------------------------------------
# DCE configuration
# ---------------------------------------------------------------
print " - Configure DCE"
dceManager = ns.DCE.DceManagerHelper()
dceManager.Install (nodes);
dce = ns.DCE.DceApplicationHelper()
#dce = ns.DCE.DceApplication()
dce.SetBinary ("tenseconds")
dce.SetStackSize (1<<20)
#apps is an instance of ns.DCE.ApplicationContainer

print " **************************************"
print " ** [dce]:"+str(dce)
#print " ** [dce]:"+str( dir(dce) )
attrs = dir(dce)
print '\n   - '.join("%s" % item for item in attrs )
print " **************************************"

apps = dce.Install (nodes )
apps.Start ( ns.core.Seconds (4.0))

# ---------------------------------------------------------------
# Simulation
# ---------------------------------------------------------------
print " - Run Simulation"
print "------------------------------------------------------------------------------>"
ns.core.Simulator.Stop (ns.core.Seconds(220.0))
ns.core.Simulator.Run ()
ns.core.Simulator.Destroy ()
print "<------------------------------------------------------------------------------"
print " - Done"
