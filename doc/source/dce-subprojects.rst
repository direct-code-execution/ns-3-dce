.. include:: replace.txt

Subprojects of DCE
******************

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
`More information <../../manual-quagga/html/index.html>`_.

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
The `umip <http://umip.org>`_ (Usagi-Patched Mobile IPv6 stack) support on DCE enables the users to reuse routing protocol implementations of Mobile IPv6. UMIP now supports Mobile IPv6 (RFC3775), Network Mobility (RFC3963), Proxy Mobile Ipv6 (RFC5213), etc, and can be used these protocols implementation as models of network simulation. 

For more information, see the latest support `document <../../manual-umip/html/index.html>`_.

Linux kernel (from 2.6.36 to 3.14 version)
============================================
Linux kernel support is built with a separate 'dce-linux' module, 
available on `github
<https://github.com/direct-code-execution/net-next-sim>`_. Many
protocols implemented in kernel space such as TCP, IPv4/IPv6, Mobile
IPv6, Multipath-TCP, SCTP, DCCP, etc, are available with ns-3.

FreeBSD kernel (10.0.0 version)
=========================================
`FreeBSD kernel support <https://github.com/direct-code-execution/freebsd-sim>`_ is based on Linux kernel module of DCE. A few
protocols implemented in kernel space such as TCP, IPv4, etc, are
available with ns-3.


thttpd
======
(TBA)

torrent
=======
(TBA)
