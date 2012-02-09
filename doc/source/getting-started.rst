Introduction
------------

The DCE ns-3 module provides facilities to execute within ns-3 existing
implementations of userspace and kernelspace network protocols. 

As of today, the Quagga routing protocol implementation, the CCNx CCN
implementation, and recent versions of the Linux kernel network stack are
known to run within DCE, hence allowing network protocol experimenters and
researchers to use the unmodified implementation of their protocols for
real-world deployments and simulations.


Getting Started
---------------

Prerequisite
************

DCE has been tested only on few systems see:  `Ns-3-dce portability <http://www.nsnam.org/wiki/index.php/Ns-3-dce_portability>`_.
Now I am using it well with a Fedora 14 x86-64 and glibc 2.13-1.

Building options
****************

DCE offers two majors mode of operation:
 1. The basic mode, where DCE use the NS3 TCP stacks,
 2. The advanced mode, where DCE uses a linux network stack instead.

Building ns-3 and DCE
*********************

First you need to download NS-3 DCE using mercurial:

::

  $ mkdir test_build_ns3_dce
  $ cd test_build_ns3_dce
  $ hg clone http://code.nsnam.org/furbani/ns-3-dce 


DCE brings a script to retrieve the other sources and build the whole things:

:: 

  $ ls ns-3-dce/utils/clone_and_compile_ns3_dce.sh

You should edit this file if you want to try the advanced mode and change the following line:

::

  USE_KERNEL=NO

to

::

  USE_KERNEL=YES

then you can build the project:

::

  $ ns-3-dce/utils/clone_and_compile_ns3_dce.sh
  clone readversiondef
  ...
  2 files updated, 0 files merged, 0 files removed, 0 files unresolved
  clone ns-3-dce
  ...
  2105 files updated, 0 files merged, 0 files removed, 0 files unresolved
  ...
  Launch NS3TEST-DCE
  PASS process-manager 16.030ms
    PASS Check that process "test-empty" completes correctly. 1.220ms
    PASS Check that process "test-sleep" completes correctly. 0.030ms
    PASS Check that process "test-pthread" completes correctly. 0.020ms
    PASS Check that process "test-mutex" completes correctly. 0.110ms
    PASS Check that process "test-once" completes correctly. 0.030ms
    PASS Check that process "test-pthread-key" completes correctly. 0.020ms
    PASS Check that process "test-sem" completes correctly. 0.040ms
    PASS Check that process "test-malloc" completes correctly. 0.030ms
    PASS Check that process "test-malloc-2" completes correctly. 0.020ms
    PASS Check that process "test-fd-simple" completes correctly. 0.030ms
    PASS Check that process "test-strerror" completes correctly. 0.030ms
    PASS Check that process "test-stdio" completes correctly. 0.030ms
    PASS Check that process "test-string" completes correctly. 0.030ms
    PASS Check that process "test-netdb" completes correctly. 0.220ms
    PASS Check that process "test-env" completes correctly. 0.030ms
    PASS Check that process "test-cond" completes correctly. 0.100ms
    PASS Check that process "test-timer-fd" completes correctly. 0.030ms
    PASS Check that process "test-stdlib" completes correctly. 0.030ms
    PASS Check that process "test-select" completes correctly. 0.080ms
    PASS Check that process "test-nanosleep" completes correctly. 0.030ms
    PASS Check that process "test-random" completes correctly. 0.030ms
    PASS Check that process "test-fork" completes correctly. 0.030ms
    PASS Check that process "test-local-socket" completes correctly. 12.840ms
    PASS Check that process "test-poll" completes correctly. 0.090ms
    PASS Check that process "test-tcp-socket" completes correctly. 0.880ms
    
DCE is compiled after a few minutes and if the tests completed successfully, you should see the directories:

::

  $ ls
  build  ns-3-dev  ns-3-dce  readversiondef ns-3-linux iproute2-2.6.33

