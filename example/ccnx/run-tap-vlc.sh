#!/bin/bash
# 
#  Should be run with a root user or with a tap-creator with root owner and sticky bit setted.
#
#  You should also provide a VLC playable VIDEO file see Variable VIDEOFILE below
# 
############################################################################
##
##  Video playing using VLC and CCN for the transport.
##
##  For this test we use :
##
##    A. some real applications on real machine for 4 parts :
##
##     1. the vlc player with the ccn plugin : see CCNX sources in apps/vlc directory
##     2. a local real ccnd running on default port (9596), which role is to serve the VLC client,
##     3. a local real ccnd running on port 3000, which role is to serve the CCN REPOSITORY
##     4. the CCN repository talking to CCND:3000 (because it is written in Java Not Yet Supported by DCE).
##
##    B. Two virtual nodes linked by CSMA with:
##     1. the first node is bridged to the real machine using a tap-bridge,
##     2. the second one run a ccnd where VIDEO interrests and datas will transit.
##
##  For the need of tap bridge and VIDEO display NS3 is running in Real Time Mode.
##
##  CCN Routes:
##          1. ccnd(9596) forward every interrests to NS3/CCND(2000)
##          2. NS3/CCND(2000) forward every interrests to CCND(3000)
##
##  Steps:  detail in run-tap-vlc.sh
##
##     1. First launch this DCE exe which do this steps :
##         I   - Create TAP-BRIDGE
##         II  - Launch CCND on NODE 2 (10.0.0.2) and port 2000
##         III - Configure this CCND to forward interrests to CCND on 10.0.0.1:3000
##         IV  - Wait end for 10 minutes.
##
##     2. Launch real CCND:9596
##     3. Launch real CCND:3000
##     4. Launch CCN REPOSITORY
##     5. Route CCND:9596 interrests to CCND:10.0.0.2:2000
##     6. Put the video file to the repository
##     7. Launch VLC
##     8. Wait 10 minutes.
##     9. stop all daemons
##
##
##  +----------+
##  | external |
##  |  Linux   |
##  |   Host   | 1 ccnd on standard port (9596), 1 ccnd on port 3000, 1 repository using ccnd:3000
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
##               +----------+    +----------+
##               | 10.0.0.1 |    | 10.0.0.2 |  ccnd:2000
##               +----------+    +----------+
##                     |               |
##                     |               |
##                     |               |
##                     =================
##                      CSMA LAN 10.0.0
##
##
##
##
##############################################################################
## File to watch with VLC
VIDEOFILE=big_buck_bunny_240p_mpeg4.ts
# TRANSPORT : Use tcp or udp
TRANSPORT=tcp
cd `dirname $BASH_SOURCE`/../../
BASEDCE=$PWD
cd example/ccnx
PATH=$PATH:$DCE_PATH
for i in ccndstop  ping sleep ccndstart ccndc  ccnput ccnget
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
if [ -x $BASEDCE/build/bin/dce-tap-vlc ]
then
    NS3SCRIPT=$BASEDCE/build/bin/dce-tap-vlc
else
	echo dce-tap-vlc not found !
	exit 1
fi
echo Stop ccnd daemon
echo
ccndstop
echo
EXE=dce-tap-vlc
if [ "" == "$GDB" ]
then
    echo Start NS3 first to create TAP device and 10.0.0.1 the corresponding network address
    echo
    $NS3SCRIPT --transport=$TRANSPORT 2>&1 | tee -a output.txt &
    echo
    echo Sleep 1s.
    echo
    sleep 1
    echo
    echo Start real ccnd daemon
    echo
    CCND_DEBUG=-1 CCND_LOG=ccnd_log.txt ccndstart
    echo
    echo Sleep 1s.
    echo
    sleep 1
    echo
    echo Start real ccnd daemon using port 3000
    echo
	CCN_LOCAL_PORT=3000 CCND_DEBUG=-1 CCND_LOG=ccnd_log_3.txt ccndstart
	echo
    echo Sleep 1s.
    echo
    sleep 1 
    echo 
    echo Start ccn repo
    echo
    CCN_LOCAL_PORT=3000 ccn_repo REPOSITORY_CCN
    echo
    echo Sleep 1s.
    echo    
    sleep 1
    echo
    echo Forward route: ccndc add / $TRANSPORT 10.0.0.2 2000
    echo
    ccndc add / $TRANSPORT 10.0.0.2 2000
    echo
    echo Sleep 1s.
    echo        
    sleep 1
    echo
    echo use ccnpufile to publish video
    echo
    CCN_LOCAL_PORT=3000 ccnputfile ccnx:///VIDEO/bunny.ts $VIDEOFILE
    echo
    echo Sleep 6s.
    echo        
    sleep 6
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
    $GDB $NS3_BIN/$EXE
fi
echo stop ccnd
echo
ccndstop
echo Sleep 1s.
echo        
sleep 1
echo
echo stop repo
echo
CCN_LOCAL_PORT=3000 ccn_repo stopall
echo        
echo Sleep 1s.
echo
sleep 1
echo stop ccnd 3000
echo
CCN_LOCAL_PORT=3000 ccndstop
echo Sleep 1s.
echo
sleep 1
echo


