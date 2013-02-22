.. include:: replace.txt

User's Guide
************
This document is for the people who want to use your application in |ns3| using DCE.

.. - how can I use my own application in ns-3 ?
.. - what should I do if my apps doesn't run on DCE ?
.. -- how can I extend DCE ?


Prerequisites
=============

Before start you must verify these points:

1. be familiar with the tool |ns3|,
2. Owning the sources of the executable(s) you want to run within DCE. Be able to recompile them.


Installation
============

In order to install DCE you must follow the tutorial :ref:`build-dce`.


Installation result
===================

The result of the installation process is the creation of libraries from source of DCE and that of |ns3| and also some tools and sources of an optional Linux kernel if you have also chosen to use the stack of a Linux kernel. 
Below you will find the main directories:

::

  ├── build                Target of |ns3| Core and DCE compilation
  │   ├── bin
  │   ├── include
  │   └── lib
  │
  ├── ns-3-dce             DCE main tree 
  │   ├── build            Target of DCE compilation
  │   │   ├── bin
  │   │   ├── bin_dce
  │   │   ├── lib
  │   ├── doc              Documentation source
  │   ├── example          Some exemple of scenarios using DCE
  │   │   ├── bash
  │   │   └── ccnx
  │   ├── helper           Source code of DCE
  │   ├── model            Source code of DCE
  │   ├── netlink          Source code of DCE
  │   ├── ns3waf          
  │   ├── test             Source code of DCE unit tests
  │   └── utils            Utility scripts for DCE build and use
  │
  ├── ns-3-dev             |ns3| CORE Main tree
  │   ├── bindings         |ns3| Core python bindings
  │   ├── build            Target of |ns3| Core compilation
  │   ├── doc              Documentation source
  │   ├── examples         Many exemple of  scenarios using |ns3|
  │   ├── ns3              empty directory
  │   ├── scratch          Scratch directory usable to write scripts
  │   ├── src              |ns3| source code
  │   ├── utils            Utilities
  │   └── waf-tools        "Makefile" tool like
  │
  └── readversiondef       Little utility used by DCE to create dynamic libraries version tags. Used only during compilation of DCE.
  

DCE Configuration Environment
=============================

The DCE specifics variables are essentially two PATH like variables: so within them you may put paths separated by ':' character.

**DCE_PATH** is used by DCE to find the executable you want to launch within |ns3| simulated network. This variable is used when you reference the executable using a relative form like 'ping'.

**DCE_ROOT** is similar to DCE_PATH but it is used when you use an absolute form for exemple '/bin/bash'.

**Please pay attention** that executables that you will place in the directories indicated in the previous variables should be recompiled accordingly to the rules defined in the next chapter.

(FIXME: to be updated)

The universe of execution of DCE
================================

File System
-----------

To start a program in the world of |ns3| you must indicate on which node it will be launched. Once launched this program will have access only to the file system corresponding to the node that corresponds to a directory on your machine called file-X where X is the decimal number of the corresponding node. The file-X directories are created by DCE, only when they do not already exist. Also **note** that the contents of this directory is not cleared when starting the script. So you can copy the files required for the operation of your executables in the tree nodes. If possible it is best that you create these files from the script itself in order to simplify maintenance. DCE provides some helpers for creating configuration files necessary to the execution of certain apps like CCNx and Quagga.

Network
-------

Your program running in a |ns3| node views the network defined by the script for this node.

Time
----

Time perceived by your executable is the simulated time of |ns3|. Also **note** that DCE supports real time scheduler of |ns3| with the same limitations.


Recompile the target executable(s)
==================================

As explained in :ref:`how-it-works`, DCE needs to relocate the executable binary in memory, and these binary files need to be built with specific compile/link options.

In order to this you should follow the two following rules:

1. Compile your objects using this gcc flag: **-fPIC** for exemple :   ``gcc -fPIC -c foo.c``

 1. (option) Some application needs to be compile with **-U_FORTIFY_SOURCE** so that the application doesn't use alternative symbols including **__chk** (like memcpy_chk).

