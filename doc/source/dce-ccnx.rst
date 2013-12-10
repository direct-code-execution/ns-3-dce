.. include:: replace.txt

.. From Bake-ed getting started doc

CCNx examples
*************

Under example/ccnx there is more realistics examples using the implementation of an experimental protocol named CCN. 
In this examples we use the `PARC  <http://www.parc.com>`_ implementation named `CCNx <http://www.ccnx.org>`_ (c) in its early version 0.6.2.
Bake has already installed for you **Ccnx**.
  
Example CCNX-SIMPLE
===================

This simulation launches a *ccnd* daemon, publishes a file using *ccnput* and retrieves this data using *ccnget* command, all commands are on a single node:

::

  $ source bakeSetEnv.sh
  $ dce-ccnd-simple 

Verify the status of execution:

::

  $ cat files-0/var/log/*/status
  Start Time: NS3 Time:          0s (                   +0.0ns) , REAL Time: 1328707904
        Time: NS3 Time:          0s (                   +0.0ns) , REAL Time: 1328707904 --> Starting: /tmp/ccn/build/bin/ccnd
        Time: NS3 Time:         59s (         +59001000000.0ns) , REAL Time: 1328707905 --> Exit (0)
  Start Time: NS3 Time:          1s (          +1000000000.0ns) , REAL Time: 1328707904
        Time: NS3 Time:          1s (          +1000000000.0ns) , REAL Time: 1328707904 --> Starting: /tmp/ccn/build/bin/ccnput
        Time: NS3 Time:          2s (          +2001000000.0ns) , REAL Time: 1328707905 --> Exit (0)
  Start Time: NS3 Time:          2s (          +2000000000.0ns) , REAL Time: 1328707905
        Time: NS3 Time:          2s (          +2000000000.0ns) , REAL Time: 1328707905 --> Starting: /tmp/ccn/build/bin/ccnget
        Time: NS3 Time:          2s (          +2002000000.0ns) , REAL Time: 1328707905 --> Exit (0)
  Start Time: NS3 Time:         59s (         +59000000000.0ns) , REAL Time: 1328707905
        Time: NS3 Time:         59s (         +59000000000.0ns) , REAL Time: 1328707905 --> Starting: /tmp/ccn/build/bin/ccndsmoketest
        Time: NS3 Time:         59s (         +59001000000.0ns) , REAL Time: 1328707905 --> Exit (0)

Verify the output of the command *ccnget*:

::

  $ cat files-0/var/log/53514/stdout
  The wanted data is here :)

Example CCND LINEAR MULTIPLE
============================

This simulation uses multiple nodes placed in a line, each node are linked 2 by 2 by a point to point link, each node holds a ccnd daemon, the first node put a file (with ccnput), and the last node fetch this file (with ccnget). Also each node minus the first one forward interrests starting with /NODE0 to its predecessor.

  .. image:: images/ccnd-linear-multiple-1.png

The launch script dce-ccnd-linear-multiple offer 3 options:

::

  $ dce-ccnd-linear-multiple --PrintHelp
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
 3. kernel allows to use Linux IP Stack (only working in advanced mode) instead of |ns3| one.

for example with 200 nodes and TCP transport you should see this in the first ccnget output command:

::

  $ dce-ccnd-linear-multiple --nNodes=200 --tcp=1 --kernel=0
  $ cat files-199/var/log/30916/status
  Start Time: NS3 Time:          2s (          +2700000000.0ns) , REAL Time: 1328710217
        Time: NS3 Time:          2s (          +2700000000.0ns) , REAL Time: 1328710217 --> Starting: build/bin/ccnget
        Time: NS3 Time:          4s (          +4399711801.0ns) , REAL Time: 1328710218 --> Exit (0)
  $ cat files-199/var/log/30916/stdout
  The wanted data is here :)[

You can see that the first get take about 1.6 seconds.
This example produce also a netanim file named *NetAnimLinear.xml* that you can use with the  `NetAnim <http://www.nsnam.org/wiki/index.php/NetAnim>`_ tool in order to visualize packets moving through the Network:

  .. image:: images/netanim-1.png


  

Example VLC Player
==================

This demonstration show how to watch video using VLC CCN and NS3.

Prerequisite
------------

You should be able to build and run the CCN plugin for VLC in order to display Video using CCNx. 
So you should follow carefully the instructions delivered in CCNx distribution in the directory :  ccnx-0.6.1/apps/vlc 

You should ensure that the executable named *ns3-dev-tap-creator-debug* is owned by *root* and have the sticky bit setted :

::

   $ cd build/bin
   $ su
   # chown root ns3-dev-tap-creator-debug
   # chmod +s ns3-dev-tap-creator-debug

Overview
--------

In this example we use other exe than *ccnd*:

1. *vlc* the well known media player
2. *ccnr* is a CCN repository used to serve the Video file
3. *SyncTest* used to fill the repository with our Video file

The first exe is not usable under DCE: *vlc* use a graphical interface and DCE do not supports this kind of application.

So *vlc* will be launched normally outside of DCE environment.
We will also use 2 *ccnd*:

1. the first *ccnd* will be launched normally outside DCE, it will be the server for *vlc* player ,it will use the standard CCNx port ie 9596.
2. the second *ccnd* will be launched inside DCE listening on second node.

then we install ccn route like this : first *ccnd* forward every interests starting by /VIDEO/ to second *ccnd*.

In order to link real world and NS3 network we use the |ns3| TAP BRIDGE functionnality which is more documented there: `Tap NetDevice <http://www.nsnam.org/docs/release/3.12/models/singlehtml/index.html#document-tap>`_

A schema of our network:

::

  +----------+
  | external |
  |  Linux   |
  |   Host   | 1 ccnd on standard port (9596)
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
               | 10.0.0.1 |    | 10.0.0.2 |  ccnd and ccnr
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

  $ cd $BASEDCE/myscripts/ccn-tap-vlc
  $ ./run-ccn-vlc.sh

If all is right you should see a *vlc* window playing the video, then after 600 seconds the script stops itself
if you interrupt the script before you should terminate real the processes ie:

1. 1 instances of *ccnd*
2. and 1 *dce-ccn-vlc*

Note that if you replay the video (url: ccnx:///VIDEO/bunny.ts) the content should be cached in first *ccnd* so in this case 
NS3/DCE will probably not be used for the second delivery of the video.