Where:
 1. *build* contains the result of compilation: some binaries some libs and some include files usable to do your simulations scripts.

 2. *ns-3-dev* contains the current sources of NS3, 

 3. *ns-3-dce* contains the DCE sources,

 4. *readversiondef* contains source of a tool used by DCE build system. 

 5. *ns-3-linux* (only in advanced mode) contains source of a linux kernel + some glue code for DCE / Kernel communication.

 6. *iproute2-2.6.33* (only in advanced mode) contains source of *ip* tool needed to be compiled for DCE in order to configure ip routes of the slave kernel used by DCE.

Setting Environnement
*********************

Call the setenv.sh script to correctly setup the environment variables (i.e., PATH, LD_LIBRARY_PATH and PKG_CONFIG_PATH)

::

  $ source ns-3-dce/utils/setenv.sh

Examples
********

Under example directory there is the sources files of DCE examples simulations.

Example: DCE UDP EXAMPLE
########################

This example execute the binaries named udp-client and udp-server under NS3 using DCE.

These 2 binaries are writen using libc api in order to send and receive udp packets.

Please take time to look at the source dce-udp-simple.cc which is our NS3 simulation "script":

::

  int main (int argc, char *argv[])
  {
    CommandLine cmd;
    cmd.Parse (argc, argv);
  
    NodeContainer nodes;
    nodes.Create (1);
  
    InternetStackHelper stack;
    stack.Install (nodes);
  
    DceManagerHelper dceManager;
    dceManager.Install (nodes);
  
    DceApplicationHelper dce;
    ApplicationContainer apps;
  
    dce.SetStackSize (1<<20);
  
    dce.SetBinary ("udp-server");
    dce.ResetArguments();
    apps = dce.Install (nodes.Get (0));
    apps.Start (Seconds (4.0));
  
    dce.SetBinary ("udp-client");
    dce.ResetArguments();
    dce.AddArgument ("127.0.0.1");
    apps = dce.Install (nodes.Get (0));
    apps.Start (Seconds (4.5));
  
    Simulator::Stop (Seconds(1000100.0));
    Simulator::Run ();
    Simulator::Destroy ();
  
    return 0;
  }
  
You can notice that we create a NS-3 Node with an Internet Stack (please refer to `NS-3 <http://www.nsnam.org/documentation/>`_ doc. for more info),
and we can also see 2 new Helpers:

 1. DceManagerHelper which is used to Manage DCE loading system in each node where DCE will be used.
 2. DceApplicationHelper which is used to describe real application to be lauched by DCE within NS-3 simulation environnement.
 
As you have already set the environnement variables you can launch this simulation from anywhere:

::

  $ cd /tmp
  $ mkdir my_test
  $ cd my_test
  $ dce-udp-simple
  $ ls 
    elf-cache  files-0
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
files-0 contains first node's file system, it also contains the output files of the dce applications launched on this node. In the /var/log directory there is some directories named with the virtual pid of corresponding DCE applications. Under these directories there is always 4 files:

1. cmdline: which contains the command line of the corresponding DCE application, in order to help you to retrieve what is it,
2. stdout: contains the stdout produced by the execution of the corresponding application,
3. stderr: contains the stderr produced by the execution of the corresponding application.
4. status: contains a status of the corresponding process with its start time. This file also contains the end time and exit code if applicable.
              
Before launching a simulation, you may also create files-xx directories and provide files required by the applications to be executed correctly.

DCE LINUX Example
#################

This example shows how to use DCE in advanced mode, with a linux kernel IP stack.
It uses also the binaries *udp-server* and *udp-client* like the above example, there is also *tcp-server* and *tcp-client* if you choose the reliable transport option.
Two other binaries are needed: the linux kernel stack named *libnet-next-2.6.so* and the tool needed to configure this kernel stack named *ip*.
This example simulates an exchange of data between too nodes, using TCP or UDP, and the nodes are linked by one of three possible links , Wifi, Point 2 point or CSMA.
The main executable is named *dce-linux*, it cames with too options:

1. linkType allow to choose the link type between c, w or p for Csma, Wifi or Point 2 point,
2. reliable allow to choose transport between TCP (1) or UDP (0).

