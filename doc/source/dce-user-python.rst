.. include:: replace.txt

.. _python_scripts-label:

DCE Python Scripts
******************

Currently DCE includes an experimental support to the `Python <http://www.python.org/>`_ language. 
To enable it, you may need to recompile it with the flags:

::

  --with-pybindgen=HERE_THE_PYBINDGEN_PATH

indicating the path to an existing `Pybindgen <http://packages.python.org/PyBindGen>`_ source tree to use. 
Or in case waf didn't find the interpreter, you can try to use the flags:

::

  --with-python=HERE_THE_PYTHON_PATH

The first thing you may want to do is to import the DCE module.
For example a minimal DCE script in Python could be:

.. code-block:: python
    
    from ns.DCE import *
    print "It works!"


A first example
###############

In this example, DCE executes a program running ten seconds on a single node.

.. code-block:: python

    # DCE import 
    from ns.DCE import *
    # ns-3 imports 
    import ns.applications
    import ns.core
    import ns.network
    
    # Increase the verbosity level
    ns.core.LogComponentEnable("Dce", ns.core.LOG_LEVEL_INFO)
    ns.core.LogComponentEnable("DceManager", ns.core.LOG_LEVEL_ALL)
    ns.core.LogComponentEnable("DceApplication", ns.core.LOG_LEVEL_INFO)
    ns.core.LogComponentEnable("DceApplicationHelper", ns.core.LOG_LEVEL_INFO)
    
    # Node creation
    nodes = ns.network.NodeContainer()
    nodes.Create(1)
    
    # Configure DCE
    dceManager = ns.DCE.DceManagerHelper()
    dceManager.Install (nodes);
    dce = ns.DCE.DceApplicationHelper()
    
    # Set the binary
    dce.SetBinary ("tenseconds")
    dce.SetStackSize (1<<20)
    # dce.Install returns an instance of ns.DCE.ApplicationContainer
    apps = dce.Install (nodes )
    apps.Start ( ns.core.Seconds (4.0))
    
    # Simulation
    ns.core.Simulator.Stop (ns.core.Seconds(20.0))
    ns.core.Simulator.Run ()
    ns.core.Simulator.Destroy ()
    print "Done."
    
