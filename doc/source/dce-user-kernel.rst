.. include:: replace.txt

.. _lkl:    https://github.com/lkl/linux

Using your in-kernel protocol implementation
============================================

There are a number of protocols implemented in kernel space, like many
transport protocols (e.g., TCP, UDP, SCTP, DCCP, MPTCP) and the Layer-3
forwarding plane (IPv4, IPv6 with related protocols ARP, Neighbor
Discovery, etc). DCE simulates these protocols with |ns3| and the Linux
kernel built as a library: the Linux Kernel Library, lkl_ (Linux 6.12).

Each node gets its own kernel; every ns-3 NetDevice of the node becomes a
kernel network device named ``sim<n>``, and the sockets of DCE
applications are kernel sockets.

This document describes how to build the kernel library, configure it,
and use it in DCE, with SCTP and Multipath TCP (MPTCP) as examples.

.. contents::
   :local:

.. _configure:

1. Configure the kernel
-----------------------

``utils/build_lkl.sh`` configures the kernel with LKL's defconfig plus
``utils/lkl-dce.config``, which enables what the DCE examples and tests
use: SCTP, DCCP, MPTCP, AF_PACKET, IPsec, Mobile IPv6, tunnels, multicast
routing and more TCP congestion controls. To enable another feature, add
its options to ``utils/lkl-dce.config``; ``build_lkl.sh`` reconfigures the
kernel when the file changes.

For instance, the Linux SCTP implementation needs ``CONFIG_IP_SCTP=y``.

2. Build the kernel library
---------------------------

::

    $ ./utils/build_lkl.sh ../lkl ../lkl/dce

The first argument is the LKL source tree (cloned at the pinned version if
missing), the second the output directory. ``build_lkl.sh`` applies the
patches in ``utils/lkl-patches`` (they let the kernel run on simulated
time) and writes ``liblkl.so``, its debug information
(``liblkl.so.debug``) and the headers DCE is built with.

Then configure DCE with it, and make ``liblkl.so`` visible through
*DCE_PATH* (e.g. by copying it to ``build/bin_dce``):

::

    $ ./waf configure --with-ns3=$HOME/dce/build --with-lkl=../lkl/dce
    $ ./waf build
    $ cp ../lkl/dce/liblkl.so build/bin_dce/

3. Write user space application to use this protocol
----------------------------------------------------

Then, we need userspace applications using the new feature of the kernel
protocol. In case of SCTP, see ``example/sctp-client.cc`` and
``example/sctp-server.cc``. These need *lksctp-tools* (``libsctp-dev``)
to build.

4. Write ns-3 scenario to use above applications
------------------------------------------------

``example/dce-sctp-simple.cc`` is the script that we prepared. The kernel
is selected with ``ns3::LinuxSocketFdFactory`` (its *Library* attribute
defaults to ``liblkl.so``; ``liblinux.so``, the name of the former libos
kernel, selects it too):

::

    DceManagerHelper dceManager;
    dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory");
    dceManager.Install (nodes);

    DceApplicationHelper process;
    ApplicationContainer apps;

    process.SetBinary ("sctp-server");
    process.ResetArguments ();
    process.SetStackSize (1<<16);
    apps = process.Install (nodes.Get (0));
    apps.Start (Seconds (1.0));

    process.SetBinary ("sctp-client");
    process.ResetArguments ();
    process.ParseArguments ("10.0.0.1");
    apps = process.Install (nodes.Get (1));
    apps.Start (Seconds (1.5));

Network configuration is done as on Linux, with ``ip`` (iproute2 6.12,
built by ``utils/build_kernel_deps.sh``) through
``LinuxStackHelper::RunIp``, and with sysctls through
``LinuxStackHelper::SysctlSet`` and ``SysctlGet``: ``".net.ipv4.tcp_sack"``
is ``/proc/sys/net/ipv4/tcp_sack``; ``SysctlGet`` also reads any kernel
file given by its absolute path, e.g. ``/proc/net/netstat``.

Multipath TCP
~~~~~~~~~~~~~

Linux uses MPTCP only for sockets created with ``IPPROTO_MPTCP``. The
*Mptcp* attribute makes the TCP sockets of all applications MPTCP sockets,
like ``mptcpize`` does on Linux; the subflows are configured with
``ip mptcp``:

::

    Config::SetDefault ("ns3::LklSocketFdFactory::Mptcp", BooleanValue (true));
    ...
    LinuxStackHelper::RunIp (client, Seconds (0.5), "mptcp limits set subflows 4 add_addr_accepted 4");
    LinuxStackHelper::RunIp (server, Seconds (0.5), "mptcp limits set subflows 4 add_addr_accepted 4");
    // a subflow from the client's second address
    LinuxStackHelper::RunIp (client, Seconds (0.5), "mptcp endpoint add 10.1.1.1 dev sim1 subflow");

See ``example/dce-iperf-mptcp.cc`` and ``test/dce-mptcp-test.cc``.

5. Run it
---------

::

  ./waf --run dce-sctp-simple

If you face errors, such as unresolved symbols in system calls (called by
userspace applications) or invalid memory access, adding the missing
functions to DCE, so called *glue-code*, is the next step.