The following code snippet show how to enable DCE advanced mode (you can see it in the source file dce-linux.cc under example directory):

::

  DceManagerHelper processManager;
 // processManager.SetLoader ("ns3::DlmLoaderFactory");
  processManager.SetNetworkStack("ns3::LinuxSocketFdFactory", "Library", StringValue ("libnet-next-2.6.so"));
  processManager.Install (nodes);

  for (int n=0; n < 2; n++)
    {
      AddAddress (nodes.Get (n), Seconds (0.1), "sim0", "10.0.0.", 2 + n, "/8" );
      RunIp (nodes.Get (n), Seconds (0.11), ( 'p' == linkType )? "link set sim0 up arp off":"link set sim0 up arp on");
      RunIp (nodes.Get (n), Seconds (0.2), "link show");
      RunIp (nodes.Get (n), Seconds (0.3), "route show table all");
      RunIp (nodes.Get (n), Seconds (0.4), "addr list");
    }

The first important call is *SetNetworkStack* used to indicate which file contains the linux kernel stack.
Then in the for loop we setup on each nodes the network interfaces using the ip executable to configure the kernel stack.
Because this source code factorizes some call, it is not very readeable so below there is the corresponding calls to ip executable with the arguments:

::

   ip -f inet addr add 10.0.0.2 dev sim0        // set the ip adresse of the first (sim0) net device of the corresponding node
   ip link set sim0 up arp on                   // enable the use of the device use arp off instead for P2P link
   ip link show
   ip route show table all
   ip addr list

Quagga Example
##############

`Quagga <http://www.quagga.net/about.php>`_ is a routing software suite, providing implementations of OSPFv2, OSPFv3, RIP v1 and v2, RIPng and BGP-4 for Unix platforms, particularly FreeBSD, Linux, Solaris and NetBSD.

Quagga build
============

In order to use Quagga with DCE you should build it following this steps:

::

   $ wget http://www.quagga.net/download/quagga-0.99.20.tar.gz
   $ tar xvf quagga-0.99.20.tar.gz
   $ cd quagga-0.99.20/
   $ CFLAGS=-fPIC LDFLAGS=-pie ./configure --disable-shared --enable-static --disable-user --disable-group --disable-capabilities 

At this step you should disable 2 things in config.h file ie HAVE_CLOCK_MONOTONIC and HAVE_RUSAGE by commenting them out like that:

::

   $ egrep  HAVE_CLOCK_MONOTONIC\|HAVE_RUSAGE config.h
   //#define HAVE_CLOCK_MONOTONIC /**/
   //#define HAVE_RUSAGE /**/

Then you can build all using *make* command.

For now the quagga helper shipped within DCE use only 2 binaries from quagga which are *zebra* and *ospfd*. So you need to give DCE a way to find this two executables,
after the successfull *make* you should find them under directories *quagga-0.99.20/zebra* and *quagga-0.99.20/ospfd* for example you can add this directories in your PATH env variable.

Quagga test
===========

Under DCE sample directory you should find a file named *dce-zebra-simple.cc*, this file contains a sample scenario using Quagga.
After DCE compilation you must find the executable named *dce-zebra-simple* under *build/bin* directory. You may launch it like this:

::

   $ ./build/bin/dce-zebra-simple 
   Process exit path=/NodeList/0/$ns3::DceManager/Exit pid=53512 retcode=0
   Process exit path=/NodeList/0/$ns3::DceManager/Exit pid=53513 retcode=0  
   Process exit path=/NodeList/1/$ns3::DceManager/Exit pid=39770 retcode=0
   Process exit path=/NodeList/1/$ns3::DceManager/Exit pid=39771 retcode=0
   Process exit path=/NodeList/2/$ns3::DceManager/Exit pid=32477 retcode=0
   Process exit path=/NodeList/2/$ns3::DceManager/Exit pid=32478 retcode=0
   Process exit path=/NodeList/3/$ns3::DceManager/Exit pid=25008 retcode=0
   Process exit path=/NodeList/3/$ns3::DceManager/Exit pid=25009 retcode=0

