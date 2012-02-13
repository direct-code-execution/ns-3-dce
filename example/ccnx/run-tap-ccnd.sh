#!/bin/bash
# 
#  Should be run with a root user or with a tap-creator with root owner and sticky bit setted.
#
#
#  +----------+
#  | external |
#  |  Linux   |
#  |   Host   |
#  |          |
#  | "thetap" |
#  +----------+
#  | 10.0.0.1 | ccnd
#  +----------+
#       |           node0         node1
#       |       +----------+    +----------+
#       +-------|  tap     |    |          |
#               | bridge   |    |          |
#               +----------+    +----------+
#               |  CSMA    |    |  CSMA    |
#               +----------+    +----------+
#               | 10.0.0.1 |    | 10.0.0.2 |  ccnd
#               +----------+    +----------+
#                     |               |
#                     |               |
#                     |               |
#                     =================
#                      CSMA LAN 10.0.0
#
#
#  Interraction between a real CCND and a CCND running within NS3.
#
#  First NS3 is launched, this create the tap-bridge with 10.0.0.1 address
#
#  Then ccnd is launched on local machine with a forwarding to NS3 ccnd listening on 10.0.0.2,
#
#  a ccnput is done,
#  and within NS3 the ccnget retrieve the file !
#
#  then the local machine do also a ccnget to retrieve a file from within NS3.
#
CV=5
cd `dirname $BASH_SOURCE`/../../
BASEDCE=$PWD
cd example/ccnx
PATH=$PATH:$DCE_PATH
for i in ccndstop  ping sleep ccndstart ccndc ccnput ccnget
do
which $i >/dev/null
if [ 1 == $? ]
then
	echo $i not found !
	echo perhaps you forget to set DCE environment variables via setenv.sh ?
	exit 1
fi
done
if [ -x $BASEDCE/build/bin/dce-tap-ccnd ]
then
    NS3SCRIPT="$BASEDCE/build/bin/dce-tap-ccnd --cv=$CV"
else
	echo dce-tap-ccnd not found !
	exit 1
fi
echo Stopping ccnd daemon
echo
ccndstop
echo
echo Launch NS-3 script : dce-tap-ccnd
echo
$NS3SCRIPT &
echo
echo Sleep 1s.
sleep 1
echo
echo ping -c 1 10.0.0.2
ping -c 1 10.0.0.2 &
echo
echo Launch real ccnd daemon 
echo
CCND_DEBUG=-1 CCND_LOG=ccnd_log.txt ccndstart
echo
echo Sleep 1s.
sleep 1
echo
echo Setting interrest forward route : ccndc add /NS3WORLD tcp 10.0.0.2 2000
echo
ccndc add /NS3WORLD tcp 10.0.0.2 2000
echo
echo Sleep 1s.
sleep 1
echo
echo 'Publish a file' : ccnput /REALWORLD/README < README &
echo
ccnput /REALWORLD/README < README &
echo
echo Sleep 1s.
sleep 1
echo
echo 'Getting a file' : ccnget -c /NS3WORLD/README >ccnget_result.txt 2>&1
echo
ccnget -c /NS3WORLD/README >ccnget_result.txt 2>&1
echo
echo Sleep 6s.
sleep 6
echo
echo Stopping ccnd daemon
echo
ccndstop
echo
echo done
echo you may type 
echo more $PWD'/files-1/var/log/*/*' 
echo in order to look at NS-3 results files.


