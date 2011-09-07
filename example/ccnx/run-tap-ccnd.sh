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
. run-ccnx-common.sh
echo init keystores
for i in 1
do
    install_ccnd_keystore $i
    install_user_keystore $i
    mkdir -p files-$i/tmp
done
echo setting test file
cp README files-1/tmp
echo launching read ccnd and clients ...
$CCNX_PATH/bin/ccndstop
EXE=dce-tap-ccnd
if [ "" == "$GDB" ]
then
    $NS3_BIN/$EXE 2>&1 | tee -a output.txt &
    sleep 1
    ping -c 1 10.0.0.2 &
    CCND_DEBUG=-1 CCND_LOG=ccnd_log.txt $CCNX_PATH/bin/ccndstart
    sleep 1
    $CCNX_PATH/bin/ccndc add /NS3WORLD tcp 10.0.0.2 2000
    sleep 1
    $CCNX_PATH/bin/ccnput /REALWORLD/README < README &
    sleep 1
   	$CCNX_PATH/bin/ccnget -c /NS3WORLD/README >ccnget_result.txt 2>&1
    sleep 6
else
    $GDB $NS3_BIN/$EXE
fi
$CCNX_PATH/bin/ccndstop
emacs output.txt  files-*/var/log/*/* &

