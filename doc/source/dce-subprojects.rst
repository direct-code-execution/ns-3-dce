.. include:: replace.txt
Supported Applications
**********************

Below are the list of the tested applications with DCE.

.. contents::
   :local:

CCNx
====

`CCNx <http://www.ccnx.org/about/>`_ is an open source implementation of **Content Centric Networking**.
All the C written executables are supported and none of the Java ones. The versions tested are those between 0.4.0 and 0.6.1 included.

For more detail document, see :doc:`dce-ccnx`.

Quagga
======
`Quagga <http://www.quagga.net/about.php>`_ is a routing software suite, providing implementations of OSPFv2, OSPFv3, RIP v1 and v2, RIPng and BGP-4 for Unix platforms, particularly FreeBSD, Linux, Solaris and NetBSD.
For more information, see the latest support `document <http://www.nsnam.org/~thehajime/ns-3-dce-quagga/index.html>`_.

iperf
=====

iperf from the following archive  `<http://walami.googlecode.com/files/iperf-2.0.5.tar.gz>`_ as been tested.
It is the exception that proves the rule. That is to say that this particular example requires a change in its code. 
In the source file named ``Thread.c`` at line 412 in the function named ``thread_rest`` you must add a ``sleep(1)`` in order to help DCE to break the infinite loop.

ping/ping6
==========

Ping from the following archive `<http://www.skbuff.net/iputils/iputils-s20101006.tar.bz2>`_ is supported.

ip (iproute2 package)
=====================

.. iptables

umip (Mobilt IPv6 daemon)
=========================

Linux kernel (from 2.6.36 to 3.7 version)
=========================================

thttpd
======

torrent
=======

.. toctree::
   :maxdepth: 1

   dce-ccnx
..   dce-quagga
..   dce-umip
..   dce-linux
