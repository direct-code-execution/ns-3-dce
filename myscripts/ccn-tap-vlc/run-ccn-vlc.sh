#!/bin/bash
# 
#  Should be run with a root user or with a ns3-dev-tap-creator-optimized or
#  ns3-dev-tap-creator-debug with root owner and sticky bit setted.
#
#  You should also provide a VLC playable VIDEO file see Variable VIDEOFILE below
# 
#####################################################################################
##
##  Video playing using VLC and CCN for the transport.
##
##  For this test we use :
##
##    A. some real applications on real machine for 2 parts :
##
##     1. the vlc player with the ccn plugin : see CCNX sources in apps/vlc directory
##     2. a local real ccnd running on default port (9596), which role is to serve the VLC client,
##
##    B. Two virtual nodes linked by CSMA with:
##     1. the first node is bridged to the real machine using a tap-bridge,
##     2. the second one run a ccnd where VIDEO interrests and datas will transit,
##        with also a ccnr the repository serving the VIDEO.
##
##  For the need of tap bridge and VIDEO display NS3 is running in Real Time Mode.
##
##  CCN Route:
##          1. the real ccnd(9596) forward interrests starting by /VIDEO to NS3/CCND on node 2
##
##  Steps:  detail in run-ccn-vlc.sh
##
##     1. First launch this DCE exe which do this steps :
##         I   - Create TAP-BRIDGE
##         II  - Launch CCND on NODE 2 (10.0.0.2) at second 1.0
##         III - Launch CCNR on NODE 2 (10.0.0.2) at second 2.0
##         IV  - Publish the file into the repository using this command (at second 5.0) :
##               SyncTest -put  $VIDEOFILE ccnx:///VIDEO/bunny.ts
##         V   - Wait end for 10 minutes.
##
##     2. Launch real CCND:9596
##     3. Route CCND:9596 interrests to CCND:10.0.0.2
##     4. Launch VLC
##     5. Wait 10 minutes.
##     6. stop real ccnd
##
##
##  +----------+
##  | external |
##  |  Linux   |
##  |   Host   | 1 ccnd on standard port (9596)
##  |          |
##  | "thetap" | 1 vlc client querying ccnx:///VIDEO/bunny.ts
##  +----------+
##  | 10.0.0.1 |
##  +----------+
##       |           node0         node1
##       |       +----------+    +----------+
##       +-------|  tap     |    |          |
##               | bridge   |    |          |
##               +----------+    +----------+
##               |  CSMA    |    |  CSMA    |
##               +----------+    +----------+  ccnr
##               | 10.0.0.1 |    | 10.0.0.2 |  ccnd
##               +----------+    +----------+
##                     |               |
##                     |               |
##                     |               |
##                     =================
##                      CSMA LAN 10.0.0
##
##
##
#####################################################################################
## File to watch with VLC
VIDEOFILE=bunny.ts
# TRANSPORT : Use tcp or udp
TRANSPORT=tcp
DATARATE=1800Kbps
PATH=$PATH:$DCE_PATH
for i in ccndstop  ping sleep ccndstart ccndc  ccnput ccnget SyncTest
do
which $i >/dev/null
if [ 1 == $? ]
then
	echo $i not found !
	echo perhaps you forget to set DCE environment variables via setenv.sh ?
	exit 1
fi
done
if [ ! -f $VIDEOFILE ]
then
	echo Video file not found : $VIDEOFILE ?
	exit 2
fi
if [ -x $BASEDCE/build/myscripts/ccn-tap-vlc/bin/dce-ccn-vlc ]
then
    NS3SCRIPT=$BASEDCE/build/myscripts/ccn-tap-vlc/bin/dce-ccn-vlc
else
	echo dce-ccn-vlc not found !
	exit 1
fi
echo Stop ccnd daemon
echo
ccndstop
echo
if [ "" == "$GDB" ]
then
    echo Start NS3 first to create TAP device and 10.0.0.1 the corresponding network address
    echo
    $NS3SCRIPT --dr=$DATARATE | tee -a output.txt &
    echo
    echo Sleep 1s.
    echo
    sleep 1
    echo
    echo Start real ccnd daemon
    echo
    CCND_LOG=ccnd_log.txt ccndstart
    echo
    echo Sleep 10s.
    echo
    sleep 10
    echo
    echo Forward route: ccndc add /VIDEO $TRANSPORT 10.0.0.2 
    echo
    ccndc add /VIDEO $TRANSPORT 10.0.0.2 
    echo
    echo Sleep 15s.
    echo        
    sleep 15
    echo
    echo launch VLC...
    echo
    vlc ccnx:///VIDEO/bunny.ts >vlc_output.txt 2>&1 &
    echo
    echo Sleep 10m.
    echo        
    sleep 600 
    echo
else
    $GDB $NS3SCRIPT
fi
echo stop ccnd
echo
ccndstop
echo Sleep 1s.
echo        
sleep 1
echo



