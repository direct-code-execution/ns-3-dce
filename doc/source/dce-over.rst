DCE - OVERVIEW
**************

Direct Code Execution (DCE) is a module for `NS-3 <http://www.nsnam.org/>`_ that provides facilities to execute, within ns-3, existing implementations of userspace and kernelspace network protocols or applications without source code changes. For example instead of using the pseudo application provided by NS-3 **V4PingHelper** you can use the true **ping**.

Features
========

- To run an application using DCE, it is **not** necessary to **change** these sources. However you will need to **recompile** it.
- The simulation is executed wholly within a **single process** which greatly facilitates the **debugging**.
- DCE is very **memory-efficient**, thanks to the way it loads the executables similarly to shared libraries.

Limitations
===========

- Currently the POSIX API (libc) is not fully supported by DCE. However there are already about **400 methods** supported. As the goal of DCE is to allow to execute network applications, many methods related to the network are supported for example ``socket``, ``connect``,  ``bind``, ``listen``, ``read``, ``write``, ``poll``, ``select``. The next chapter list the applications well tested using DCE. 
- Some methods are not usable with all options of DCE. For more details refer to chapter **Coverage API** that lists all the supported methods.
- The scheduler is not as advanced as that of a kernel, for example if an infinite loop in a hosted application, DCE can not get out, but this should not happen in applications written correctly.


Compatibility list
==================

Below are listed the applications tested with DCE. You may find scripts examples of them under the ``example`` directory. The part tutorial explains how to run 
`examples <getting-started.html#examples>`_

CCNX
----

`CCNx <http://www.ccnx.org/about/>`_ is an open source implementation of **Content Centric Networking**.
All the C written executables are supported and none of the Java ones. The versions tested are those between 0.4.0 and 0.6.1 included.


Ping
----

Ping from the following archive `<http://www.skbuff.net/iputils/iputils-s20101006.tar.bz2>`_ is supported.

Quagga
------

Quagga from the following archive `<http://www.quagga.net/download/quagga-0.99.20.tar.gz>`_ as been tested.
Only ``zebra`` and ``ospfd`` binaries are supported for now. You can see the tutorial for more information.

iPERF
-----

iperf from the following archive  `<http://walami.googlecode.com/files/iperf-2.0.5.tar.gz>`_ as been tested.
It is the exception that proves the rule. That is to say that this particular example requires a change in its code. 
In the source file named ``Thread.c`` at line 412 in the function named ``thread_rest`` you must add a ``sleep(1)`` in order to help DCE to break the infinite loop.

API Coverage
============
Below there is the list of the systems calls supported by DCE, the column named **Type** represents how the system call is implemented ie:

1. **DCE** the method is fully rewritten, 
2. **NATIVE** the real corresponding system call is used.

.. csv-table:: API Coverage
   :header: "System Call Name","Domain","Include file","Type","Remarks"
   :widths: 50,10,10,7,23
   :file: api_cov.csv