2. Link your executable using this gcc flag: **-pie**  for exemple :   ``gcc -o foo -pie foo.o``
3. Verify the produced executable using readelf utility in order to display the ELF file header and to verify that your exe is of type **DYN** indicating that DCE should be able to relocate and virtualize it under |ns3| virtual world and network. For exemple : ``readelf -h foo|grep Type:`` ==> ``Type: DYN (Shared object file)``
4. Check also that your executable runs as expected outside of |ns3| and DCE.


Install the target executables
==============================

Copy the executable file produced in a specified directory in the variable environment DCE_PATH so that DCE can find it.
(FIXME: to be updated)

Write a Script
==============

Now that you have compiled your executable you can use it within |ns3| script with the help of a set of DCE Helper Class:

+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| HELPER CLASS NAME    | INCLUDE NAME                   |  DESCRIPTION                                                                |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| DceManagerHelper     | ns3/dce-manager-helper.h       | A DceManager is a DCE internal class which manage the execution of the      |
|                      |                                | executable you will declare to run within |ns3|; The DceManagerHelper is    |
|                      |                                | the tool you will use within your script to parameter and install DceManager|
|                      |                                | on the |ns3| nodes where you plan to run binaries.                          |
|                      |                                |                                                                             |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| DceApplicationHelper | ns3/dce-application-helper.h   | You will use this helper in order to define which application you want to   |
|                      |                                | run within |ns3| by setting the name of the binary its optionals arguments  |
|                      |                                | , its environment variables, and also optionaly if it take its input from   |
|                      |                                | a file instead of stdin.                                                    |
|                      |                                |                                                                             |
|                      |                                | This class can be derived if you need to do more preparation before running |
|                      |                                | your application. Often applications need configuration file to work        |
|                      |                                | properly, for example if you look at the contents of the helper named       |
|                      |                                | CcnClientHelper you will see that his job is to create the key files needed | 
|                      |                                | for the operation of CCNx's applications.                                   |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| LinuxStackHelper     | ns3/linux-stack-helper.h       | This helper is used to configure parameters of Linux kernel when we are     |
|                      |                                | using the advanced mode.                                                    |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| CcnClientHelper      | ns3/ccn-client-helper.h        | This helper is a subclass of DceApplicationHelper, its jobs is to create    |
|                      |                                | keys files used by ccnx executables in order to run them correctly within   |
|                      |                                | NS3.                                                                        |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| QuaggaHelper         | ns3/quagga-helper.h            | This helper is a subclass of DceApplicationHelper.                          |
|                      |                                | It will help you to setup Quagga applications.                              |
|                      |                                |                                                                             |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+

**Note** that the table above indicates the name of includes, so you can look at the comments in them, but in reality for DCE use you need to include only the file ``ns3/dce-module.h``.

The directory named ``myscripts`` is a good place to place your scripts. To create a new script you should create a new directory under ``myscripts``, and put your sources and a configuration file for waf build system, this file should be named ``wscript``.  For starters, you may refer to the contents of the directory ``myscripts/ping``.

For more detail, please refer :ref:`DCE API document<dce-doxygen>`.

Compile a Script
================

To compile simply execute the command waf. The result must be under the directory named ``build/bin/myscripts/foo/bar`` where **foo** is your directory and **bar** your executable according to the content of your ``wscript`` file. 


Launch a Script
===============

Simply launch your script like any other program. 

::

  $ ./waf --run bar
  $


Results
=======

The execution of the apps using DCE generates special files which reflect the execution thereof. On each node DCE creates a directory ``/var/log``, this directory will contain subdirectory whose name is a number. This number is the pid of a process. Each of these directories contains the following files ``cmdline``, ``status``, ``stdout``, ``stderr``. The file ``cmdline`` recalls the name of the executable run followed arguments. The file ``status`` contains an account of the execution and dating of the start; optionally if the execution is completed there is the date of the stop and the return code. The files ``stdout`` and ``stderr`` correspond to the standard output of the process in question.



Tweaking
========

DCE is configurable with NS3 Attributes. Refer to the following table:

