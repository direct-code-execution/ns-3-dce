.. Introduction
.. ************

.. This manual consists of three parts:

.. 1. User Guide: This part is for people who will use DCE to experiment.
.. 2. How it works: This section explains how DCE works.
.. 3. Developper Guide: This section is intended for those who want to improve DCE.

DCE - USER GUIDE
****************

Prerequisites
=============

Before start you must verify these points:

1. be familiar with the tool NS3,
2. Verify OS `compatibility <http://www.nsnam.org/wiki/index.php/Ns-3-dce_portability>`_
3. Owning the sources of the executable(s) you want to run within DCE. Be able to recompile them.


Installation
============

In order to install DCE you must follow the tutorial part `"Building ns-3 and DCE" <getting-started.html#building-ns-3-and-dce>`_. 

.. Shortly you will be able to use Bake in order to install DCE and its dependances.

Installation result
===================

The result of the installation process is the creation of libraries from source of DCE and that of NS-3 and also some tools and sources of an optional Linux kernel if you have also chosen to use the stack of a linux kernel. 
Below you will find the main directories:

::

  ├── build                Target of NS-3 Core and DCE compilation
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
  ├── ns-3-dev             NS-3 CORE Main tree
  │   ├── bindings         NS-3 Core python bindings
  │   ├── build            Target of NS-3 Core compilation
  │   ├── doc              Documentation source
  │   ├── examples         Many exemple of  scenarios using NS-3
  │   ├── ns3              empty directory
  │   ├── scratch          Scratch directory usable to write scripts
  │   ├── src              NS-3 source code
  │   ├── utils            Utilities
  │   └── waf-tools        "Makefile" tool like
  │
  └── readversiondef       Little utility used by DCE to create dynamic libraries version tags. Used only during compilation of DCE.
  

DCE Configuration Environment
=============================

The DCE specifics variables are essentially two PATH like variables: so within them you may put paths separated by ':' character.

**DCE_PATH** is used by DCE to find the executable you want to launch within NS-3 simulated network. This variable is used when you reference the executable using a relative form like 'ping'.

**DCE_ROOT** is similar to DCE_PATH but it is used when you use an absolute form for exemple '/bin/bash'.

**Please pay attention** that executables that you will place in the directories indicated in the previous variables should be recompiled accordingly to the rules defined in the next chapter.

The universe of execution of DCE
================================

File System
-----------

To start a program in the world of NS-3 you must indicate on which node it will be launched. Once launched this program will have access only to the file system corresponding to the node that corresponds to a directory on your machine called file-X where X is the decimal number of the corresponding node. The file-X directories are created by DCE, only when they do not already exist. Also **note** that the contents of this directory is not cleared when starting the script. So you can copy the files required for the operation of your executables in the tree nodes. If possible it is best that you create these files from the script itself in order to simplify maintenance. DCE provides some helpers for creating configuration files necessary to the execution of certain apps like CCNx and Quagga.

Network
-------

Your program running in a NS-3 node views the network defined by the script for this node.

Time
----

Time perceived by your executable is the simulated time of NS-3. Also **note** that DCE supports real time scheduler of NS-3 with the same limitations.


Recompile the target executable(s)
==================================

As explained in Part **How it works** DCE needs to relocate the executable in memory, to do this it is necessary to recompile the exe target.
In order to this you should follow the two following rules:

1. Compile your objects using this gcc flag: **-fPIC** for exemple :   ``gcc -fPIC -c foo.c``
2. Link your executable using this gcc flag: **-pie**  for exemple :   ``gcc -o foo -pie foo.o``
3. Verify the produced executable using readelf utility in order to display the ELF file header and to verify that your exe is of type **DYN** indicating that DCE should be able to relocate and virtualize it under NS-3 virtual world and network. For exemple : ``readelf -h foo|grep Type:`` ==> ``Type: DYN (Shared object file)``
4. Check also that your executable runs as expected outside of NS-3 and DCE.


Install the target executables
==============================

Copy the executable file produced in a specified directory in the variable environment DCE_PATH so that DCE can find it.


Write a Script
==============

Now that you have compiled your executable you can use it within NS-3 script with the help of a set of DCE Helper Class:

