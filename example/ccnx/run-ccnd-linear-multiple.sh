#!/bin/bash
. run-ccnx-common.sh
echo init keystores
NNODES=100
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
FIBER=--ns3::TaskManager::FiberManagerType=UcontextFiberManager
NS_LOG=$NS_LOG:CcndInLine
# Comment out to use TCP instead of UDP
USE_TCP=--tcp=1
echo Run NS3
if [ "" == "$GDB" ]
then
    $NS3_BIN/$EXE $FIBER --nNodes=$NNODES $USE_TCP 2>&1 | tee -a output.txt
else
    $GDB $NS3_BIN/$EXE $FIBER --nNodes=$NNODES $USE_TCP
fi
emacs output.txt  files-*/var/log/*/* &

# avec UcontextFiberManager
#real	0m43.150s
#user	0m21.672s
#sys	0m14.837s
# et sans :
#real	0m49.192s
#user	0m24.258s
#sys	0m18.596s
# 500 nodes avec
#real	2m56.723s
#user	1m29.995s
#sys	1m20.047s
