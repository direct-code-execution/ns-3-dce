.. include:: replace.txt

Using your userspace protocol implementation
============================================

As explained in :ref:`how-it-works`, DCE needs to relocate the executable binary in memory, and these binary files need to be built with specific compile/link options.

In order to this you should follow the two following rules:

1. Compile your objects using this gcc flag: **-fPIC** for exemple:   ``gcc -fPIC -c foo.c``

 1. (option) Some application needs to be compile with **-U_FORTIFY_SOURCE** so that the application doesn't use alternative symbols including **__chk** (like memcpy_chk).

2. Link your executable using this gcc flag: **-pie** and **-rdynamic** for exemple:   ``gcc -o foo -pie -rdynamic foo.o``
3. Verify the produced executable using readelf utility in order to display the ELF file header and to verify that your exe is of type **DYN** indicating that DCE should be able to relocate and virtualize it under |ns3| virtual world and network. For exemple: ``readelf -h foo|grep Type:`` ==> ``Type: DYN (Shared object file)``
4. Check also that your executable runs as expected outside of |ns3| and DCE.


Install the target executable
------------------------------

Copy the executable file produced in a specified directory in the variable environment DCE_PATH so that DCE can find it.
DCE_PATH behaves like the variable PATH and can contain several directories such as ``/home/USER/iproute2/ip:/home/USER/iperf3/src:/home/USER/iperf2/src``

Write a |ns3| script
--------------------

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
|                      |                                | run within |ns3| by setting the name of the binary its optionals arguments, |
|                      |                                | its environment variables, and also optionally if it take its input from    |
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

For more detail, please refer `DCE API (doxygen) document <../../html/index.html>`_.

Compile the script
------------------

To compile simply execute the command waf. The result must be under the directory named ``build/bin/myscripts/foo/bar`` where **foo** is your directory and **bar** your executable according to the content of your ``wscript`` file. 


Launch the script
-----------------

Simply launch your script like any other program. 

.. code-block:: console

    $ ./waf --run bar


Results
-------

The execution of the apps using DCE generates special files which reflect the execution thereof. On each node DCE creates a directory ``/var/log``, this directory will contain subdirectory whose name is a number. This number is the pid of a process. Each of these directories contains the following files ``cmdline``, ``status``, ``stdout``, ``stderr``. The file ``cmdline`` recalls the name of the executable run followed arguments. The file ``status`` contains an account of the execution and dating of the start; optionally if the execution is completed there is the date of the stop and the return code. The files ``stdout`` and ``stderr`` correspond to the standard output of the process in question.

.. _dce-udp-simple-example:

Example: DCE Simple UDP (dce-udp-simple)
----------------------------------------
The example uses two POSIX socket-based application in a simulation.
Please take time to look at the source dce-udp-simple.cc:

.. literalinclude:: /../../example/dce-udp-simple.cc
    :language: c++
    :linenos:

You can notice that we create a |ns3| Node with an Internet Stack (please refer to |ns3| `doc <http://www.nsnam.org/documentation/>`_. for more info),
and we can also see 2 new Helpers:

 1. DceManagerHelper which is used to Manage DCE loading system in each node where DCE will be used.
 2. DceApplicationHelper which is used to describe real application to be launched by DCE within |ns3| simulation environment.
 

.. _dce-iperf-example:

Example: DCE with iperf(dce-iperf)
----------------------------------
The example uses iperf traffic generator in a simulation.
The scenario is here:

.. literalinclude:: /../../example/dce-iperf.cc
    :language: c++
    :linenos:

This scenario is simple there is 2 nodes linked by a point 2 point link, the node 0 launch iperf as a client via the command **iperf -c 10.1.1.2 -i 1 --time 10** and the node 1 launch iperf as a server via the command **iperf -s -P 1**.
You can follow this to launch the experiment:




