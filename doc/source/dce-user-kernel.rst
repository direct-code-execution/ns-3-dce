.. include:: replace.txt

.. _net-next-sim: https://github.com/direct-code-execution/net-next-sim 
.. _mptcp:  https://github.com/multipath-tcp/mptcp
.. _mptcp_git:  git://github.com/multipath-tcp/mptcp
.. _libos:  https://github.com/libos-nuse/net-next-nuse
.. _lkl:    https://github.com/lkl/linux
.. _linux_vanilla_git:  git@github.com:torvalds/linux.git

Using your in-kernel protocol implementation
============================================

There are a number of protocols implemented in kernel space, like many transport protocols (e.g., TCP, UDP, SCTP), Layer-3 forwarding plane (IPv4, v6 with related protocols ARP, Neighbor Discovery, etc). 
DCE can simulate these protocols with |ns3| and a kernel compiled as a library. This is not possible with the linux vanilla kernel hence you need to use a slightly modified kernel (net-next-sim_ - deprecated - or libos_ which is the successor of net-next-sim_).

This document describes how to retrieve, configure, compile and use this custom kernel in DCE.
As an example, it shows how to enable an in-tree but optional protocol - Stream Control Transmission Protocol (SCTP) - and an out of tree protocol: Multipath TCP (MPTCP).
Although other protocols may not adapt these patterns as-is, you will see what's needed to implement for your purpose.

.. contents::
   :local:

.. _configure:

1. Configure a kernel (make menuconfig in Linux)
-----------------------------------------------------------
In Linux kernel, there is a configuration system in Linux kernel to enable/disable features. This is typically done by *make menuconfig* command, and it writes a file (.config) at the kernel source directory. Build system (e.g., make bzImage) refers the file which source files are compiled.

.. figure:: images/dce-make-menuconfig.png
   :scale: 70 %

In our DCE Linux kernel module (i.e. **net-next-sim** available at github.com), we have *arch/sim/defconfig* file to store the default configuration of kernel features. You may need to add proper configuration parameters (e.g., CONFIG_IP_SCTP) to build the protocol by default.

For the Linux SCTP implementation, we need at least the following configuration parameters.

 - CONFIG_IP_SCTP=y
 - CONFIG_SCTP_DEFAULT_COOKIE_HMAC_NONE=y
 - CONFIG_CRYPTO_CRC32C=y
 - CONFIG_CRC32=y

These can be added to the file (*arch/sim/defconfig*) or added manually to *.config* when needed.

**Note**

These configuration options **SHOULD** be minimized at the beginning since adding new option may require new functions which DCE doesn't support at the time and need to implement glue code.



2. Build a DCE compatible linux kernel 
---------------------------------------------

DCE can simulate these protocols with |ns3| and a kernel compiled as a library named *liblinux.so*. This is not possible with the linux vanilla kernel hence you need to use a slightly modified kernel (net-next-sim_ - deprecated - or libos_ which is the successor of net-next-sim_).
The following section presents the two methods, net-next-sim_ being slightly easier to install. 

2.1 With *net-next-sim* (deprecated)
----------------------------------------------------------------------
To build the liblinux.so, DCE version of Linux kernel,

::

    make defconfig ARCH=sim
    make library ARCH=sim

You can use *make menuconfig* command (below) instead of editing the defconfig file.
If everything is fine, you will see **liblinux.so** linked to **libsim-linuxv.y.z.so** file at the root directory of Linux kernel.

::

    make menuconfig ARCH=sim


2.2 Build an MPTCP kernel with *net-next-sim*
--------------------------------------------------------------------

DISCLAIMER: This is a transcript of `Hajime's guide https://plus.google.com/+HajimeTazaki/posts/1QUmR3n3vNA`  updated on a best effort basis. Hence it may be possible to compile newer versions, in which case patches are welcome.
Build steps for DCE integration is also available as a script in *utils/mptcp-build.sh*.

#) Get linux kernel mptcp_ variant:

    .. code:: shell 

        $ git clone mptcp_git_
        $ git checkout -b sim3.14 mptcp_v0.89

.. 3.14 can work with mptcp 0.89.8.

#) Merge in mptcp code the changes required to make it work in DCE. Those changes are listed in the net-next-sim_ project

    .. code:: shell 

        % cd mptcp
        % git remote add dce git://github.com/direct-code-execution/net-next-sim.git 
        % git fetch dce
        % git merge dce/sim-ns3-3.14.0-branch

