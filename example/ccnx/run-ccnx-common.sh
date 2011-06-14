#!/bin/bash
################################################################################
# Some Paths
# CCNX SOURCES AND BUILD
CCNX_PATH=/user/furbani/home/ccnx/ccnx-0.4.0
# NS3 BIN PATH
NS3_BIN=../../build/bin
# CCNX KEYSTORE 
CCNX_KEY_PATH=$HOME/.ccnx
# CCND REAL KEYSTORE PATH
CCND_REAL_KEYSTORE=/var/tmp/.ccnx-user15019
# VIRTUAL USER KEYSTORE
VIRTUAL_USER_KEYSTORE=/home/furbani
################################################################################
# NS3 LOG LEVEL
#export NS_LOG=
export NS_LOG='*'
#export NS_LOG='Simulator:DceManager:TaskManager:DceStdio'
#export NS_LOG=LocalDatagramSocketFd:DceStdio:SimuFd:TcpSocketBase:Simulator:UnixStreamSocketFd:UnixSocketFd:LocalSocketFd:LocalStreamSocketFd
#export NS_LOG=LocalStreamSocketFd:DceManager
################################################################################
# 
#GDB=
GDB=gdb
# clean previous files
rm -rf files-*
rm output.txt 

install_ccnd_keystore ()
{
    nodeid=$1
    mkdir -p files-$nodeid/var/tmp/
    cp -r $CCND_REAL_KEYSTORE files-$nodeid/var/tmp/.ccnx-user0/
}

install_user_keystore ()
{
    nodeid=$1
    mkdir -p files-$nodeid/$VIRTUAL_USER_KEYSTORE/.ccnx
    cp $CCNX_KEY_PATH/.ccnx_keystore files-$nodeid/$VIRTUAL_USER_KEYSTORE/.ccnx/
}

export PATH=$CCNX_PATH/bin:$PATH
