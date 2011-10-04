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
. run-ccnx-common.sh
EXE=dce-tap-udp-echo
if [ "" == "$GDB" ]
then
    $NS3_BIN/$EXE 2>&1 | tee -a output.txt & 
else
    $GDB $NS3_BIN/$EXE 
    goto fin
fi
sleep 1
$NS3_BIN/udp-echo-client 10.0.0.2 "Hello NS3" >client_out.txt
sleep 1
emacs client_out.txt output.txt  files-*/var/log/*/* &

fin:

