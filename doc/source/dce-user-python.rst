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


How to run Python examples outside Waf
######################################

To run the examples outside Waf, you need to tell the system where it needs
to look for the ns-3 and DCE modules. You may want to set four variables 
in the shell:

  * DCE_ROOT: the installation directory (not the source one)
  * DCE_PATH: the directories containing the DCE binaries
  * LD_LIBRARY_PATH: the shared objects directories
  * PYTHONPATH: the directory with the python bindings
 
For example, to run the script 'tenseconds.py':
 
.. code-block:: sh

    export DCE_ROOT=$HOME/dce/build
    export DCE_PATH=$DCE_ROOT/bin_dce:$DCE_ROOT/build/bin:$DCE_ROOT/sbin:$DCE_ROOT/bin_lib
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DCE_ROOT/lib
    export PYTHONPATH=$PYTHONPATH:$DCE_ROOT/lib/python2.7/site-packages
    python tenseconds.py 


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
    