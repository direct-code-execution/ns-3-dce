.. include:: replace.txt

Introduction
------------

Overview
********

Direct Code Execution (DCE) is a module for `ns-3 <https://www.nsnam.org/>`_ that provides facilities to execute, within |ns3|, existing implementations of userspace and kernelspace network protocols or applications without source code changes. For example instead of using the pseudo application provided by |ns3| **V4PingHelper** you can use the true **ping**.

If you wish to cite DCE in your academic work, please reference `this paper <https://dl.acm.org/doi/abs/10.1145/2535372.2535374>`_.

.. _maintainers:

Maintainers
***********

DCE is maintained by `Hajime Tazaki <https://github.com/thehajime>`_ and `Matthieu Coudron <https://github.com/teto>`_.  

Manual Structure
****************

This document consists of the following parts:

0. :doc:`getting-started`: The document describes a quick instruction of DCE.
1. :doc:`dce-user-doc`: The document is for people who will use DCE to experiment.

.. 2. :doc:`dce-dev-doc`: The document is intended for those who want to extend DCE.
.. 3. :doc:`how-it-works`: This section explains how DCE works.

DCE Outlook
***********

- To run an application using DCE, it is usually **not** necessary to **change** its sources. However you will need to **recompile** them.
- The simulation is executed wholly within a **single process** which greatly facilitates the **debugging**.
- DCE is very **memory-efficient**, thanks to the way it loads the executables similarly to shared libraries.

Supported Features
******************
- Simulation with POSIX socket application (no manual modifications)
 
- C/C++ applications

- Simulation with Linux kernel implemented network protocol

 - IPv4/IPv6
 - TCP/UDP/DCCP
 - running with POSIX socket applications and ns-3 socket applications
 - configuration via sysctl-like interface
 - multiple nodes debugging with single gdb interface
 - memory analysis by single valgrind execution with multiple nodes

- Variance of network stacks

 - ns-3 native stack (IPv4/IPv6, partially)
 - Network simulation cradle network stack (IPv4 TCP only)
 - Linux network stack (IPv4/IPv6/others)

- Per-node configuration/stdin input
- Per-node syslog/stdout/stderr files output

Tested Applications
*******************
- CCNx
- Quagga
- iperf
- ping/ping6
- ip (iproute2 package)
- Mobilt IPv6 daemon (umip)
- Linux kernel (from 2.6.36 to 3.7 versions)
- http server (thttpd)
- torrent (libtorrent from rasterbar + opentracker)

Tested Environment
******************

Currently, DCE only supports Linux-based operating systems.
DCE has been tested on the following distributions:

- Ubuntu 16.04 and equivalents (gcc-5.4, libc-2.23)

but you can try on others (e.g., CentOS, RHEL).  However, unfortunately
only a narrow range of older Linux systems work with DCE (gcc-5-based
systems such as Ubuntu 16.04).

If you find that you can run on another distribution, please let the
:ref:`maintainers` know.  Also, if you would like to work on getting DCE to 
work on newer systems, please let the :ref:`maintainers` know.