+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+
| ATTRIBUTE NAME       | DESCRIPTION                                                      |  VALUES                   | EXAMPLES                                                           |
+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+
|**FiberManagerType**  |The TaskManager is used to switch the execution context between   |**UcontextFiberManager**   |``--ns3::TaskManager::FiberManagerType=UcontextFiberManager``       |
|                      |threads and processes.                                            |the more efficient.        |                                                                    |
|                      |                                                                  |                           |``dceManager.SetTaskManagerAttribute("FiberManagerType",            |
|                      |                                                                  |                           |StringValue("UcontextFiberManager"));``                             |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |**PthreadFiberManager**    |``--ns3::TaskManager::FiberManagerType=PthreadFiberManager``        |
|                      |                                                                  |helpful with **gdb** to see|                                                                    |
|                      |                                                                  |the threads. This is the de|                                                                    |
|                      |                                                                  |fault.                     |                                                                    |
+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+
|**LoaderFactory**     |The LoaderFactory is used to load the hosted binaries.            |**CoojaLoaderFactory** is  |``--ns3::DceManagerHelper::LoaderFactory=ns3::CoojaLoaderFactory[]``|
|                      |                                                                  |the default and the only   |                                                                    |
|                      |                                                                  |one that supports ``fork``.|``$ dce-runner my-dce-ns3-script``                                  |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |                           |OR                                                                  |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |**DlmLoaderFactory** is the|``gcc -o  my-dce-ns3-script Wl,--dynamic-linker=PATH2LDSO/ldso ...``|
|                      |                                                                  |more efficient. To use it  |                                                                    |
|                      |                                                                  |you have two ways:         |``$ my-dce-ns3-script --ns3::DceManagerHelper::LoaderFactory=ns3::Dl|
|                      |                                                                  |                           |mLoaderFactory[]``                                                  |
|                      |                                                                  |- use ``dce-runner``       |                                                                    |
|                      |                                                                  |- link using ``ldso`` as   |``dceManager.SetLoader("ns3::DlmLoaderFactory");``                  |
|                      |                                                                  |default interpreter.       |                                                                    |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |                           |                                                                    |
+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+





Use Cases
=========
This section shows examples of DCE applicaitons.


.. _dce-udp-simple-example:

Example: DCE Simple UDP (dce-udp-simple)
----------------------------------------
The example uses two POSIX socket-based application in a simulation.
Please take time to look at the source dce-udp-simple.cc:

::

  int main (int argc, char *argv[])
  {
    CommandLine cmd;
    cmd.Parse (argc, argv);
  
    NodeContainer nodes;
    nodes.Create (1);
  
    InternetStackHelper stack;
    stack.Install (nodes);
  
    DceManagerHelper dceManager;
    dceManager.Install (nodes);
  
    DceApplicationHelper dce;
    ApplicationContainer apps;
  
    dce.SetStackSize (1<<20);
  
    dce.SetBinary ("udp-server");
    dce.ResetArguments();
    apps = dce.Install (nodes.Get (0));
    apps.Start (Seconds (4.0));
  
    dce.SetBinary ("udp-client");
    dce.ResetArguments();
    dce.AddArgument ("127.0.0.1");
    apps = dce.Install (nodes.Get (0));
    apps.Start (Seconds (4.5));
  
    Simulator::Stop (Seconds(1000100.0));
    Simulator::Run ();
    Simulator::Destroy ();
  
    return 0;
  }
  
You can notice that we create a |ns3| Node with an Internet Stack (please refer to |ns3| `doc <http://www.nsnam.org/documentation/>`_. for more info),
and we can also see 2 new Helpers:

 1. DceManagerHelper which is used to Manage DCE loading system in each node where DCE will be used.
 2. DceApplicationHelper which is used to describe real application to be lauched by DCE within |ns3| simulation environnement.
 

.. _dce-iperf-example:

Example: DCE with iperf(dce-iperf)
----------------------------------
The example uses iperf traffic generator in a simulation.
The scenario is here:

