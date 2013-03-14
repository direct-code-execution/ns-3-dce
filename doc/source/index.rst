.. include:: replace.txt
.. NS3 DCE Documentation documentation master file, created by
   sphinx-quickstart on Thu Jun 30 10:55:38 2011.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Direct Code Execution (DCE)
***************************
Overview
========

Direct Code Execution (DCE) is a module for `ns-3 <http://www.nsnam.org/>`_ that provides facilities to execute, within |ns3|, existing implementations of userspace and kernelspace network protocols or applications without source code changes. For example instead of using the pseudo application provided by |ns3| **V4PingHelper** you can use the true **ping**.

Introduction
============


This document consists of the following parts:

0. :doc:`getting-started`: The document describes a quick instruction of DCE.
1. :doc:`dce-user-doc`: The document is for people who will use DCE to experiment.

.. 2. :doc:`dce-dev-doc`: The document is intended for those who want to extend DCE.
.. 3. :doc:`how-it-works`: This section explains how DCE works.

Outlook
=======

- To run an application using DCE, it is **not** necessary to **change** these sources. However you will need to **recompile** it.
- The simulation is executed wholly within a **single process** which greatly facilitates the **debugging**.
- DCE is very **memory-efficient**, thanks to the way it loads the executables similarly to shared libraries.

Supported Features
==================
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
 - network simulation cradle network stack (IPv4 TCP only)
 - Linux network stack (IPv4/IPv6/others)

- per-node configuration/stdin input
- per-node syslog/stdout/stderr files output

Tested Applications
===================
- CCNx
- Quagga
- iperf
- ping/ping6
- ip (iproute2 package)
- Mobilt IPv6 daemon (umip)
- Linux kernel (from 2.6.36 to 3.7 versions)
- http server (thttpd)
- torrent (opentracker)

Tested Environment
==================

Currently, DCE only supports Linux-based operating system.
DCE has been tested on the following distributions:

- Ubuntu 10.04 32bits/64bits
- Ubuntu 10.10 32bits/64bits
- Ubuntu 11.04 32bits/64bits
- Ubuntu 11.10 32bits/64bits
- Ubuntu 12.04 32bits/64bits
- Fedora 14 32bits/64bits
- Fedora 15 32bits/64bits
- Fedora 16 32bits/64bits

but you can try on the others (e.g., CentOS, RHEL).
If you got run on another distribution, please let us know.


Contents
========

.. toctree::
   :maxdepth: 1

   getting-started
   dce-user-doc
   dce-dev-doc
   how-it-works
   dce-subprojects

   dce-about
 
    

