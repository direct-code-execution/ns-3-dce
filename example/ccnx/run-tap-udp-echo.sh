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
#  | 10.0.0.2 |
#  +----------+
#       |
#     node0
#  +----------+
#  |   dev0   |
#  | 10.0.0.1 | udp-echo-server listening on port 2000
#  +----------+
#
. run-ccnx-common.sh
export TAPCREATOR=/home/furbani/dev/dce/new-one/ns-3-dev/build/debug/src/tap-bridge/tap-creator
EXE=dce-tap-udp-echo
if [ "" == "$GDB" ]
then
    $NS3_BIN/$EXE 2>&1 | tee -a output.txt & 
else
    $GDB $NS3_BIN/$EXE &
fi
sleep 1
$NS3_BIN/udp-echo-client 10.0.0.1 "Hello NS3" >client_out.txt
sleep 9
emacs client_out.txt output.txt  files-*/var/log/*/* &

