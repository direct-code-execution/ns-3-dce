#!/bin/bash
. run-ccnx-common.sh
echo init keystores
NNODES=20
for (( i=0; i<$NNODES; i++ ))
do
    install_ccnd_keystore $i
    install_user_keystore $i
    mkdir -p files-$i/tmp
#    cp ccnd$i.conf files-$i/tmp
#    echo 'GET / ' > files-$i/tmp/getSlash.txt
done
echo setting test file
cp README files-0/tmp
EXE=dce-ccnd-linear-multiple
#FIBER=--ns3::TaskManager::FiberManagerType=UcontextFiberManager
MINFILES=--ns3::DceManager::MinimizeOpenFiles=1
#NS_LOG=$NS_LOG:CcndInLine
# Comment out to use TCP instead of UDP
USE_TCP=--tcp=1
KERN=--kernel=1
echo Run NS3
if [ "" == "$GDB" ]
then
#valgrind --leak-check=full
     $NS3_BIN/$EXE $FIBER --nNodes=$NNODES $USE_TCP $KERN $MINFILES 2>&1 | tee -a output.txt
else
    echo  $FIBER --nNodes=$NNODES $USE_TCP $KERN $MINFILES
    $GDB $NS3_BIN/$EXE
fi

