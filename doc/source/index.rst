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


This document consists of three parts:

0. :doc:`getting-started`: The document describes a quick instruction of DCE.
1. :doc:`dce-user-doc`: The document is for people who will use DCE to experiment.
2. :doc:`dce-dev-doc`: The document is intended for those who want to extend DCE.
3. :doc:`how-it-works`: This section explains how DCE works.

Features
========

- To run an application using DCE, it is **not** necessary to **change** these sources. However you will need to **recompile** it.
- The simulation is executed wholly within a **single process** which greatly facilitates the **debugging**.
- DCE is very **memory-efficient**, thanks to the way it loads the executables similarly to shared libraries.

Supported Environment
=====================

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


Contents:

.. toctree::
   :maxdepth: 1

   getting-started
   dce-subprojects
   dce-user-doc
   dce-dev-doc
   how-it-works

   dce-about
 
    

