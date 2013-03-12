.. include:: replace.txt

DCE in a Nutshell
=================

File System
-----------

To start a program in the world of |ns3| you must indicate on which node it will be launched. Once launched this program will have access only to the file system corresponding to the node that corresponds to a directory on your machine called file-X where X is the decimal number of the corresponding node. The file-X directories are created by DCE, only when they do not already exist. Also **note** that the contents of this directory is not cleared when starting the script. So you can copy the files required for the operation of your executables in the tree nodes. If possible it is best that you create these files from the script itself in order to simplify maintenance. DCE provides some helpers for creating configuration files necessary to the execution of certain apps like CCNx and Quagga.

Network
-------

Your program running in a |ns3| node views the network defined by the script for this node.

Time
----

Time perceived by your executable is the simulated time of |ns3|. Also **note** that DCE supports real time scheduler of |ns3| with the same limitations.
