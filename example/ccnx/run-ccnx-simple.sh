#!/bin/bash
. run-ccnx-common.sh
echo init keystores
install_ccnd_keystore 0
install_user_keystore 0
echo setting test file
mkdir -p files-0/tmp
cp $CCNX_PATH/README files-0/tmp
EXE=dce-ccnd-simple
if [ "" == "$GDB" ]
then
    $NS3_BIN/$EXE 2>&1 | tee -a output.txt
else
    $GDB $NS3_BIN/$EXE
fi
emacs output.txt  files-0/var/log/*/* &

