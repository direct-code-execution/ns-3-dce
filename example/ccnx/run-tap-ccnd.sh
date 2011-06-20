#!/bin/bash
# 
#  Prior running please change the value of TAPCREATOR env var below, it should point on NS3 tap-creator executable,
#  ATTENTION: the tap-creator must be owned by root and have stiky bit in order to function well.
#
#     tap0
#  +----------+
#  | external |
#  |  Linux   |
#  |   Host   |
#  |----------|
#  | 10.0.0.2 | ccnd
#  +----------+
#       |
#     node0
#  +----------+
#  |   dev0   |
#  | 10.0.0.1 | ccnd
#  +----------+
#
#  Interraction between a real CCND and a CCND running within NS3.
#
#  First NS3 is launched, this create the tap0 interface with 10.0.0.2 external address, and 10.0.0.1 internal address. 
#  
#  Then ccnd is launched on local machine with a forwarding to NS3 ccnd,
#
#  a ccnput is done,
#  and within NS3 the ccnget retrieve the file !
#
#
. run-ccnx-common.sh
echo init keystores
for i in 0 
do
    install_ccnd_keystore $i
    install_user_keystore $i
    mkdir -p files-$i/tmp
done
echo setting test file
cp README files-0/tmp
echo launching read ccnd and clients ...
$CCNX_PATH/bin/ccndstop
EXE=dce-tap-ccnd
export TAPCREATOR=/home/furbani/dev/dce/new-one/ns-3-dev/build/debug/src/tap-bridge/tap-creator
if [ "" == "$GDB" ]
then
    $NS3_BIN/$EXE 2>&1 | tee -a output.txt &
    sleep 1
    ping -c 1 10.0.0.1
    CCND_DEBUG=-1 CCND_LOG=ccnd_log.txt $CCNX_PATH/bin/ccndstart
    sleep 1
    $CCNX_PATH/bin/ccndc add / tcp 10.0.0.1 2000
    sleep 1
    $CCNX_PATH/bin/ccnput /REALWORLD/README < README &
    sleep 44
else
    $GDB $NS3_BIN/$EXE
fi
$CCNX_PATH/bin/ccndstop
emacs output.txt  files-*/var/log/*/* &

