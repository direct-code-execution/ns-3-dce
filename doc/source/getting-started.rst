.. include:: replace.txt


Quick Start
-----------

Introduction
************

The DCE ns-3 module provides facilities to execute within ns-3 existing
implementations of userspace and kernelspace network protocols. 

As of today, the Quagga routing protocol implementation, the CCNx CCN
implementation, and recent versions of the Linux kernel network stack are
known to run within DCE, hence allowing network protocol experimenters and
researchers to use the unmodified implementation of their protocols for
real-world deployments and simulations.

.. _build-dce:

Build DCE
*********

DCE offers two major modes of operation:
 1. The basic mode, where DCE use the |ns3| TCP stacks,
 2. The advanced mode, where DCE uses a Linux network stack instead.

Building DCE basic mode
+++++++++++++++++++++++

First you need to download Bake using Git and set some environment variables:

.. code-block:: sh

  git clone https://gitlab.com/nsnam/bake.git
  cd bake
  export PATH=$PATH:`pwd`/build/bin:`pwd`/build/bin_dce
  export PYTHONPATH=$PYTHONPATH:`pwd`/build/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/build/lib


then you can install it using bake:

.. code-block:: sh

   bake.py configure -e dce-ns3-$version
   bake.py check
   bake.py show

The check and show commands show you missing packages needed by the configured
module.  Please use your package manager to install missing ones, and then
proceed with:

.. code-block:: sh

   bake.py download
   bake.py build
 
Note that dce-ns3-$version is the DCE module with a version number. If you would like to use the development version of DCE module, you can specify **dce-ns3-dev** as a module name for bake.  As of March 2020, this (dev version) is recommended.

the output should look something like this:

.. code-block:: none

  Installing selected module and dependencies.
  Please, be patient, this may take a while!
  >> Downloading ccnx
  >> Download ccnx - OK
  >> Downloading iperf
  >> Download iperf - OK
  >> Downloading ns-3-dev-dce
  >> Download ns-3-dev-dce - OK
  >> Downloading dce-ns3
  >> Download dce-ns3 - OK
  >> Building ccnx
  >> Built ccnx - OK
  >> Building iperf
  >> Built iperf - OK
  >> Building ns-3-dev-dce
  >> Built ns-3-dev-dce - OK
  >> Building dce-ns3
  >> Built dce-ns3 - OK

Building DCE advanced mode (with Linux kernel)
++++++++++++++++++++++++++++++++++++++++++++++
If you would like to try Linux network stack instead of |ns3| network stack, you can try the advanced mode.
The difference to build the advanced mode is the different module name *dce-linux* instead of *dce-ns3* (basic mode).

.. code-block:: sh
    :caption: Installation of DCE via `bake`
    :linenos:

    bake.py configure -e dce-linux-$version
    bake.py check
    bake.py show 
    bake.py download
    bake.py build

Note that dce-linux-$version is the DCE module with a version number. If you would like to use the development version of DCE module, you can specify **dce-linux-dev** as a module name for bake.


Building DCE using WAF
++++++++++++++++++++++

While Bake is the best option, another one is the configuration and build using WAF.
WAF is a Python-based framework for configuring, compiling and installing applications.
The configuration scripts are coded in Python files named *wscript*, calling the WAF 
framework, and called by the *waf* executable.
 
In this case you need to install the single packages one by one. You may want to start with *ns-3*:


* GIT_NS3= |git_ns3|
* LAST_VERSION= |ns3_tag|


.. code-block:: sh
    :caption: ns-3 installation
    :linenos:

    export HOME=`pwd`
    mkdir dce
    cd dce
    # Download pybindgen (optional)
    git clone https://github.com/gjcarneiro/pybindgen.git
    cd pybindgen
    python3 setup.py install

    # Download ns-3
    # git instructions
    git clone GIT_NS3
    git checkout LAST_VERSION

    # Configure
    ./waf configure --enable-examples -d optimized --prefix=$HOME/dce/install

    # Build and install in the directory specified by
    # --prefix parameter
    ./waf build
    ./waf install

More detailed information on installation requirements can be found on the `ns-3 wiki <https://www.nsnam.org/wiki/Installation>`_.
 
 
Then you can download and install *net-next-nuse and DCE (*net-next-nuse* includes the linux stack module):
 
* GIT_DCE= |git_dce|

.. code-block:: sh
    :caption: Kernel and DCE installation 
    :linenos:

    # Clone net-next-nuse
    git clone https://github.com/libos-nuse/net-next-nuse.git
    cd net-next-nuse
    # Select a kernel version
    git checkout libos-v4.4
    # Configure and build
    make defconfig OPT=yes ARCH=sim
    make library OPT=yes ARCH=sim
    cd ..

    # Download, configure, build and install DCE
    git clone GIT_DCE
    ./waf configure --with-ns3=$HOME/dce/build --enable-opt \
                    --enable-kernel-stack=$HOME/dce/net-next-nuse/arch \
                    --prefix=$HOME/dce/install
    ./waf build
    ./waf install


Examples
********
If you succeeded to build DCE, you can try an example script which is already included in DCE package.
  
Example: Simple UDP socket application
++++++++++++++++++++++++++++++++++++++

This example execute the binaries named udp-client and udp-server under |ns3| using DCE.
These 2 binaries are written using POSIX socket API in order to send and receive UDP packets.

If you would like to see what is going on this script, please refer to the :ref:`user's guide<dce-udp-simple-example>`.