::

  $ cat source/dce/example/dce-iperf.cc
  #include "ns3/network-module.h"
  #include "ns3/core-module.h"
  #include "ns3/internet-module.h"
  #include "ns3/dce-module.h"
  #include "ns3/point-to-point-module.h"
  #include "ns3/applications-module.h"
  #include "ns3/netanim-module.h"
  #include "ns3/constant-position-mobility-model.h"
  #include "ccnx/misc-tools.h"
  
  using namespace ns3;
  // ===========================================================================
  //
  //         node 0                 node 1
  //   +----------------+    +----------------+
  //   |                |    |                |
  //   +----------------+    +----------------+
  //   |    10.1.1.1    |    |    10.1.1.2    |
  //   +----------------+    +----------------+
  //   | point-to-point |    | point-to-point |
  //   +----------------+    +----------------+
  //           |                     |
  //           +---------------------+
  //                5 Mbps, 2 ms
  //
  // 2 nodes : iperf client en iperf server ....
  //
  // Note : Tested with iperf 2.0.5, you need to modify iperf source in order to
  //        allow DCE to have a chance to end an endless loop in iperf as follow:
  //        in source named Thread.c at line 412 in method named thread_rest
  //        you must add a sleep (1); to break the infinite loop....
  // ===========================================================================
  int main (int argc, char *argv[])
  {
    bool useKernel = 0;
    bool useUdp = 0;
    std::string bandWidth = "1m";
    CommandLine cmd;
    cmd.AddValue ("kernel", "Use kernel linux IP stack.", useKernel);
    cmd.AddValue ("udp", "Use UDP. Default false (0)", useUdp);
    cmd.AddValue ("bw", "BandWidth. Default 1m.", bandWidth);
    cmd.Parse (argc, argv);
  
    NodeContainer nodes;
    nodes.Create (2);
  
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
  
    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);
  
    DceManagerHelper dceManager;
    dceManager.SetTaskManagerAttribute( "FiberManagerType", StringValue ( "UcontextFiberManager" ) );
  
    if (!useKernel)
      {
        InternetStackHelper stack;
        stack.Install (nodes);
      }
    else
      {
        dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
        LinuxStackHelper stack;
        stack.Install (nodes);
      }
  
    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces = address.Assign (devices);
  
    // setup ip routes
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
    dceManager.Install (nodes);
  
    DceApplicationHelper dce;
    ApplicationContainer apps;
  
    dce.SetStackSize (1<<20);
  
    // Launch iperf client on node 0
    dce.SetBinary ("iperf");
    dce.ResetArguments();
    dce.ResetEnvironment();
    dce.AddArgument ("-c");
    dce.AddArgument ("10.1.1.2");
    dce.AddArgument ("-i");
    dce.AddArgument ("1");
    dce.AddArgument ("--time");
    dce.AddArgument ("10");
    if (useUdp)
      {
        dce.AddArgument ("-u");
        dce.AddArgument ("-b");
        dce.AddArgument (bandWidth);
      }
  
    apps = dce.Install (nodes.Get (0));
    apps.Start (Seconds (0.7));
    apps.Stop (Seconds (20));
  
    // Launch iperf server on node 1
    dce.SetBinary ("iperf");
    dce.ResetArguments();
    dce.ResetEnvironment();
    dce.AddArgument ("-s");
    dce.AddArgument ("-P");
    dce.AddArgument ("1");
    if (useUdp)
      {
        dce.AddArgument ("-u");
      }
  
    apps = dce.Install (nodes.Get (1));
  
    pointToPoint.EnablePcapAll (useKernel?"iperf-kernel":"iperf-ns3", false);
  
    apps.Start (Seconds (0.6));
  
    setPos (nodes.Get (0), 1, 10, 0);
    setPos (nodes.Get (1), 50,10, 0);
  
    Simulator::Stop (Seconds(40.0));
    Simulator::Run ();
    Simulator::Destroy ();
  
    return 0;
  }
    
This scenario is simple there is 2 nodes linked by a point 2 point link, the node 0 launch iperf as a client via the command **iperf -c 10.1.1.2 -i 1 --time 10** and the node 1 launch iperf as a server via the command **iperf -s -P 1**.
You can follow this to launch the experiement:


.. toctree::
   :maxdepth: 1

   dce-debug
   dce-tech