Without any argument this sample create 4 nodes, then you should look at the results files:

::
   
   $ cat routes.log

   Time: 10s
   Node: 0
   Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
   127.0.0.0       0.0.0.0         255.0.0.0       U     0     -      -   0
   10.1.0.0        0.0.0.0         255.255.255.0   U     0     -      -   1
   10.2.0.0        0.0.0.0         255.255.255.0   U     0     -      -   2
   
   Time: 10s
   Node: 1
   Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
   127.0.0.0       0.0.0.0         255.0.0.0       U     0     -      -   0
   10.1.0.0        0.0.0.0         255.255.255.0   U     0     -      -   1
   10.2.1.0        0.0.0.0         255.255.255.0   U     0     -      -   2
   .....
   $ cat files-[0-3]/var/log/*/cmdline
   zebra -f /usr/local/etc/zebra.conf -i /usr/local/etc/zebra.pid 
   ospfd -f /usr/local/etc/ospfd.conf -i /usr/local/etc/ospfd.pid 
   zebra -f /usr/local/etc/zebra.conf -i /usr/local/etc/zebra.pid 
   ospfd -f /usr/local/etc/ospfd.conf -i /usr/local/etc/ospfd.pid 
   zebra -f /usr/local/etc/zebra.conf -i /usr/local/etc/zebra.pid 
   ospfd -f /usr/local/etc/ospfd.conf -i /usr/local/etc/ospfd.pid 
   zebra -f /usr/local/etc/zebra.conf -i /usr/local/etc/zebra.pid 
   ospfd -f /usr/local/etc/ospfd.conf -i /usr/local/etc/ospfd.pid 
   
   $ cat files-[0-3]/var/log/*/status
   Start Time: NS3 Time:          1s (          +1000000000.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   Start Time: NS3 Time:          2s (          +2000000000.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   Start Time: NS3 Time:          1s (          +1010000000.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   Start Time: NS3 Time:          2s (          +2100000000.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   Start Time: NS3 Time:          1s (          +1020000000.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   Start Time: NS3 Time:          2s (          +2200000000.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   Start Time: NS3 Time:          1s (          +1030000000.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   Start Time: NS3 Time:          2s (          +2299999999.0ns) , REAL Time: 1321290743
         Time: NS3 Time:        150s (        +150000000000.0ns) , REAL Time: 1321290744 --> Stopped by NS3.
   
   $ cat files-[0-3]/var/log/*/std*
   2010/01/01 00:00:01 ZEBRA: Zebra 0.99.20 starting: vty@2601
   2010/01/01 00:00:02 ZEBRA: zebra message comes from socket [9]
   2010/01/01 00:00:02 ZEBRA: zebra message received [ZEBRA_ROUTER_ID_ADD] 0
   2010/01/01 00:00:02 ZEBRA: zebra message comes from socket [9]
   2010/01/01 00:00:02 ZEBRA: zebra message received [ZEBRA_INTERFACE_ADD] 0
   2010/01/01 00:00:03 ZEBRA: zebra message comes from socket [9]
   2010/01/01 00:00:03 ZEBRA: zebra message received [ZEBRA_IPV4_ROUTE_ADD] 17
   2010/01/01 00:00:03 ZEBRA: zebra message comes from socket [9]
   2010/01/01 00:00:03 ZEBRA: zebra message received [ZEBRA_IPV4_ROUTE_ADD] 17
   2010/01/01 00:00:42 ZEBRA: zebra message comes from socket [9]
   2010/01/01 00:00:42 ZEBRA: zebra message received [ZEBRA_IPV4_ROUTE_ADD] 17
   2010/01/01 00:00:42 ZEBRA: netlink_route_multipath() (single hop): RTM_NEWROUTE 10.2.1.0/24, type IPv4 nexthop
   2010/01/01 00:00:42 ZEBRA: netlink_route_multipath() (single hop): nexthop via 10.1.0.2 if 1
   2010/01/01 00:00:42 ZEBRA: netlink_talk: netlink-cmd type RTM_NEWROUTE(24), seq=6
   2010/01/01 00:00:42 ZEBRA: netlink_parse_info: netlink-cmd ACK: type=RTM_NEWROUTE(24), seq=6, pid=0
   2010/01/01 00:00:42 ZEBRA: netlink_parse_info: netlink-listen type RTM_NEWROUTE(24), seq=6, pid=0
   2010/01/01 00:00:42 ZEBRA: RTM_NEWROUTE ipv4 unicast proto Zebra
   .....

         
CCNx examples
#############

Under example/ccnx there is more realistics examples using the implementation of an experimental protocol named CCN. In this examples we use the `PARC  <http://www.parc.com>`_ implementation named `CCNx <http://www.ccnx.org>`_ (c) in its early version 0.4.2.
  
CCNx setup
==========

In order to run ccnx binaries you must compile them with some required compilator and linker parameters.
The principe here is to obtain Position Independent Executable. 
To obtain this type of exe you should use the gcc -fPIC when compiling sources, and the option -pie when linking your exe.
For CNNx we notice that (under linux) its configure script sets by default the -fPIC option, you can check it in the generated file named conf.mk under directory ccnx.0.4.0/csrc:
::

  $ cat cscr/conf.mk
  ...
  PLATCFLAGS=-fPIC
  ...

Then you should start the make like this:

::

  $ make MORE_LDLIBS=-pie

You should also install the ccn binaries in a directory present in the DCE_PATH environment variable, by default after you execute the DCE *setenv.sh* script your should have a DCE_PATH env. var like this :

::

  $ echo $DCE_PATH
  /where/is/ns-3-dce/build/bin_dce:/where/is/ns-3-dce/build/bin:/where/is/ns-3-dce/../build/bin

CCNx installation example
+++++++++++++++++++++++++

::

  $ cd /where/is/ns-3-dce/
  $ wget http://www.ccnx.org/releases/ccnx-0.4.2.tar.gz
  $ tar zxf ccnx-0.4.2.tar.gz && cd ccnx-0.4.2
  $ INSTALL_BASE=$PWD/../build ./configure
  $ make MORE_LDLIBS=-pie && make install
 
CCNx simple test in real world 
++++++++++++++++++++++++++++++

Before using it within DCE we will do a little test in real world.  For this we will start the ccnd daemon, publish a file, and request the file, then you end up stopping the ccnd daemon.

::
 
  $ source /where/is/ns-3-dce/utils/setenv.sh
  $ pwd
  /where/is/ns-3-dce/  
  $ cd build/bin
  $ ls -l ccnd
  -rwxr-xr-x 1 furbani planete 426969 Feb  7 15:54 ccnd
  $ ./ccndstart
  1328704870.766811 ccnd[5211]: CCND_DEBUG=1 CCND_CAP=50000
  1328704870.766964 ccnd[5211]: listening on /tmp/.ccnd.sock
  1328704870.767043 ccnd[5211]: accepting ipv4 datagrams on fd 4 rcvbuf 126976
  1328704870.767068 ccnd[5211]: accepting ipv4 connections on fd 5
  1328704870.767122 ccnd[5211]: accepting ipv6 datagrams on fd 6 rcvbuf 126976
  1328704870.767152 ccnd[5211]: accepting ipv6 connections on fd 7
  1328704870.812268 ccnd[5211]: accepted client fd=8 id=6
  1328704870.812322 ccnd[5211]: shutdown client fd=8 id=6
  1328704870.812332 ccnd[5211]: recycling face id 6 (slot 6)
  $ echo HELLO >file
  $ ./ccnput /H <file &
  $ ./ccnget -c /H
  HELLO
  [1]+  Done                    ./ccnput /H < file
  $ ./ccndstop

Before running the CCN daemon within DCE we make a final verification, we use readelf tool to verify that ccnd executable is of type **DYN**:

::

  $ readelf -h ccnd
  ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Shared object file)
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x31d0
  Start of program headers:          64 (bytes into file)
  Start of section headers:          395872 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         8
  Size of section headers:           64 (bytes)
  Number of section headers:         39
  Section header string table index: 36

Example CCNX-SIMPLE
###################

This simulation launches a *ccnd* daemon, publishes a file using *ccnput* and retrieves this data using *ccnget* command, all commands are on a single node:

::

  $ . ./ns-3-dce/utils/setenv.sh
  $ ./build/bin/dce-ccnd-simple

Verify the status of execution:

::

  $ cat files-0/var/log/*/status
  Start Time: NS3 Time:          0s (                   +0.0ns) , REAL Time: 1328707904
        Time: NS3 Time:          0s (                   +0.0ns) , REAL Time: 1328707904 --> Starting: /user/furbani/home/dev/dce/dev/ns-3-dce/build/bin/ccnd
        Time: NS3 Time:         59s (         +59001000000.0ns) , REAL Time: 1328707905 --> Exit (0)
  Start Time: NS3 Time:          1s (          +1000000000.0ns) , REAL Time: 1328707904
        Time: NS3 Time:          1s (          +1000000000.0ns) , REAL Time: 1328707904 --> Starting: /user/furbani/home/dev/dce/dev/ns-3-dce/build/bin/ccnput
        Time: NS3 Time:          2s (          +2001000000.0ns) , REAL Time: 1328707905 --> Exit (0)
  Start Time: NS3 Time:          2s (          +2000000000.0ns) , REAL Time: 1328707905
        Time: NS3 Time:          2s (          +2000000000.0ns) , REAL Time: 1328707905 --> Starting: /user/furbani/home/dev/dce/dev/ns-3-dce/build/bin/ccnget
        Time: NS3 Time:          2s (          +2002000000.0ns) , REAL Time: 1328707905 --> Exit (0)
  Start Time: NS3 Time:         59s (         +59000000000.0ns) , REAL Time: 1328707905
        Time: NS3 Time:         59s (         +59000000000.0ns) , REAL Time: 1328707905 --> Starting: /user/furbani/home/dev/dce/dev/ns-3-dce/build/bin/ccndsmoketest
        Time: NS3 Time:         59s (         +59001000000.0ns) , REAL Time: 1328707905 --> Exit (0)

Verify the output of the command *ccnget*:

::

  $ cat files-0/var/log/53514/stdout
  The wanted data is here :)




Example CCND LINEAR MULTIPLE
############################

This simulation uses multiple nodes placed in a line, each node are linked 2 by 2 by a point to point link, each node holds a ccnd daemon, the first node put a file (with ccnput), and the last node fetch this file (with ccnget). Also each node minus the first one forward interrests starting with /NODE0 to its predecessor.

  .. image:: images/ccnd-linear-multiple-1.png

The launch script dce-ccnd-linear-multiple offer 3 options:

::

  $ ./build/bin/dce-ccnd-linear-multiple --PrintHelp
  --PrintHelp: Print this help message.
  --PrintGroups: Print the list of groups.
  --PrintTypeIds: Print all TypeIds.
  --PrintGroup=[group]: Print all TypeIds of group.
  --PrintAttributes=[typeid]: Print all attributes of typeid.
  --PrintGlobals: Print the list of globals.
  User Arguments:
      --nNodes: Number of nodes to place in the line
      --tcp: Use TCP to link ccnd daemons.
      --kernel: Use kernel linux IP stack.


 1. nNodes allows to choose the Number of Nodes,
 2. tcp allows to use TCP or if not UDP to connect the ccnd deamons (via forwarding interrest).
 3. kernel allows to use Linux IP Stack (only working in advanced mode) instead of NS3 one.

for example with 200 nodes and TCP transport you should see this in the first ccnget output command:

::

  $ ./build/bin/dce-ccnd-linear-multiple --nNodes=200 --tcp=1 --kernel=0
  $ cat files-199/var/log/30916/status
  Start Time: NS3 Time:          2s (          +2700000000.0ns) , REAL Time: 1328710217
        Time: NS3 Time:          2s (          +2700000000.0ns) , REAL Time: 1328710217 --> Starting: /user/furbani/home/dev/dce/dev/ns-3-dce/build/bin/ccnget
        Time: NS3 Time:          4s (          +4399711801.0ns) , REAL Time: 1328710218 --> Exit (0)
  $ cat files-199/var/log/30916/stdout
  The wanted data is here :)[

You can see that the first get take about 1.6 seconds.
This example produce also a netanim file named *NetAnimLinear.xml* that you can use with the  `NetAnim <http://www.nsnam.org/wiki/index.php/NetAnim>`_ tool in order to visualize packets moving through the Network:

  .. image:: images/netanim-1.png


Example VLC Player
##################

This demonstration show how to watch video using VLC CCN and NS3.

Prerequisite
============

You should be able to build and run the CCN plugin for VLC in order to display Video using CCNx. 
So you should follow carefully the instructions delivered in CCNx distribution in the directory :  ccnx.0.4.0/apps/vlc 

You should ensure that the executable named *tap-creator* is owned by *root* and have the sticky bit setted :

::

   $ cd build/bin
   $ su
   # chown root tap-creator 
   # chmod +s tap-creator

Overview
========

In this example we use other exe than *ccnd*:

1. *ccn_repo* is a CCN repository used to serve the Video file
2. *vlc* the well known media player
3. *ccnputfile* used to fill the repository with our Video file

The two first exe are not usable under DCE:

1. *ccn_repo* is a java program and DCE do not yet supports Java,
2. *vlc* use a graphical interface and DCE do not supports this kind of application.

So the parts *cnn_repo* and *vlc* will be launched normally outside of DCE environnement.
We will also use 3 *ccnd*:

1. the first *ccnd* will be launched normally outside DCE, it will be the server for *vlc* player ,it will use the standard CCNx port ie 9596.
2. the second *ccnd* will be launched inside DCE listening port 2000.
3. the third *ccnd* will be launched normally outside DCE listening port 3000 

then we install ccn routes like this : first *ccnd* forward every interests to second *ccnd* and second *ccnd* forward every interests to third one.

In order to link real world and NS3 network we use the NS-3 TAP BRIDGE functionnality which is more documented there: `Tap NetDevice <http://www.nsnam.org/docs/release/3.12/models/singlehtml/index.html#document-tap>`_

A schema of our network:

::

  +----------+
  | external |
  |  Linux   |
  |   Host   | 1 ccnd on standard port (9596), 1 ccnd on port 3000, 1 repository using ccnd:3000
  |          |
  | "thetap" | 1 vlc client querying ccnx:///VIDEO/bunny.ts
  +----------+
  | 10.0.0.1 |
  +----------+
       |           node0         node1
       |       +----------+    +----------+
       +-------|  tap     |    |          |
               | bridge   |    |          |
               +----------+    +----------+
               |  CSMA    |    |  CSMA    |
               +----------+    +----------+
               | 10.0.0.1 |    | 10.0.0.2 |  ccnd:2000
               +----------+    +----------+
                     |               |
                     |               |
                     |               |
                     =================
                      CSMA LAN 10.0.0


Before running the launch script you should edit it to furnish a Video file in the corresponding variable :

::

  $ vi run-tap-vlc.sh
  .... 
  VIDEOFILE=big_buck_bunny_240p_mpeg4.ts
  .... 

Note also that NS3 is launched in real time mode in order to communicate to real world.

Run :

::

  $ ./run-tap-vlc.sh

If all is right you should see a *vlc* window playing the video, then after 600 seconds the script stops itself
if you interrupt the script before you should terminate real the processes ie:

1. 2 instances of *ccnd*
2. 1 *ccn_repo*
3. and 1 *dce-tap-vlc*

you may also delete ccnd sockets files like */tmp/.ccnd.sock* and */tmp/.ccnd.sock.3000*

Note that if you replay the video (url: ccnx:///VIDEO/bunny.ts) the content should be cached in first *ccnd* so in this case 
NS3/DCE will probably not be used for the second delivery of the video.

