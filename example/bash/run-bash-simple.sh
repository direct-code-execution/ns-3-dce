#!/bin/bash
SHELL_BIN=$HOME/bin_dce/sh
OPENSSL=$HOME/local
. run-ccnx-common.sh
mkdir -p files-0/tmp
#cp $CCNX_PATH/bin/ccnd-init-keystore-helper files-0/tmp
cp ccnd-init-keystore-helper files-0/tmp
echo \010\043\103\375\327\237\152\351\155 >files-0/p
#mkdir -p files-0/bin
#cp $SHELL_BIN files-0/bin
cp -r $HOME/bin_dce files-0/bin
mkdir -p files-0/$OPENSSL
cp -r $OPENSSL files-0/$OPENSSL/..
EXE=dce-bash-simple
if [ "" == "$GDB" ]
then
    $NS3_BIN/$EXE 2>&1 | tee -a output.txt
else
    $GDB $NS3_BIN/$EXE
fi
emacs output.txt  files-0/var/log/*/* &

