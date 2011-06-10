#!/bin/bash
. run-ccnx-common.sh
echo init keystores
for i in 0 1
do
    install_ccnd_keystore $i
    install_user_keystore $i
    mkdir -p files-$i/tmp
    cp ccnd$i.conf files-$i/tmp
done
echo setting test file
cp README files-0/tmp
EXE=dce-ccnd-udp-2-nodes
if [ "" == $GDB ]
then
    $NS3_BIN/$EXE 2>&1 | tee -a output.txt
else
    $GDB $NS3_BIN/$EXE
fi
emacs output.txt  files-*/var/log/*/* &

