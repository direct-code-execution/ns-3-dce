#!/bin/bash
#
#  Should be run with a root user or with a tap-creator with root owner and sticky bit setted.
#
# 
#   +----------+
#   | external |
#   |  Linux   |
#   |   Host   |
#   |          |
#   | "thetap" |
#   +----------+
#   | 10.0.0.1 |
#   +----------+
#        |           node0         node1
#        |       +----------+    +----------+
#        +-------|  tap     |    |          |
#                | bridge   |    |          |
#                +----------+    +----------+
#                |  CSMA    |    |  CSMA    |
#                +----------+    +----------+
#                | 10.0.0.1 |    | 10.0.0.2 | udp-echo-server listening on port 2000
#                +----------+    +----------+
#                      |               |
#                      |               |
#                      |               |
#                      =================
#                       CSMA LAN 10.0.0
# 
cd `dirname $BASH_SOURCE`/../../
BASEDCE=$PWD
cd example/ccnx
if [ -x $BASEDCE/build/bin/dce-tap-udp-echo ]
then
   NS3SCRIPT=$BASEDCE/build/bin/dce-tap-udp-echo 
else
	echo dce-tap-udp-echo not found !
	exit 1
fi
if [ -x $BASEDCE/build/bin_dce/udp-echo-client ]
then
	THECLIENT=$BASEDCE/build/bin_dce/udp-echo-client
else
	echo udp-echo-client not found
	exit 2
fi
echo Running DCE/NS-3 Script : dce-tap-udp-echo
$NS3SCRIPT &
echo sleep one second
sleep 1
echo
echo About to run udp client 
echo
$THECLIENT 10.0.0.2 "Hello NS3"
echo
echo Client exit code : $?
echo 
if [ -f files-1/var/log/39770/stdout ]
then
	echo NS-3 stdout of udp-echo-server '>>>'
	cat files-1/var/log/39770/stdout 
	echo '<<<'
	exit 0
else
	echo NS-3 stdout of udp-echo-server not found should be in file 'files-1/var/log/39770/stdout'
	exit 3
fi 
