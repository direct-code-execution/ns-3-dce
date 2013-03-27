#!/bin/sh
OUTPUT="output/`date \"+%y%m%d_%H%M\"`"
mkdir -p ${OUTPUT}
TRIAL="`seq 1 100`"
TRIAL_TIME="`seq 1 5`"
NODES="`seq 1 2 100`"
DURATIONS="`seq 10 10 400`"
STACKS="dce dce-linux3 ns3 nsc-linux dce-dccp dce-dccp3"
PCAP=""

for stack in $STACKS
do
for trial in $TRIAL
do
if [ $trial -eq 1 ] ; then
PCAP="--enablePcap=1"
else
PCAP="--enablePcap=0"
fi

echo -n "$stack $trial " >>$OUTPUT/goodput.txt
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=UcontextFiberManager' ./build/bin/dce-runner ./build/bin/dce-tcp-ns3-nsc-comparison --stack=$stack --seed=$trial ${PCAP} --stopTime=64 | awk '{print $6}' | sed s/\(// >> $OUTPUT/goodput.txt
done

for trial in $TRIAL_TIME
do
for node in $NODES
do
echo -n "$stack $node $trial " >>$OUTPUT/sim-time-nnodes.txt
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=UcontextFiberManager' /usr/bin/time -p ./build/bin/dce-runner ./build/bin/dce-tcp-ns3-nsc-comparison --stack=$stack --nNodes=$node --seed=$trial -stopTime=64 2>&1 | grep real | awk '{print $2}' >> ${OUTPUT}/sim-time-nnodes.txt
done

for duration in $DURATIONS
do
echo -n "$stack $duration $trial " >>$OUTPUT/sim-time-duration.txt
NS_ATTRIBUTE_DEFAULT='ns3::TaskManager::FiberManagerType=UcontextFiberManager' /usr/bin/time -p ./build/bin/dce-runner ./build/bin/dce-tcp-ns3-nsc-comparison --stack=$stack --stopTime=$duration --seed=$trial 2>&1 | grep real | awk '{print $2}' >> ${OUTPUT}/sim-time-duration.txt
done
done

done