+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| HELPER CLASS NAME    | INCLUDE NAME                   |  DESCRIPTION                                                                |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| DceManagerHelper     | ns3/dce-manager-helper.h       | A DceManager is a DCE internal class which manage the execution of the      |
|                      |                                | executable you will declare to run within NS-3; The DceManagerHelper is the |
|                      |                                | tool you will use within your script to parameter and install DceManager on |
|                      |                                | the NS-3 nodes where you plan to run binaries.                              |
|                      |                                |                                                                             |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| DceApplicationHelper | ns3/dce-application-helper.h   | You will use this helper in order to define which application you want to   |
|                      |                                | run within NS-3 by setting the name of the binary its optionals arguments   |
|                      |                                | , its environment variables, and also optionaly if it take its input from   |
|                      |                                | a file instead of stdin.                                                    |
|                      |                                |                                                                             |
|                      |                                | This class can be derived if you need to do more preparation before running |
|                      |                                | your application. Often applications need configuration file to work        |
|                      |                                | properly, for example if you look at the contents of the helper named       |
|                      |                                | CcnClientHelper you will see that his job is to create the key files needed | 
|                      |                                | for the operation of CCNx's applications.                                   |
|                      |                                |                                                                             |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| CcnClientHelper      | ns3/ccn-client-helper.h        | This helper is a subclass of DceApplicationHelper, its jobs is to create    |
|                      |                                | keys files used by ccnx executables in order to run them correctly within   |
|                      |                                | NS3.                                                                        |
|                      |                                |                                                                             |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+
| QuaggaHelper         | ns3/quagga-helper.h            | This helper is a subclass of DceApplicationHelper.                          |
|                      |                                | It will help you to setup Quagga applications.                              |
|                      |                                |                                                                             |
+----------------------+--------------------------------+-----------------------------------------------------------------------------+

**Note** that the table above indicates the name of includes, so you can look at the comments in them, but in reality for DCE use you need to include only the file ``ns3/dce-module.h``.

The directory named ``myscripts`` is a good place to place your scripts. To create a new script you should create a new directory under ``myscripts``, and put your sources and a configuration file for waf build system, this file should be named ``wscript``.  For starters, you may refer to the contents of the directory ``myscripts/ping``.

Compile a Script
================

To compile simply execute the command waf. The result must be under the directory named ``build/bin/myscripts/foo/bar`` where **foo** is your directory and **bar** your executable according to the content of your ``wscript`` file. 


Launch a Script
===============

Simply launch your script like any other program. 


Results
=======

The execution of the apps using DCE generates special files which reflect the execution thereof. On each node DCE creates a directory ``/var/log``, this directory will contain subdirectory whose name is a number. This number is the pid of a process. Each of these directories contains the following files ``cmdline``, ``status``, ``stdout``, ``stderr``. The file ``cmdline`` recalls the name of the executable run followed arguments. The file ``status`` contains an account of the execution and dating of the start; optionally if the execution is completed there is the date of the stop and the return code. The files ``stdout`` and ``stderr`` correspond to the standard output of the process in question.


Debug
=====

It is possible to use gdb to debug a script DCE/NS-3. As explained somewhere in the execution of a script is monoprocess, then you can put breakpoints in both sources of DCE and those of binaries hosted by DCE.

If you use gdb (a CVS or stable version), do not forget to execute the following command prior
to running any DCE-based program:::

  (gdb) handle SIGUSR1 nostop
  Signal        StopPrintPass to programDescription
  SIGUSR1       NoYesYesUser defined signal 1
  (gdb)

An alternate way to do this and avoid having to repeat this command ad-nauseam
involves creating a .gdbinit file in your ns-3-dce directory and putting this
inside:::

  handle SIGUSR1 nostop

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
|                      |                                                                  |1. use ``dce-runner``      |                                                                    |
|                      |                                                                  |2. link using ``ldso`` as  |``dceManager.SetLoader("ns3::DlmLoaderFactory");``                  |
|                      |                                                                  |default interpreter.       |                                                                    |
|                      |                                                                  |                           |                                                                    |
|                      |                                                                  |                           |                                                                    |
+----------------------+------------------------------------------------------------------+---------------------------+--------------------------------------------------------------------+