.. code-block:: console
  :caption: Listing of the files generated by the dce-udp-simple example

  $ cd source/ns-3-dce
  $ ./waf --run dce-udp-simple
  $ ls 
    elf-cache  files-0 exitprocs
  $ ls -lR files-0
    files-0:
    total 4
    drwxr-x--- 3 furbani planete 4096 Sep  2 17:02 var

    files-0/var:
    total 4
    drwxr-x--- 4 furbani planete 4096 Sep  2 17:02 log

    files-0/var/log:
    total 8
    drwxr-x--- 2 furbani planete 4096 Sep  2 17:02 53512
    drwxr-x--- 2 furbani planete 4096 Sep  2 17:02 53513

    files-0/var/log/53512:
    total 12
    -rw------- 1 furbani planete  12 Sep  2 17:02 cmdline
    -rw------- 1 furbani planete 185 Sep  2 17:02 status
    -rw------- 1 furbani planete   0 Sep  2 17:02 stderr
    -rw------- 1 furbani planete  21 Sep  2 17:02 stdout

    files-0/var/log/53513:
    total 12
    -rw------- 1 furbani planete  22 Sep  2 17:02 cmdline
    -rw------- 1 furbani planete 185 Sep  2 17:02 status
    -rw------- 1 furbani planete   0 Sep  2 17:02 stderr
    -rw------- 1 furbani planete  22 Sep  2 17:02 stdout

This simulation produces two directories, the content of elf-cache is not important now for us, but files-0 is.
files-0 contains first node's file system, it also contains the output files of the dce applications launched on this node. In the /var/log directory there are some directories named with the virtual pid of corresponding DCE applications. Under these directories there is always 4 files:

1. cmdline: which contains the command line of the corresponding DCE application, in order to help you to retrieve what is it,
2. stdout: contains the stdout produced by the execution of the corresponding application,
3. stderr: contains the stderr produced by the execution of the corresponding application.
4. status: contains a status of the corresponding process with its start time. This file also contains the end time and exit code if applicable.
              
Before launching a simulation, you may also create files-xx directories and provide files required by the applications to be executed correctly.

Example: iperf
++++++++++++++

This example shows the usage of iperf with DCE. You are able to generate traffic by well-known traffic generator `iperf` in your simulation.
For more detail of the scenario description, please refer to the :ref:`user's guide<dce-iperf-example>`.

Once you successfully installed DCE with bake, you can execute the example using iperf.

.. code-block:: sh

  cd source/ns-3-dce
  ./waf --run dce-iperf
  
As we saw in the previous example the experience creates directories containing the outputs of different executables,
take a look at the server (node 1) output:

.. code-block:: none

  $ cat files-1/var/log/*/stdout
  ------------------------------------------------------------
  Server listening on TCP port 5001
  TCP window size:  124 KByte (default)
  ------------------------------------------------------------
  [  4] local 10.1.1.2 port 5001 connected with 10.1.1.1 port 49153
  [ ID] Interval       Transfer     Bandwidth
  [  4]  0.0-11.2 sec  5.75 MBytes  4.30 Mbits/sec

 
the client (node-0) output bellow:

.. code-block:: none
    :caption: iperf output

    $ cat files-0/var/log/*/stdout
    ------------------------------------------------------------
    Client connecting to 10.1.1.2, TCP port 5001
    TCP window size:  124 KByte (default)
    ------------------------------------------------------------
    [  3] local 10.1.1.1 port 49153 connected with 10.1.1.2 port 5001
    [ ID] Interval       Transfer     Bandwidth
    [  3]  0.0- 1.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  1.0- 2.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  2.0- 3.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  3.0- 4.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  4.0- 5.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  5.0- 6.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  6.0- 7.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  7.0- 8.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  8.0- 9.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  9.0-10.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  0.0-10.2 sec  5.75 MBytes  4.72 Mbits/sec


if you have already built the advanced mode, you can use Linux network stack over iperf.

.. code-block:: sh

    cd source/ns-3-dce
    ./waf --run "dce-iperf --stack=linux"

the command line option **--stack=linux** makes the simulation use the Linux kernel stack instead of |ns3| network stack.

.. code-block:: none

    $ cat files-1/var/log/*/stdout
    ------------------------------------------------------------
    Server listening on TCP port 5001
    TCP window size: 85.3 KByte (default)
    ------------------------------------------------------------
    [  4] local 10.1.1.2 port 5001 connected with 10.1.1.1 port 60120
    [ ID] Interval       Transfer     Bandwidth
    [  4]  0.0-11.2 sec  5.88 MBytes  4.41 Mbits/sec


.. code-block:: none

    $ cat files-0/var/log/*/stdout
    ------------------------------------------------------------
    Client connecting to 10.1.1.2, TCP port 5001
    TCP window size: 16.0 KByte (default)
    ------------------------------------------------------------
    [  3] local 10.1.1.1 port 60120 connected with 10.1.1.2 port 5001
    [ ID] Interval       Transfer     Bandwidth
    [  3]  0.0- 1.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  1.0- 2.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  2.0- 3.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  3.0- 4.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  4.0- 5.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  5.0- 6.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  6.0- 7.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  7.0- 8.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  8.0- 9.0 sec   512 KBytes  4.19 Mbits/sec
    [  3]  9.0-10.0 sec   640 KBytes  5.24 Mbits/sec
    [  3]  0.0-10.2 sec  5.88 MBytes  4.84 Mbits/sec

Interestingly, the two results between two network stacks are slightly different, though the difference is out of scope of this document.
