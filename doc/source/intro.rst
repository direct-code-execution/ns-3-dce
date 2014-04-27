.. include:: replace.txt

Introduction
------------

Overview
********

Direct Code Execution (DCE) is a module for `ns-3 <http://www.nsnam.org/>`_ that provides facilities to execute, within |ns3|, existing implementations of userspace and kernelspace network protocols or applications without source code changes. For example instead of using the pseudo application provided by |ns3| **V4PingHelper** you can use the true **ping**.

Manual Structure
****************


This document consists of the following parts:

0. :doc:`getting-started`: The document describes a quick instruction of DCE.
1. :doc:`dce-user-doc`: The document is for people who will use DCE to experiment.

.. 2. :doc:`dce-dev-doc`: The document is intended for those who want to extend DCE.
.. 3. :doc:`how-it-works`: This section explains how DCE works.

DCE Outlook
***********

- To run an application using DCE, it is **not** necessary to **change** its sources. However you will need to **recompile** them.
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

Currently, DCE only supports Linux-based operating system.
DCE has been tested on the following distributions:

- Ubuntu 10.04 64bit
- Ubuntu 12.04 32bit/64bit
- Ubuntu 12.10 64bit
- Ubuntu 13.04 64bit
- Ubuntu 13.10 64bit (new)
- Fedora 18 32bit
- CentOS 6.2 64bit

but you can try on the others (e.g., CentOS, RHEL).
If you got run on another distribution, please let us know.