#) Enable mptcp in the kernel configuration. There are 2 ways possible:

    - patch the kernel config manually (you can 

        .. code:: shell 

            % cat >> arch/sim/defconfig <<END
            CONFIG_MPTCP=y
            CONFIG_MPTCP_PM_ADVANCED=y
            CONFIG_MPTCP_FULLMESH=y
            CONFIG_MPTCP_NDIFFPORTS=y
            CONFIG_DEFAULT_FULLMESH=y
            CONFIG_DEFAULT_MPTCP_PM="fullmesh"
            CONFIG_TCP_CONG_COUPLED=y
            CONFIG_TCP_CONG_OLIA=y
            END


   - or use menuconfig to enable these options as explained in 
     http://multipath-tcp.org/pmwiki.php/Users/DoItYourself

    .. code:: shell 

        % make menuconfig ARCH=sim

#) Generate the kernel configuration:

    .. code:: shell 

        % make defconfig ARCH=sim


#) build kernel (as a shared library)

    .. code:: shell 

        $ make library ARCH=sim



If everything is going well, you can try to use it over ns-3

#) build ns-3 related tools

    .. code:: shell

        % make testbin -C arch/sim/test

#) run an mptcp simulation !

   .. code :: shell

        % cd arch/sim/test/buildtop/source/ns-3-dce
        % ./waf --run dce-iperf-mptcp

you should see generated \*.pcap files in your dce folder.

2.3 With libos_
----------------------------------------

Libos_ is the successor of net-next-sim_. There are attempts to merge libos_ within
the linux kernel library lkl_ project but it will take quite some time before being able to run lkl_ in DCE.


#) Get libos_ code:

    .. code:: shell 

        $ git clone git@github.com:libos-nuse/net-next-nuse.git
        $ git checkout libos-v4.4
        $ cd net-next-nuse

#) Configure the kernel (you can refer to configure_)

    .. code:: shell

        make defconfig ARCH=lib

#) Compile the kernel

    .. code:: shell

        make library ARCH=lib

    .. target library is defined in arch/lib/Makefile 

    This will download and compile git submodules from https://github.com/libos-nuse/linux-libos-tools and execute
    additionnal steps from the arch/lib/tools/Makefile, as for instance generate an additional libsim_linux.so.
    This is the shared library you need to load in DCE. By default DCE looks for "liblinux.so" so you should do:

    .. code:: cpp
      
        DceManagerHelper dceManager;
        dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("libsim-linux.so"));

3. Write user space application to use this protocol
------------------------------------------------------
Then, we need to write userspace applications using new feature of kernel protocol. In case of SCTP, we wrote `sctp-client.cc <http://code.nsnam.org/ns-3-dce/file/tip/example/sctp-client.cc>`_ and `sctp-server.cc <http://code.nsnam.org/ns-3-dce/file/tip/example/sctp-server.cc>`_.

**Optional**
You may optinally need external libraries to build/run the applications. In this case, the applications need *lksctp-tools*, so that applications fully benefit the features of SCTP, rather than only using standard POSIX socket API. 

Moreover, adding system dependency to bake configuration file (i.e., bakeconf.xml) would be nice to assist build procedure.
The following is an example of *lksctp-tools*, which above applications use.

::

    <module name="lksctp-dev">
      <source type="system_dependency">
        <attribute name="dependency_test" value="sctp.h"/>
        <attribute name="try_to_install" value="True"/>
        <attribute name="name_apt-get" value="lksctp-dev"/>
        <attribute name="name_yum" value="lksctp-tools-devel"/>
        <attribute name="more_information" value="Didn't find:   lksctp-dev package; please install it."/>
      </source>
      <build type="none" objdir="no">
      </build>
    </module>

4. Write ns-3 scenario to use above applications.
---------------------------------------------------
The next step would be writing |ns3| simulation scenario to use the applications you prepared. 
You need first to add to your *DCE_PATH* the path to the previously compiled *liblinux.so*:

::
    $ export DCE_PATH="$HOME/net-next-sim:$DCE_PATH"

`dce-sctp-simple.cc <http://code.nsnam.org/ns-3-dce/file/tip/example/dce-sctp-simple.cc>`_ is the script that we prepared. In the script, you may need to load the applications by using *DceApplicationHelper* as follows.

::

    DceApplicationHelper process;
    ApplicationContainer apps;

    /* by default DCE, loads liblinux.so. If for some reason your library has a different name,
     either create a symlink or use these commands to change the name */
    DceManagerHelper dceManager;
    dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));

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

5. run it !
-------------

::

  ./waf --run dce-simple-sctp

If you're lucky, it's done.

If you aren't lucky, you may face errors of DCE, such as unresolved symbols in system calls (called by userspace applications) or missing kernel functions (used by newly added CONFIG_IP_SCTP option), or invalid memory access causing segmentation fault. In that case, adding missing functions, so called *glue-code* would be the next step.

 


.. 6. Write Wrapper classes for DCE Cradle
.. 7. Test
